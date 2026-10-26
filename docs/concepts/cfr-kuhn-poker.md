# CFR on Kuhn Poker

Builds on `cfr-rock-paper-scissors.md`. Code: `solver/include/solver/` (`cfr.h`, `kuhn.h`),
demo `examples/kuhn_cfr.cpp`, tests `solver/tests/`.

## The game

Deck J, Q, K; both ante 1; one card each. Actions are always pass (check / fold) or
bet (bet / call). Histories: `pp` showdown ±1, `bp` P1 wins 1, `bb` showdown ±2,
`pbp` P2 wins 1, `pbb` showdown ±2.

## Information sets

A player can't see the opponent's card, so nodes that differ only in that card are one
decision: an **infoset**, keyed by my card + the history (`"Qpb"`). Kuhn has 12:
P1 `J Q K Jpb Qpb Kpb`, P2 `Jp Jb Qp Qb Kp Kb`. Each holds `regretSum` and `strategySum`
exactly like an RPS player, and regret matching runs in each one.

## CFR = regret matching at every infoset, values from a tree walk

`cfr(state, reach)` returns the node's value to P1:
- terminal → payoff; chance → probability-weighted average over the 6 deals;
- decision → current strategy σ = regret matching of the infoset's `regretSum`;
  value of each action = recurse; node value = Σ σ[a]·v[a].
- Then, from the acting player's point of view (P2 flips the sign):
  - `regretSum[a]   += opponent reach × chance × (v[a] − v_node)` ← counterfactual
  - `strategySum[a] += own reach × σ[a]`
- σ is fixed for a whole iteration; after the tree walk every infoset gets its next σ.

Worked update (iteration 1, all uniform, deal Q vs K, P2's infoset `Kb`): fold → P1 +1,
call → P1 −2, node −0.5. P2's view: regrets fold −1.5, call +1.5. Opponent reach =
1/6 (deal) × 0.5 (P1 bets with Q) = 1/12 → `regretSum += [−0.125, +0.125]` → next
iteration `Kb` calls 100%.

## Design choices

- **Template `CfrTrainer<Game>`, not a virtual base class:** each game keeps its own
  small `State` value type, copied down the recursion with no heap allocation.
- **Vanilla CFR, all 6 deals every iteration** (not chance sampling): deterministic, so
  the tests are exact and repeatable. 100k iterations ≈ 0.2 s.
- **RPS through the same core** as a smoke test, plus weighted RPS (rock beats scissors
  for 2) whose equilibrium [1/4, 1/2, 1/4] is not where CFR starts.

## Result (100k iterations)

P1 value −0.05555 vs theory −1/18 = −0.05556. The equilibrium is a family with P1
bluff frequency α ∈ [0, 1/3]: CFR landed on α ≈ 0.205, with K bet 0.624 (theory 3α =
0.616) and Qpb call 0.541 (theory α + 1/3 = 0.539). P2's strategy is unique and matches:
K always bets/calls, Q calls 1/3 and checks behind, J folds and bluffs 1/3.

## Next ideas

- Exploitability (best response) as a convergence measure.
- CFR+ (faster convergence), Monte Carlo CFR (sampling; the version that maps to a GPU).
- A bigger game (Leduc hold'em).
