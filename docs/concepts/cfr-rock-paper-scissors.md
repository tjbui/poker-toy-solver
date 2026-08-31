# CFR: Regret Matching on Rock-Paper-Scissors

Language-agnostic notes on the first building block of CFR — **regret matching**
— worked through the classic Rock-Paper-Scissors (RPS) example.

> Mirrors / summarizes the longer Obsidian notes at
> `Poker Toy Solver/Counterfactual Regret Minimization.md` so the repo is
> self-contained. Fill this in as you learn.

## The idea (TODO)

- What "regret" means for one action on one iteration.
- Why we accumulate a **regret sum** and derive the current strategy from it.
- Why the **average** strategy (not the current one) converges to Nash.

## Worked example (TODO)

- Two players, seeded pure strategies, watch current bounce while the average
  slides to `[.33, .33, .33]`.

## Code

- Reference implementation: `examples/rps_cfr.cpp`.

## Reference

- Neller & Lanctot, *An Introduction to Counterfactual Regret Minimization* (2013).
