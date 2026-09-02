/* engine_cpu_iterative.cpp -- exact equity by enumeration, single-threaded */

#include "engine.h"
#include "simulation.h"

#include <array>
#include <chrono>
#include <cstdint>
#include <optional>

/*
 * Enumerate every (matchup x board runout) exactly, on one thread.
 * Works for exact-vs-exact, exact-vs-range, and range-vs-range: the mode
 * only changes how many matchups prepare_matchups() produces.
 */
EquityResult get_equity_iterative(const EquityRequest& request) {
    auto start = std::chrono::high_resolution_clock::now();

    std::array<std::optional<Card>, 5> known_community;
    std::vector<Matchup> matchups = prepare_matchups(request, known_community);

    std::uint64_t total = total_runouts(matchups, known_community);
    EquityCounts counts = count_slice(matchups, known_community, 0, total);

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return finalize_result(counts, ms);
} /* get_equity_iterative() */
