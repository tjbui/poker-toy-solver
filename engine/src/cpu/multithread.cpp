/* multithread.cpp -- split the work across std::threads */

#include "cpu_backend.h"

#include <algorithm>
#include <thread>
#include <vector>

namespace equity {

namespace {

/*
 * Why not just give each thread whole matchups? Exact vs exact is ONE matchup,
 * which would leave every thread but one idle. So instead, all the work is
 * numbered as one long list -- matchup 0's runouts, then matchup 1's, ... --
 *
 *   index:    0 .. R-1  | R .. 2R-1 | 2R .. 3R-1 | ...     (R = runouts per matchup)
 *   matchup:  0         | 1         | 2          | ...
 *
 * so index = matchup * R + runout, and the list is cut into equal slices.
 * A slice can start partway through one matchup and end partway through
 * another, so this walks the matchups it touches and asks the core for just
 * the runouts of each that fall inside the slice.
 */
EquityCounts count_slice(const Workload& w, std::uint64_t first, std::uint64_t last) {
    EquityCounts counts;
    const std::uint64_t R = w.runoutsPerMatchup;
    if (first >= last || R == 0) return counts;

    // Which matchups does [first, last) touch? (last is exclusive, so use last - 1.)
    const std::uint64_t first_matchup = first / R;
    const std::uint64_t last_matchup = (last - 1) / R;

    for (std::uint64_t m = first_matchup; m <= last_matchup; m++) {
        // Matchup m owns indices [start, start + R). Clip that to the slice,
        // then subtract start to turn indices back into runout numbers.
        const std::uint64_t start = m * R;
        const std::uint64_t runout_from = std::max(first, start) - start;
        const std::uint64_t runout_to = std::min(last, start + R) - start;

        counts += cpu_count_matchup(w.matchups[m], w.known.data(), w.knownCount,
                                    runout_from, runout_to);
    }
    return counts;
}

} // namespace

/*
 * One slice per thread. The slices share no mutable state, so each thread
 * counts into its own EquityCounts and the partial counts are summed after
 * join() -- identical to the single-thread result.
 */
EquityCounts cpu_count_multi(const Workload& w, unsigned threads) {
    if (threads == 0) {
        threads = std::thread::hardware_concurrency();
    }
    if (threads == 0) {
        threads = 4;
    }
    if (w.total < threads) {
        threads = static_cast<unsigned>(std::max<std::uint64_t>(1, w.total));
    }

    std::vector<EquityCounts> partial(threads);
    std::vector<std::thread> pool;
    pool.reserve(threads);

    // Every thread gets total / threads indices; the first (total % threads)
    // threads take one extra so the slices cover [0, total) exactly.
    const std::uint64_t base = w.total / threads;
    const std::uint64_t remainder = w.total % threads;

    std::uint64_t first = 0;
    for (unsigned t = 0; t < threads; t++) {
        const std::uint64_t last = first + base + (t < remainder ? 1 : 0);
        pool.emplace_back([&w, &partial, t, first, last]() {
            partial[t] = count_slice(w, first, last);
        });
        first = last;
    }
    for (std::thread& th : pool) {
        th.join();
    }

    EquityCounts counts;
    for (const EquityCounts& p : partial) {
        counts += p;
    }
    return counts;
}

} // namespace equity
