# poker-toy-solver

A C++17 project that merges two components:

1. A poker equity engine — exact equity by enumerating every board runout,
   with interchangeable CPU and CUDA GPU backends that return identical results
2. A toy poker solver built on CFR (Counterfactual Regret Minimization),
   written once against an abstract game and verified on Rock-Paper-Scissors
   and Kuhn poker (reaches the known equilibrium, game value -1/18).

## Layout

```
poker-equity/
├── docs/         # the "learning" half
│   ├── concepts/ # algorithm / domain ideas (language-agnostic)
│   └── notes/    # C++ and CUDA lessons learned in practice
├── engine/       # equity engine — headless library
│   ├── include/equity/equity.h   # public API (the only header callers use)
│   ├── src/common/   # shared by all backends: workload prep, HD evaluator, combinatorics
│   ├── src/cpu/      # single-thread + multithreaded backends
│   ├── src/gpu/      # CUDA backend: runouts / matchup core / reduce / kernel (.cuh)
│   │                 #   + gpu_backend.cu host side; stub when built without nvcc
│   ├── src/equity.cpp  # compute_equity(): picks the backend
│   └── tests/
├── solver/       # toy CFR solver — header-only library
│   ├── include/solver/cfr.h    # CfrTrainer<Game>: vanilla CFR written once for any game
│   ├── include/solver/kuhn.h   # Kuhn poker;  rps.h: Rock-Paper-Scissors
│   └── tests/                  # Kuhn reaches -1/18, (weighted) RPS reaches equilibrium
└── examples/     # terminal programs that USE the libraries
    ├── rps_cfr.cpp       # regret matching on Rock-Paper-Scissors (standalone)
    ├── kuhn_cfr.cpp      # Kuhn poker solved with the CFR core
    ├── equity_cli.cpp    # terminal equity calculator
    └── equity_bench.cpp  # CPU vs GPU benchmark
```

## Using the engine

```cpp
#include "equity/equity.h"

equity::EquityRequest req;
req.hero.mode = equity::PlayerMode::Range;
req.hero.range = {"AA", "KK", "AKs"};
req.villain.cards = {std::string("Qs"), std::string("Qd")};   // exact hand
req.community = {std::string("2c"), std::string("7d"), std::string("9s"), std::nullopt, std::nullopt};

equity::EquityOptions opt;            // backend = Auto: GPU for big jobs if available
// opt.backend = equity::Backend::CpuMulti;
equity::EquityResult r = equity::compute_equity(req, opt);
// r.heroWinPct, r.villainWinPct, r.tiePct, r.evaluatedRunouts, r.backend
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build       # engine + solver tests (+ GPU-vs-CPU exactness test on CUDA builds)
```

The CUDA backend builds automatically when CMake finds `nvcc`; otherwise the
project builds CPU-only (`gpu_available()` is false, `Backend::Gpu` throws).

## Run

```sh
./build/rps_cfr                                  # RPS self-play; average strategy -> [.33, .33, .33]
./build/kuhn_cfr                                 # Kuhn poker: 12-infoset strategy, P1 value -> -1/18
./build/equity_cli AhKd 2c2d 7h8s9c              # exact equity, Auto backend
./build/equity_cli --backend cpu-multi AsAh KsKd # force a backend: auto|cpu-single|cpu-multi|gpu
./build/equity_cli --gpu "AA,KK" AKs             # shorthand for --backend gpu (CUDA builds only)
./build/equity_bench                             # CPU vs GPU benchmark
```

Results and design notes: `docs/notes/cuda-notes.md` (GPU engine),
`docs/concepts/cfr-kuhn-poker.md` (solver).

## Reference

- Neller & Lanctot, *An Introduction to Counterfactual Regret Minimization* (2013).
