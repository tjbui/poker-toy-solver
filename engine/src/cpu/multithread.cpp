/* multithread.cpp -- CPU enumeration sliced across std::threads */

#include "cpu_backend.h"

#include <algorithm>
#include <thread>
#include <vector>

namespace equity {

/*
 * Split [0, total) into one contiguous slice per thread. The slices share no
 * mutable state, so each thread counts into its own EquityCounts and the
 * partial counts are summed after join() -- identical to the single-thread
 * result.
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

    const std::uint64_t base = w.total / threads;
    const std::uint64_t remainder = w.total % threads;

    std::uint64_t lo = 0;
    for (unsigned t = 0; t < threads; t++) {
        const std::uint64_t hi = lo + base + (t < remainder ? 1 : 0);
        pool.emplace_back([&w, &partial, t, lo, hi]() {
            partial[t] = cpu_count_range(w, lo, hi);
        });
        lo = hi;
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
