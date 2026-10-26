# CUDA Notes

What I built for the GPU equity backend, why, and what the measurements showed.
Hardware: NVIDIA A10 (sm_86, Lambda Cloud) vs the box's 30-vCPU host. CUDA 12.8.

## 0. Where things live

```
engine/include/equity/equity.h   public API: compute_equity(request, options), Backend, gpu_available()
engine/src/common/               shared: workload.* (parse/ranges/matchups), hand_eval.h, combinatorics.h (HD)
engine/src/cpu/                  enumerate.cpp (core: cpu_count_matchup = one exact matchup, all runouts;
                                 cpu_count_single = loop over matchups), multithread.cpp (slices)
engine/src/gpu/                  gpu_backend.cu (kernels), gpu_stub.cpp (no-CUDA builds)
engine/src/equity.cpp            dispatch: prepare the Workload once, pick a backend, build the result
```

- Backends only **count**: each takes the same `Workload` and returns `EquityCounts`.
  The dispatcher owns timing, percentages, and the `wins + losses + ties == total` check.
- The **math is shared** (`hand_eval.h`, `combinatorics.h`, all `HD`); only the
  orchestration differs (threads vs grid, host vectors vs device buffers, summing
  partials vs warp shuffle + atomics). A backend-specific evaluator is only worth adding
  if a benchmark proves it faster, and it must still match `hand_eval.h` exactly.
- `Backend::Auto`: GPU if the job has at least `autoGpuMinRunouts` (default 200k)
  runouts **and** a GPU is present; otherwise `CpuMulti`. The size is checked first,
  so small jobs never touch the CUDA runtime.
- Caveat: the first GPU call in a process pays ~120 ms of CUDA context creation
  (`equity_cli AsAh KsKd` on Auto: 129 ms on the GPU vs 6 ms on `cpu-multi`).
  Fine for a long-running caller (the solver pays it once); for one-shot CLI runs on
  mid-size jobs, `--backend cpu-multi` is faster.

## 1. One evaluator for CPU and GPU (`engine/src/common/hand_eval.h`)

- A `__global__` kernel can only call `__device__` / `__host__ __device__`
  functions, so the old `evaluator.cpp` (plain host code) couldn't run on the GPU.
- Fix: a header-only evaluator marked `HD` (`__host__ __device__` under nvcc,
  nothing under g++). Same source compiled by both compilers → GPU and CPU agree
  bit-for-bit, so the GPU can be tested against the CPU exactly.
- Device code can't use `std::vector`, `<algorithm>`, or exceptions, so it's all
  fixed arrays + bitmasks: a 13-bit rank mask, one 13-bit mask per suit, and rank
  counts. Straights: `m & m>>1 & m>>2 & m>>3 & m>>4` on the rank mask (ace copied
  to bit 0 for the wheel). Popcount / highest bit use `__popc` / `__clz` on the
  device and `__builtin_popcount` / `__builtin_clz` on the host (`#ifdef __CUDA_ARCH__`).
- Score = `(category << 20) | five 4-bit ranks`; only comparisons matter.
- Checked against the old evaluator on 20M random deals: 0 mismatches.
- **Side effect on the CPU:** no heap allocation per evaluation → preflop
  enumeration ~5x faster (iterative 1185 → 233 ms, multithread 390 → 67 ms on an
  8-core laptop, same harness).

## 2. The kernels (`engine/src/gpu/gpu_backend.cu`)

Same work as the CPU, every (matchup, runout) pair, numbered as one list: index `i` in
`[0, total)`, `total = matchups × R` (R = runouts per matchup), `m = i / R`, `runout = i % R`;
runout `r` = the r-th lexicographic k-subset of the cards not used by matchup `m`'s hole
cards or the known board. All matchups go in ONE kernel launch.

- **Grid-stride:** thread `t` does `i = t, t + stride, t + 2*stride, ...`. Neighbouring
  threads take neighbouring indices (same matchup → the matchup load is a broadcast),
  but a thread's consecutive indices are far apart, so it **unranks every index**.
  The unrank walks the free cards with `__ffsll` + `avail &= avail - 1` and writes cards
  straight into the board — no deck array.
- **Chunked:** thread `t` owns `[t*chunk, (t+1)*chunk)`: build that matchup's deck once,
  unrank once, then step with `next_combination` (what the CPU core `cpu_count_matchup` does).
- `C(n,k)` lookup table (53×6) and the known board live in `__constant__` memory.
- Per-thread local counters; at the end **warp shuffle** (`__shfl_down_sync`) →
  shared memory → **one `atomicAdd` per counter per block**.
- Launch config from `cudaOccupancyMaxPotentialBlockSize` (block size that maximizes
  occupancy + the grid size that fills every SM once), capped by the amount of work.
- RAII wrappers (`DeviceBuffer`, `PinnedBuffer`, `CudaEvent`, `CudaStream`) so nothing
  leaks when a `CUDA_CHECK` throws. The dispatcher asserts `wins + losses + ties == total`.

## 3. Benchmarks (A10 vs 30-vCPU host, warm, medians)

