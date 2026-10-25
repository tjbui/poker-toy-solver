# poker-toy-solver

A C++17 project that merges two components:

1. A poker equity engine
2. A toy poker solver built on CFR (Counterfactual Regret Minimization),
   starting from the classic Rock-Paper-Scissors example and building toward
   Kuhn poker.

## Layout

```
poker-equity/
├── docs/         # the "learning" half
│   ├── concepts/ # algorithm / domain ideas (language-agnostic)
│   └── notes/    # C++ and CUDA lessons learned in practice
├── engine/       # equity engine — headless library (+ tests)
├── solver/       # toy CFR solver — headless library (skeleton)
└── examples/     # terminal programs that USE the libraries
    ├── rps_cfr.cpp       # regret matching on Rock-Paper-Scissors
    ├── equity_cli.cpp    # terminal equity calculator (CPU or --gpu)
    └── equity_bench.cpp  # CPU vs GPU benchmark
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build       # run engine tests
```

## Run

```sh
./build/rps_cfr                          # RPS self-play; average strategy -> [.33, .33, .33]
./build/equity_cli AhKd 2c2d 7h8s9c      # exact equity (CPU, multithreaded)
./build/equity_cli --gpu "AA,KK" AKs     # same on the GPU (CUDA builds only)
./build/equity_bench                     # CPU vs GPU benchmark
```

The CUDA backend builds automatically when CMake finds `nvcc`; otherwise the
project builds CPU-only. Results and design notes: `docs/notes/cuda-notes.md`.


## Reference

- Neller & Lanctot, *An Introduction to Counterfactual Regret Minimization* (2013).
