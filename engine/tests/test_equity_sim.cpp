/* test_simulator.cpp - test module for iterative engine */

#include "engine.h"

#include <iostream>
#include <optional>
#include <string>

static void print_equity_result(const EquityResult& result) {
    std::cout << "Hero Win %:    " << result.heroWinPct << "\n";
    std::cout << "Villain Win %: " << result.villainWinPct << "\n";
    std::cout << "Tie %:         " << result.tiePct << "\n";
    std::cout << "Runtime ms:    " << result.runtimeMs << "\n";
    std::cout << "Simulations:   " << result.simulatedHands << "\n";
}

int main() {
    std::cout << "\n========== START test_simulator.cpp ==========\n\n";

    EquityRequest request{};

    request.hero.mode = PlayerMode::Exact;
    request.hero.cards = {std::optional<std::string>("AH"),
                          std::optional<std::string>("KD")};

    request.villain.mode = PlayerMode::Exact;
    request.villain.cards = {std::optional<std::string>("2H"),
                             std::optional<std::string>("2D")};

    request.community = {
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        std::nullopt
    };

    request.simulations = 100000;

    try {
        std::cout << "\n--- Iterative ---\n";
        EquityResult result = get_equity_iterative(request);
        print_equity_result(result);

        std::cout << "\n--- Multithreaded ---\n";
        result = get_equity_multithr(request);
        print_equity_result(result);
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
    }


    std::cout << "\n========== END test_simulator.cpp ==========\n";

    return 0;
}