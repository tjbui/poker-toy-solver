#pragma once

/*
 * matchup.cuh -- the GPU core: one exact matchup, one runout or a slice of them.
 *
 * Same job as the CPU core cpu_count_matchup(): runout cards come from
 * runouts.cuh, the showdown from the shared evaluator (hand_eval.h), and the
 * result goes into this thread's own counters. Nothing here knows about
 * threads, blocks or ranges -- that's kernels.cuh.
 */

#include "common/combinatorics.h"
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
 * Returns +1 hero, -1 villain, 0 tie. (Grid-stride: one call per index.)
 */
__device__ inline int play_runout(const Matchup& mu, ull runout, ull known_mask,
                                  int known_count, int k, Card* board) {
    const int n = 48 - known_count;             // unseen cards
    unrank_cards(runout, known_mask | matchup_mask(mu), n, k, board + known_count);
    return hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board);
}

/*
 * Runouts [runout_from, runout_to) of matchup `mu`, added into `counts`.
 * Builds the unseen deck once, unranks the first runout, then steps with
 * next_combination() -- the GPU twin of cpu_count_matchup(). (Chunked.)
 */
__device__ inline void count_matchup(const Matchup& mu, ull known_mask, int known_count, int k,
                                     ull runout_from, ull runout_to, Card* board,
                                     DeviceCounts& counts) {
    Card deck[48];                               // runtime-indexed -> local memory
    int deck_size = 0;
    ull avail = ~(known_mask | matchup_mask(mu)) & kDeckMask;
    while (avail) {
        deck[deck_size++] = static_cast<Card>(__ffsll(static_cast<long long>(avail)) - 1);
        avail &= avail - 1;
    }

    int runout_positions[5];
    unrank_positions(runout_from, deck_size, k, runout_positions);
    for (ull runout = runout_from; runout < runout_to; runout++) {
        for (int j = 0; j < k; j++) board[known_count + j] = deck[runout_positions[j]];
        counts.add(hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board));
        next_combination(runout_positions, deck_size, k);
    }
}

} // namespace equity::gpu
