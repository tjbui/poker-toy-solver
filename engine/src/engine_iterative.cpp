#include "engine.h"
#include "simulation.h"
#include "evaluator.h"

#include <chrono>
#include <random>
#include <vector>

using std::vector;

EquityResult run_equity_simulation_iterative(const EquityRequest& request) {
    int simulations = request.simulations > 0 ? request.simulations : 1000;

    int hero_wins = 0;
    int villain_wins = 0;
    int ties = 0;

    vector<int> hero_cards;
    vector<int> villain_cards;
    vector<int> community_cards;

    std::random_device rd;
    std::mt19937 rng(rd());

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < simulations; ++i) {
        fill_missing_cards(request, hero_cards, villain_cards, community_cards, rng);

        result_t outcome = evaluate_hand(
            hero_cards[0],
            hero_cards[1],
            villain_cards[0],
            villain_cards[1],
            community_cards
        );

        if (outcome == HERO_WIN) hero_wins++;
        else if (outcome == VILLAIN_WIN) villain_wins++;
        else ties++;
    }

    auto end = std::chrono::high_resolution_clock::now();

    EquityResult result{};
    result.heroWinPct = 100.0 * hero_wins / simulations;
    result.villainWinPct = 100.0 * villain_wins / simulations;
    result.tiePct = 100.0 * ties / simulations;
    result.simulatedHands = simulations;
    result.runtimeMs =
        std::chrono::duration<double, std::milli>(end - start).count();
    result.engine = request.engine;

    return result;
}