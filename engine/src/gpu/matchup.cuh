#pragma once

/*
 * matchup.cuh -- the GPU core: one runout of one exact matchup.
 *
 * Runout cards come from runouts.cuh, the showdown from the evaluator shared
 * with the CPU (hand_eval.h), and the result goes into this thread's own
 * counters. Nothing here knows about threads, blocks or ranges -- that's
 * kernels.cuh.
 */

#include "common/hand_eval.h"
#include "runouts.cuh"

namespace equity::gpu {

// One thread's private win/loss/tie counters (registers, never shared).
struct DeviceCounts {
    ull hero_wins = 0;
    ull villain_wins = 0;
    ull ties = 0;

    // Add one showdown result (+1 hero, -1 villain, 0 tie). Branch-free.
    __device__ void add(int result) {
        hero_wins += (result > 0);
        villain_wins += (result < 0);
        ties += (result == 0);
    }
};

/*
 * Showdown of runout `runout` of matchup `mu`. board[0..known_count) already
 * holds the known cards; the runout's cards are written after them.
 * Returns +1 hero, -1 villain, 0 tie.
 */
__device__ inline int play_runout(const Matchup& mu, ull runout, ull known_mask,
                                  int known_count, int k, Card* board) {
    const int n = 48 - known_count;             // unseen cards
    unrank_cards(runout, known_mask | matchup_mask(mu), n, k, board + known_count);
    return hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board);
}

} // namespace equity::gpu
