/* test_kuhn.cpp -- CFR on Kuhn poker must reach the known equilibrium */

#include "solver/cfr.h"
#include "solver/kuhn.h"

#include <cmath>
#include <iostream>
#include <string>

using namespace solver;

static int g_failures = 0;

static void check(bool cond, const std::string& name) {
    std::cout << (cond ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (!cond) g_failures++;
}

static bool near(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

int main() {
    std::cout << "\n========== test_kuhn ==========\n\n";

    // ---- the game itself ----
    KuhnGame g;
    KuhnGame::State s = g.initial();
    check(g.is_chance(s), "root is the deal");
    check(g.chance_outcomes(s).size() == 6, "6 deals");
    s.cards = {2, 0};                                       // K vs J
    check(g.infoset_key(s) == "K", "P1 infoset = own card");
    s = g.apply(s, KuhnGame::kPass);
    check(g.current_player(s) == 1 && g.infoset_key(s) == "Jp", "P2 infoset = own card + history");
    s = g.apply(s, KuhnGame::kBet);
    check(!g.is_terminal(s) && g.infoset_key(s) == "Kpb", "check-bet comes back to P1");
    check(g.is_terminal(g.apply(s, KuhnGame::kPass)) &&
              g.payoff(g.apply(s, KuhnGame::kPass)) == -1.0, "P1 folds: -1");
    check(g.payoff(g.apply(s, KuhnGame::kBet)) == 2.0, "P1 calls with K vs J: +2");

    // ---- CFR ----
    CfrTrainer<KuhnGame> trainer;
    trainer.train(100000);

    check(trainer.infosets().size() == 12, "12 infosets");

    const double value = trainer.expected_value();
    std::cout << "       P1 value " << value << " (theory " << -1.0 / 18.0 << ")\n";
    check(near(value, -1.0 / 18.0, 1e-3), "P1 value = -1/18");

    auto bet = [&](const std::string& key) { return trainer.average_strategy(key)[KuhnGame::kBet]; };
    const double tol = 0.02;

    // Player 2's equilibrium strategy is unique.
    check(bet("Kb") > 1.0 - tol, "P2 K: always calls a bet");
    check(bet("Kp") > 1.0 - tol, "P2 K: always bets after a check");
    check(near(bet("Qb"), 1.0 / 3.0, tol), "P2 Q: calls a bet 1/3");
    check(bet("Qp") < tol, "P2 Q: checks behind");
    check(bet("Jb") < tol, "P2 J: folds to a bet");
    check(near(bet("Jp"), 1.0 / 3.0, tol), "P2 J: bluffs 1/3 after a check");

    // Player 1's is a family with bluff frequency alpha in [0, 1/3].
    const double alpha = bet("J");
    std::cout << "       P1 alpha (J bet) " << alpha << "\n";
    check(alpha >= 0.0 && alpha <= 1.0 / 3.0 + tol, "P1 J: bluffs alpha in [0, 1/3]");
    check(near(bet("K"), 3.0 * alpha, 3.0 * tol), "P1 K: bets 3 * alpha");
    check(bet("Q") < tol, "P1 Q: never bets");
    check(bet("Jpb") < tol, "P1 J: folds to a bet");
    check(near(bet("Qpb"), alpha + 1.0 / 3.0, tol), "P1 Q: calls a bet alpha + 1/3");
    check(bet("Kpb") > 1.0 - tol, "P1 K: always calls a bet");

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n\n";
    return g_failures == 0 ? 0 : 1;
}