`./build/equity_bench`. "total" = whole call (parse, matchup build, upload, kernel,
copy back); "kernel" = cudaEvent-timed kernel only. All backends return identical results.

| case | runouts | CPU 1 thread | CPU 30 threads | GPU grid-stride | GPU chunked |
|---|---:|---:|---:|---:|---:|
| preflop AsAh vs KsKd | 1.7M | 146 ms | 5.9 ms | **0.40 ms** (kernel 0.31) | 0.46 ms (0.38) |
| preflop AA,KK,QQ,AKs vs JJ,TT,AQs,KQs | 658M | — | 2088 ms | **137 ms** | 211 ms |
| flop, full range vs full range | 1.26B | — | 4102 ms | **257 ms** | 258 ms |
| river, full range vs full range | 1.07M | — | 10.0 ms | 2.8 ms (kernel 0.17) | 3.1 ms (0.32) |

(River numbers are after the backend restructure: matchup building now reserves its
vector up front, which cut the host-side prep from ~7 ms to ~2.5 ms.)

- Throughput: GPU ≈ **4.9 G runouts/s** (≈ 9.8 G hand evaluations/s) vs ≈ 310 M/s for
  30 CPU threads and 11.7 M/s for one → **~16x** the whole CPU, **~420x** one core.
- The first CUDA call in a process pays ~100+ ms of context creation; the bench does a
  warm-up run so that's excluded (a one-shot `equity_cli --gpu` sees it).

## 4. Design decisions the numbers settled

**Grid-stride vs chunked → grid-stride is the default.** Chunked does far fewer
unranks, yet it's equal or slower (1.5x slower on the 658M case). Why:
- `Card deck[48]` indexed by `combo[j]` is dynamically indexed → lives in **local
  memory** (off-chip, cached), not registers.
- More registers per thread → occupancy API picks 72 × 1024 threads vs 144 × 768 for
  grid-stride → fewer resident warps to hide latency.
- `next_combination` has data-dependent loop lengths and every thread in a warp is at a
  different point in its sequence → **branch divergence**.
- Grid-stride's per-index unrank is ~ (≤48 loop steps + constant-memory reads), and
  adjacent threads follow nearly the same path, so it's cheaper than it looks.

**Block reduction vs per-thread atomics.** 110k threads all hitting the same three
counters is a contention point, but it's a fixed cost at the end of the kernel:
- preflop 1.7M: 0.31 ms (block reduce) vs 0.51 ms (per-thread atomics) — matters.
- 658M / 1.26B jobs: identical within noise — the atomics are amortized away.

**Amdahl on the river case.** Kernel 0.17 ms, total ~2.8 ms: most of the rest is the host
building ~1M matchups (`prepare_workload`). Making the GPU faster can't help that case;
moving matchup generation to the GPU (or caching it) would.

## 5. Pinned memory experiment

Upload of the matchup list (1.07M matchups × 4 B = 4.3 MB, river and flop full-range cases):

| upload path | transfer (cudaEvent) | extra host prep | call total |
|---|---:|---:|---:|
| pageable `cudaMemcpyAsync` | 0.28 ms | — | 3.1 ms |
| pinned (`cudaMallocHost` + memcpy staging, async on a stream) | 0.18 ms | 1.4 ms | 4.8 ms |

- The **transfer** is ~1.75x faster pinned: the GPU DMAs straight from page-locked
  memory instead of the driver bouncing pageable memory through its own pinned buffer.
- But allocating pinned memory is expensive (it page-locks with the OS) and the extra
  staging copy costs more than the transfer saved → **net slower** per call here.
- It's a tiny fraction of the work anyway: 0.3 ms of upload vs 250 ms of kernel on the
  flop case, and nothing at all is uploaded for exact-vs-exact (24 bytes come back).
- Pinned memory pays off when transfers are large relative to compute **and** the pinned
  buffer is allocated once and reused (or data is produced directly into it), ideally
  with async copies on streams overlapping kernels (e.g. batching many queries).
  Kept as an option (`GpuOptions::pinnedUpload`) for that use, off by default.

## 6. Build notes

- CUDA is optional: root `CMakeLists.txt` uses `check_language(CUDA)`; with nvcc it
  builds `gpu/gpu_backend.cu` + `test_equity_gpu`, without it `gpu/gpu_stub.cpp`
  (`gpu_available()` is false and `Backend::Gpu` throws).
- `CMAKE_CUDA_ARCHITECTURES` defaults to 86 (A10). CMake 3.22 on the box predates
  `native`; override with `-DCMAKE_CUDA_ARCHITECTURES=...` on other GPUs.
- Default build type is Release — benchmarks at -O0 are meaningless.

## Ideas not done

- Generate matchups on the GPU (fixes the river-case Amdahl bound).
- Reduce local memory in the evaluator (`counts[13]` is dynamically indexed).
- Lookup-table evaluator (fewer branches → less divergence).
- Batch many requests with streams + reused pinned buffers.
- CFR on the GPU (needs MCCFR-style sampled traversals; tree CFR doesn't map cleanly).
