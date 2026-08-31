# poker-equity

A single C++17 project that is both a **learning artifact** and a **working
terminal tool**. It merges two things:

1. A **poker equity engine** — computes hand equity on CPU (and later CUDA).
2. A **toy poker solver** built on **CFR** (Counterfactual Regret Minimization),
   starting from the classic Rock-Paper-Scissors example and building toward
   Kuhn poker.

There is **no web app**. Everything runs in the **terminal**: the libraries are
headless, and small demo programs in `examples/` link them.

## Layout

```
poker-equity/
├── docs/         # the "learning" half
│   ├── concepts/ # algorithm / domain ideas (language-agnostic)
│   └── notes/    # C++ and CUDA lessons learned in practice
├── engine/       # equity engine — headless library (+ tests)
├── solver/       # toy CFR solver — headless library (skeleton)
└── examples/     # terminal programs that USE the libraries
    ├── rps_cfr.cpp     # regret matching on Rock-Paper-Scissors
    └── equity_cli.cpp  # terminal equity calc (skeleton)
```

## Build

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build       # run engine tests
```

## Run

```sh
./build/rps_cfr              # RPS self-play; average strategy -> [.33, .33, .33]
./build/equity_cli           # terminal equity calculator (WIP)
```

## Roadmap

0. Learn the algorithm: regret matching (RPS) → CFR on Kuhn poker.  **← here**
1. CFR core as a `solver/` library; verify RPS self-play → `[.33, .33, .33]`.
2. CFR on Kuhn poker; verify game value ≈ -1/18 for P1.
3. Rework the equity engine: exact runout **enumeration** instead of Monte Carlo.
4. Fold engine + solver into a coherent terminal tool.
5. CUDA: port the hot paths to a kernel, benchmark CPU vs GPU.

## Reference

- Neller & Lanctot, *An Introduction to Counterfactual Regret Minimization* (2013).
