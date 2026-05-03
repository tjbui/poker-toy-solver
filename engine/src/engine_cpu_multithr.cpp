/* engine_cpu_multithr.cpp - multithreaded equity simulator (CPU) */

#include "engine.h"
#include "simulation.h"
#include "evaluator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

struct ThreadCounts {
    int hero_wins = 0;
    int villain_wins = 0;
    int ties = 0;
    int simulated_hands = 0;
};

/*
 * Function that each worker thread will concurrently run
 */
static ThreadCounts run_exact_vs_exact_worker(
    Card hero1,
    Card hero2,
    Card villain1,
    Card villain2,
    const std::array<std::optional<Card>, 5>& known_community,
    const std::vector<Card>& deck,
    int simulations,
    unsigned int seed
) {
    ThreadCounts counts = {};

    std::mt19937 rng(seed);

    for (int sim = 0; sim < simulations; sim++) {
        std::array<Card, 5> final_community = fill_remaining_community(known_community, deck, rng);

        result_t result = evaluate_hands(
            hero1,
            hero2,
            villain1,
            villain2,
            final_community
        );

        if (result == HERO_WIN) {
            counts.hero_wins++;
        } else if (result == VILLAIN_WIN) {
            counts.villain_wins++;
        } else {
            counts.ties++;
        }

        counts.simulated_hands++;
    }

    return counts;
} /* run_exact_vs_exact_worker() */

/*
 * Multithreaded exact vs exact equity calculation.
 */
static EquityResult run_exact_vs_exact_multithr(const EquityRequest& request) {
    auto start = std::chrono::high_resolution_clock::now();

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

    unsigned int thread_count = std::thread::hardware_concurrency();

    if (thread_count == 0) {
        thread_count = 4;
    }

    if (request.simulations < static_cast<int>(thread_count)) {
        thread_count = request.simulations > 0 ? request.simulations : 1;
    }

    std::vector<std::thread> threads;
    std::vector<ThreadCounts> thread_results(thread_count);

    int base_sims_per_thread = request.simulations / thread_count;

    std::random_device rd;

    for (unsigned int t = 0; t < thread_count; t++) {
        int sims_for_this_thread = base_sims_per_thread;

        unsigned int seed = rd() + t; // +t makes each thread seed different

        threads.emplace_back(
            [&, t, sims_for_this_thread, seed]() {
                thread_results[t] = run_exact_vs_exact_worker(
                    hero1,
                    hero2,
                    villain1,
                    villain2,
                    known_community,
                    deck,
                    sims_for_this_thread,
                    seed
                );
            }
        );
    }

    for (auto& thread : threads) {
        thread.join();
    }

    int hero_wins = 0;
    int villain_wins = 0;
    int ties = 0;
    int simulated_hands = 0;

    for (const ThreadCounts& counts : thread_results) {
        hero_wins += counts.hero_wins;
        villain_wins += counts.villain_wins;
        ties += counts.ties;
        simulated_hands += counts.simulated_hands;
    }

    auto end = std::chrono::high_resolution_clock::now();

    EquityResult result{};
    result.simulatedHands = simulated_hands;
    result.runtimeMs =
        std::chrono::duration<double, std::milli>(end - start).count();

    if (simulated_hands > 0) {
        result.heroWinPct =
            100.0 * static_cast<double>(hero_wins) / simulated_hands;

        result.villainWinPct =
            100.0 * static_cast<double>(villain_wins) / simulated_hands;

        result.tiePct =
            100.0 * static_cast<double>(ties) / simulated_hands;
    }

    return result;
} /* run_exact_vs_exact_multithr() */

/*
 * Multithreaded equity API.
 */
EquityResult get_equity_multithr(const EquityRequest& request) {
    if (request.hero.mode == PlayerMode::Exact &&
        request.villain.mode == PlayerMode::Exact) {
        return run_exact_vs_exact_multithr(request);
    }

    throw std::runtime_error(
        "get_equity_multithr currently only supports exact vs exact"
    );
} /* get_equity_multithr() */