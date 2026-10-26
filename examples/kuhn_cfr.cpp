/*
 * kuhn_cfr.cpp -- solve Kuhn poker with CFR and print the strategy.
 *
 *   ./build/kuhn_cfr            100,000 iterations
 *   ./build/kuhn_cfr 1000       fewer: watch how far from equilibrium it still is
 */

#include "solver/cfr.h"
#include "solver/kuhn.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace solver;

namespace {

// Infosets for one player, sorted: player 0 acts at even history lengths.
std::vector<std::string> infosets_of(const CfrTrainer<KuhnGame>& trainer, int player) {
    std::vector<std::string> keys;
    for (const auto& [key, node] : trainer.infosets()) {
        const int history_len = static_cast<int>(key.size()) - 1;
        if (history_len % 2 == player) keys.push_back(key);
    }
    std::sort(keys.begin(), keys.end(), [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return a.size() < b.size();   // shorter history first
        return std::string("JQK").find(a[0]) < std::string("JQK").find(b[0]) ||
               (a[0] == b[0] && a < b);
    });
    return keys;
}

void print_player(const CfrTrainer<KuhnGame>& trainer, int player) {
    std::printf("\nPlayer %d          pass     bet\n", player + 1);
    for (const std::string& key : infosets_of(trainer, player)) {
        const std::vector<double> s = trainer.average_strategy(key);
        std::printf("  %-6s        %6.3f  %6.3f\n", key.c_str(), s[KuhnGame::kPass], s[KuhnGame::kBet]);
    }
}

} // namespace

int main(int argc, char** argv) {
    const int iterations = argc > 1 ? std::atoi(argv[1]) : 100000;

    CfrTrainer<KuhnGame> trainer;
    const auto start = std::chrono::steady_clock::now();
    trainer.train(iterations);
    const auto end = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::printf("Kuhn poker, vanilla CFR, %d iterations (%.0f ms)\n", iterations, ms);
    std::printf("Infoset = my card + history (p = pass, b = bet)\n");
    print_player(trainer, 0);
    print_player(trainer, 1);

    const double alpha = trainer.average_strategy("J")[KuhnGame::kBet];
    std::printf("\nP1 value: %.5f   (theory -1/18 = %.5f)\n", trainer.expected_value(), -1.0 / 18.0);
    std::printf("P1 bluff alpha = J bet = %.3f   ->  theory: K bet = 3*alpha = %.3f, "
                "Qpb call = alpha + 1/3 = %.3f\n",
                alpha, 3.0 * alpha, alpha + 1.0 / 3.0);
    return 0;
}
