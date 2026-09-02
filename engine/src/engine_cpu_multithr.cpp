/* engine_cpu_multithr.cpp -- exact equity by enumeration, multi-threaded */

#include "engine.h"
#include "simulation.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <thread>
#include <vector>

/*
 * Same exact enumeration as the iterative version, but the flat work space
 * [0, total) is sliced into one contiguous range per thread. Each slice is
 * independent (no shared mutable state), so the results are summed at the end
 * and match the single-threaded numbers bit-for-bit.
 */
EquityResult get_equity_multithr(const EquityRequest& request) {
    auto start = std::chrono::high_resolution_clock::now();

    std::array<std::optional<Card>, 5> known_community;
    std::vector<Matchup> matchups = prepare_matchups(request, known_community);

    std::uint64_t total = total_runouts(matchups, known_community);

    unsigned int thread_count = std::thread::hardware_concurrency();
    if (thread_count == 0) {
        thread_count = 4;
    }
    if (total < thread_count) {
        thread_count = static_cast<unsigned int>(std::max<std::uint64_t>(1, total));
    }

    std::vector<std::thread> threads;
    std::vector<EquityCounts> partial(thread_count);

    std::uint64_t base = total / thread_count;
    std::uint64_t remainder = total % thread_count;

    std::uint64_t lo = 0;
    for (unsigned int t = 0; t < thread_count; t++) {
        std::uint64_t hi = lo + base + (t < remainder ? 1 : 0);
        threads.emplace_back([&, t, lo, hi]() {
            partial[t] = count_slice(matchups, known_community, lo, hi);
        });
        lo = hi;
    }
    for (std::thread& th : threads) {
        th.join();
    }

    EquityCounts counts;
    for (const EquityCounts& p : partial) {
        counts.hero_wins += p.hero_wins;
        counts.villain_wins += p.villain_wins;
        counts.ties += p.ties;
        counts.runouts += p.runouts;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    return finalize_result(counts, ms);
} /* get_equity_multithr() */
