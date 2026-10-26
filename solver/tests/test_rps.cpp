/* test_rps.cpp -- the game-agnostic CFR core on Rock-Paper-Scissors */

#include "solver/cfr.h"
#include "solver/rps.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

using namespace solver;

static int g_failures = 0;

static void check(bool cond, const std::string& name) {
    std::cout << (cond ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (!cond) g_failures++;
}

static bool near_all(const std::vector<double>& got, const std::vector<double>& want, double tol) {
    for (std::size_t a = 0; a < want.size(); a++) {
        if (std::fabs(got[a] - want[a]) > tol) return false;
    }
    return true;
}

int main() {
    std::cout << "\n========== test_rps ==========\n\n";

    // Plain RPS: CFR starts at uniform, which is already the equilibrium.
    {
        CfrTrainer<RpsGame> trainer;
        trainer.train(1000);
        const std::vector<double> third = {1.0 / 3, 1.0 / 3, 1.0 / 3};
        check(trainer.infosets().size() == 2, "RPS: one infoset per player");
        check(near_all(trainer.average_strategy("P1"), third, 1e-9), "RPS: P1 = [1/3, 1/3, 1/3]");
        check(near_all(trainer.average_strategy("P2"), third, 1e-9), "RPS: P2 = [1/3, 1/3, 1/3]");
    }

    // Rock beats scissors for 2: the equilibrium moves to [1/4, 1/2, 1/4],
    // so CFR has to actually find it.
    {
        RpsGame game;
        game.rock_beats_scissors = 2.0;
        CfrTrainer<RpsGame> trainer(game);
        trainer.train(100000);
        const std::vector<double> want = {0.25, 0.5, 0.25};
        check(near_all(trainer.average_strategy("P1"), want, 0.01), "weighted RPS: P1 = [1/4, 1/2, 1/4]");
        check(near_all(trainer.average_strategy("P2"), want, 0.01), "weighted RPS: P2 = [1/4, 1/2, 1/4]");
        check(std::fabs(trainer.expected_value()) < 0.01, "weighted RPS: value 0 (symmetric game)");
    }

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n\n";
    return g_failures == 0 ? 0 : 1;
}
