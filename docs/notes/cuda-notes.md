# CUDA Notes

What I built for the GPU equity backend, why, and what the measurements showed.
Hardware: NVIDIA A10 (sm_86, Lambda Cloud) vs the box's 30-vCPU host. CUDA 12.8.

## 0. Where things live

```
engine/include/equity/equity.h   public API: compute_equity(request, options), Backend, gpu_available()
engine/src/common/               shared: workload.* (parse/ranges/matchups), hand_eval.h, combinatorics.h (HD)
engine/src/cpu/                  enumerate.cpp (core: cpu_count_matchup = one exact matchup, all runouts;
                                 cpu_count_single = loop over matchups), multithread.cpp (slices)
engine/src/gpu/                  runouts.cuh (runout number -> cards), matchup.cuh (core: one runout),
                                 reduce.cuh (warp/block sum + atomicAdd), kernels.cuh (grid-stride loop),
                                 cuda_utils.cuh (CUDA_CHECK + RAII), gpu_backend.cu (host: upload,
                                 launch, timing), gpu_stub.cpp (no-CUDA builds)
engine/src/equity.cpp            dispatch: prepare the Workload once, pick a backend, build the result
```

- Backends only **count**: each takes the same `Workload` and returns `EquityCounts`.
  The dispatcher owns timing, percentages, and the `wins + losses + ties == total` check.
- The **math is shared** (`hand_eval.h`, `combinatorics.h`, all `HD`); only the
  orchestration differs (threads vs grid, host vectors vs device buffers, summing
  partials vs warp shuffle + atomics).
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

## 2. The GPU code (`engine/src/gpu/`)

Split by layer, like the CPU side (core function + wrappers). The `.cuh` files are
header-only `__device__` code included into the one `.cu`: calling device functions
across `.cu` files needs relocatable device code (`-rdc`), which blocks inlining.

- **`gpu_backend.cu` (host):** upload the C(n,k) table + known board to `__constant__`
  memory, `cudaMalloc` the matchups + 3 counters, upload the matchups, pick the launch
  config, launch, copy the 3 counters back. All queued on one stream; the final
  `cudaStreamSynchronize` is the only CPU wait. RAII wrappers (`DeviceBuffer`,
  `CudaEvent`, `CudaStream`) free everything even if a `CUDA_CHECK` throws.
- **`kernels.cuh` (schedule):** the work is every (matchup, runout) pair numbered
  `i = m * R + runout` in `[0, total)`; all matchups go in ONE launch. Grid-stride loop:
  thread `t` does `i = t, t + stride, t + 2*stride, ...` (stride = threads in the grid).
- **`matchup.cuh` (core):** `play_runout` = unrank runout `r` into the board, then
  `hand_eval::compare`. `DeviceCounts` = the thread's private counters (registers).
- **`runouts.cuh` (enumerate):** `unrank_cards` walks the unseen cards as a 64-bit mask
  with `__ffsll` + `avail &= avail - 1` and writes cards straight into the board — no
  deck array. `c_choose` (53×6) and `c_known` live in `__constant__` memory.
- **`reduce.cuh` (sum):** warp shuffle (`__shfl_down_sync`) → shared memory → one
  `atomicAdd` per counter per block.
- Launch config from `cudaOccupancyMaxPotentialBlockSize` (144 blocks × 768 threads on
  the A10), capped by the amount of work.

## 3. Benchmarks (A10 vs 30-vCPU host, warm, medians)

`./build/equity_bench`. "total" = whole call (parse, matchup build, upload, kernel,
copy back); "kernel" = cudaEvent-timed kernel only. All backends return identical results.

| case | runouts | CPU 1 thread | CPU 30 threads | GPU |
|---|---:|---:|---:|---:|
| preflop AsAh vs KsKd | 1.7M | 146 ms | 5.9 ms | **0.43 ms** (kernel 0.31) |
| preflop AA,KK,QQ,AKs vs JJ,TT,AQs,KQs | 658M | — | 2054 ms | **137 ms** |
| flop, full range vs full range | 1.26B | — | 3907 ms | **254 ms** |
| river, full range vs full range | 1.07M | — | 10.2 ms | 2.8 ms (kernel 0.17) |

- Throughput: GPU ≈ **4.9 G runouts/s** (≈ 9.8 G hand evaluations/s) vs ≈ 310 M/s for
  30 CPU threads and 11.7 M/s for one → **~16x** the whole CPU, **~420x** one core.
- **Amdahl on the river case:** kernel 0.17 ms, total ~2.8 ms — most of the rest is the
  host building ~1M matchups (`prepare_workload`). A faster kernel can't help that case.

## 4. Why these choices (measured during development, alternatives removed)

- **Grid-stride over contiguous chunks per thread** (unrank once, then `next_combination`,
  like the CPU threads). Chunked was equal or slower (213 vs 137 ms on the 658M case):
  its `Card deck[48]` is runtime-indexed → per-thread stack in **local memory** (112 B vs
  48 B per thread, from `cuobjdump --dump-resource-usage`); it needed 64 registers vs 40
  → fewer resident threads (72 × 1024 vs 144 × 768); and threads in a warp hit
  `next_combination` rollovers / matchup boundaries at different times → divergence.
- **Block reduction over one `atomicAdd` per thread:** 0.31 vs 0.51 ms kernel on the
  1.7M case (110k threads contending on 3 addresses); no difference on the big jobs.
- **Pageable upload, not pinned memory:** pinned made the transfer itself faster (0.18
  vs 0.28 ms for 4.3 MB) but `cudaMallocHost` + the staging copy cost ~1.4 ms → slower
  per call. Pinned pays off only with large transfers and a buffer reused across calls.
- **Copy the matchup into a local** (`const Matchup mu = w.matchups[m];`): passing it by
  reference into global memory changed the codegen and cost ~10% on one kernel. Check
  `REG`/`STACK` with `cuobjdump --dump-resource-usage` after refactors.

## 5. Build notes

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
- CFR on the GPU (needs MCCFR-style sampled traversals; tree CFR doesn't map cleanly).
