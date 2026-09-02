# CUDA Notes

Things learned while accelerating the hot loops (equity enumeration now,
CFR/MCCFR later). Kernels, memory model, launch configuration, benchmarks.

## Plan: sharing the evaluator between CPU and GPU (Phase 5)

**Question:** can a CUDA kernel call the functions in `evaluator.cpp`?

- **Host code inside a `.cu` file** *can* call `evaluator.cpp` functions — nvcc
  compiles the host parts and links them with the g++-built objects like normal.
- **A GPU kernel (`__global__` / `__device__` code) cannot** call plain host
  `.cpp` functions. Device code can only call functions marked `__device__`
  (or `__host__ __device__`).

So to evaluate hands *inside* a kernel, there are two options:

- **(A) One shared source, marked `__host__ __device__`** in a header, compiled
  by both g++ (CPU) and nvcc (GPU). Single source of truth, no duplication.
  This is the standard "write once, run on both" pattern — recommended.
- **(B) A separate device-only port** of the evaluator. Two copies that drift.

**Chosen approach: (A), staged.** The catch is that device code can't use
`std::vector`, `<algorithm>` (`std::sort`), or exceptions — and the current
`evaluate_7_cards` uses `std::array<std::vector<uint8_t>, 4>`, `std::sort`, and
`make_score(std::vector<int>)`. To become `__host__ __device__`, the hot path
must be rewritten **allocation-free**: fixed-size arrays, manual insertion sort
of ≤7 ranks, no exceptions in the inner loop.

That rewrite *also* speeds up the CPU path (no per-evaluation heap allocation —
which is why preflop enumeration currently takes seconds), so it's worth doing
independently of the GPU. Sequence:

1. Refactor `evaluate_7_cards` + the `try_*` helpers into a header-only,
   array-only form; mark them `__host__ __device__ inline`. Keep the CPU tests
   green (numbers must not change).
2. Kernel: one thread per runout. Flatten the `(matchup, board)` work space
   (the same flat index space `count_slice` already uses) into a 1-D grid,
   evaluate, and reduce win/tie/loss counts (atomics or a per-block reduction).
3. Benchmark CPU-multithread vs GPU on the preflop C(48,5) = 1.7M case.

No local NVIDIA GPU — build/run on Colab / Kaggle / rented (vast.ai, runpod).

<!-- Add kernel-specific lessons (occupancy, coalescing, launch config) here. -->
