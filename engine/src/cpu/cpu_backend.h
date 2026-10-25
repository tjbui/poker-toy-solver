#pragma once

/* cpu_backend.h -- CPU backends: count results over the work space on the host */

#include <cstdint>

#include "common/workload.h"

namespace equity {

// Count results for the slice [lo, hi) of the work space on the calling thread.
EquityCounts cpu_count_range(const Workload& w, std::uint64_t lo, std::uint64_t hi);

// Whole work space on one thread (the reference implementation).
EquityCounts cpu_count_single(const Workload& w);

// Whole work space sliced across `threads` std::threads (0 = hardware_concurrency).
EquityCounts cpu_count_multi(const Workload& w, unsigned threads);

} // namespace equity
