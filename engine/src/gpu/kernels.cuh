#pragma once

/*
 * kernels.cuh -- SCHEDULE: which (matchup, runout) indices does each thread do?
 *
 * The work space is every (matchup, runout) pair, numbered
 *   index = matchup * runouts_per_matchup + runout,   index in [0, total).
 * All matchups go in ONE launch (one launch per matchup would mean millions of
 * launches for wide ranges). The kernel only hands out indices; the per-runout
 * work is matchup.cuh and the final sum is reduce.cuh.
 */

#include "matchup.cuh"
#include "reduce.cuh"

namespace equity::gpu {

// Kernel arguments (passed by value).
struct Work {
    const Matchup* matchups;  // device memory
    ull total;                // matchups * runouts_per_matchup
    ull runouts_per_matchup;
    int known_count;          // known board cards
    int k;                    // board cards still to deal
    ull* counts;              // device memory: [hero_wins, villain_wins, ties]
};

/*
 * Grid-stride loop: thread t does indices t, t + stride, t + 2*stride, ...
 * (stride = threads in the whole grid). The 32 threads of a warp take 32
 * consecutive indices -> same matchup, adjacent runouts -> nearly identical
 * work, which is what lockstep (SIMT) execution wants.
 */
__global__ void kernel_equity(Work w) {
    Card board[5];
    const ull known_mask = load_known_board(w.known_count, board);

    DeviceCounts counts;
    const ull stride = static_cast<ull>(gridDim.x) * blockDim.x;
    for (ull i = static_cast<ull>(blockIdx.x) * blockDim.x + threadIdx.x; i < w.total; i += stride) {
        const ull m = i / w.runouts_per_matchup;              // which matchup
        const ull runout = i - m * w.runouts_per_matchup;     // which runout of it
        const Matchup mu = w.matchups[m];                     // copy: one global read, then registers
        counts.add(play_runout(mu, runout, known_mask, w.known_count, w.k, board));
    }

    add_counts(counts, w.counts);                // every thread reaches this
}

} // namespace equity::gpu
