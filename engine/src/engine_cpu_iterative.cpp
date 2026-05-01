/* engine_cpu_iterative.cpp - iterative equity calculator (no multithreading) */

#include "engine.h"
#include "simulation.h"
#include "evaluator.h"
#include <chrono>
#include <random>
#include <stdexcept>
#include <algorithm>

static EquityResult run_exact_vs_exact(const EquityRequest& request);
static EquityResult run_exact_vs_range_internal(
    const PlayerRequest& exact_player,
    const PlayerRequest& range_player,
    const std::array<std::optional<std::string>, 5>& community,
    int simulations,
    bool exact_player_is_hero
);
static EquityResult run_range_vs_range(const EquityRequest& request);

/*
 * Iteratively runs simulations to calculate equity given an EquityRequest.
 * Parses EquityRequest mode to simulate exact vs exact, exact vs range,
 * range vs exact, or range vs range.
 */
EquityResult get_equity_iterative(const EquityRequest& request) {
    if (request.hero.mode == PlayerMode::Exact &&
        request.villain.mode == PlayerMode::Exact) {
        return run_exact_vs_exact(request);
    }

    if (request.hero.mode == PlayerMode::Exact &&
        request.villain.mode == PlayerMode::Range) {
        return run_exact_vs_range_internal(
            request.hero,
            request.villain,
            request.community,
            request.simulations,
            true
        );
    }

    if (request.hero.mode == PlayerMode::Range &&
        request.villain.mode == PlayerMode::Exact) {
        return run_exact_vs_range_internal(
            request.villain,
            request.hero,
            request.community,
            request.simulations,
            false
        );
    }

    return run_range_vs_range(request);
} /* get_equity_iterative() */

/*
 * Iteratively run out remaing boards using exact hero and villain hands
 */
static EquityResult run_exact_vs_exact(const EquityRequest& request) {
    auto start = std::chrono::high_resolution_clock::now();

    int hero_wins = 0;
    int villain_wins = 0;
    int ties = 0;

    if (!request.hero.cards[0] || !request.hero.cards[1]) {
        throw std::invalid_argument("Hero exact hand requires 2 cards");
    }

    if (!request.villain.cards[0] || !request.villain.cards[1]) {
        throw std::invalid_argument("Villain exact hand requires 2 cards");
    }

    Card hero1 = parse_card(*request.hero.cards[0]);
    Card hero2 = parse_card(*request.hero.cards[1]);
    Card villain1 = parse_card(*request.villain.cards[0]);
    Card villain2 = parse_card(*request.villain.cards[1]);

    auto known_community = parse_community(request.community);

    std::vector<Card> deck = make_deck();
    remove_known_cards(deck, hero1, hero2, villain1, villain2, known_community);

    std::random_device rd;
    std::mt19937 rng(rd());

    for (int sim = 0; sim < request.simulations; sim++) {
        std::array<Card, 5> final_community =
            fill_remaining_community(known_community, deck, rng);

        result_t result = evaluate_hands(hero1, hero2, villain1, villain2, final_community);

        if (result == HERO_WIN) {
            hero_wins++;
        } else if (result == VILLAIN_WIN) {
            villain_wins++;
        } else {
            ties++;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();

    EquityResult result{};
    result.simulatedHands = request.simulations;
    result.runtimeMs = std::chrono::duration<double, std::milli>(end - start).count();

    if (request.simulations > 0) {
        result.heroWinPct = 100.0 * hero_wins / request.simulations;
        result.villainWinPct = 100.0 * villain_wins / request.simulations;
        result.tiePct = 100.0 * ties / request.simulations;
    }

    return result;
} /* run_exact_vs_exact() */

/*
 * Iteratively run out remaing boards with range vs exact hands
 */
static EquityResult run_exact_vs_range_internal(
    const PlayerRequest& exact_player,
    const PlayerRequest& range_player,
    const std::array<std::optional<std::string>, 5>& community,
    int simulations,
    bool exact_player_is_hero
) {
    EquityResult result{};

    // TODO:
    // exact_player_is_hero tells you whether wins for exact_player
    // should increment heroWins or villainWins.

    return result;
} /* run_exact_vs_range_internal() */

/*
 * Iteratively run out remaing boards with range vs range
 */
static EquityResult run_range_vs_range(const EquityRequest& request) {
    EquityResult result{};

    // TODO:
    // Sample one hand from hero range
    // Sample one hand from villain range
    // Complete board
    // Evaluate

    return result;
} /* run_range_vs_range() */