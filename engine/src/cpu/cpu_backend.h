#pragma once

/* cpu_backend.h -- CPU backends */

#include <cstdint>

#include "common/workload.h"

namespace equity {

// ---- the core: one exact matchup ----

// Number of runouts for a board with `known_count` cards (0, 3, 4 or 5):
// C(48 - known_count, 5 - known_count). Preflop 1,712,304; flop 990; turn 44; river 1.
std::uint64_t runouts_per_matchup(int known_count);

// One exact hero hand vs one exact villain hand on a known board: evaluate every
// runout and count hero wins / villain wins / ties.
EquityCounts cpu_count_matchup(const Matchup& mu, const Card* known, int known_count);

// Same, but only runouts [runout_from, runout_to) -- lets threads split one matchup.
EquityCounts cpu_count_matchup(const Matchup& mu, const Card* known, int known_count,
                               std::uint64_t runout_from, std::uint64_t runout_to);

// ---- wrappers: ranges (many matchups) ----

// Every matchup in the workload, on the calling thread.
EquityCounts cpu_count_single(const Workload& w);

// The same work split evenly across `threads` std::threads (0 = hardware_concurrency).
EquityCounts cpu_count_multi(const Workload& w, unsigned threads);

} // namespace equity
