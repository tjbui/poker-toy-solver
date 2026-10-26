#pragma once

/*
 * runouts.cuh -- ENUMERATE: which cards does runout number r deal?
 *
 * Same numbering as the CPU (combinatorics.h): runout r of a matchup is the
 * r-th lexicographic k-subset of the cards that matchup and the known board
 * leave unseen. unrank_cards() turns r straight into board cards.
 *
 * The __constant__ tables below get one copy per .cu file that includes this
 * header, so only gpu_backend.cu includes it (via kernels.cuh).
 */

#include <cstdint>

#include "common/combinatorics.h"
#include "common/workload.h"
#include "cuda_utils.cuh"

namespace equity::gpu {

using ull = unsigned long long;

constexpr int kMaxN = 52;
constexpr int kMaxK = 5;
constexpr ull kDeckMask = (1ULL << 52) - 1;   // bit c set = card c

// C(n, k) lookup for unranking (faster than computing it), and the known board.
// Constant memory is cached and broadcast: fastest when a warp reads one address.
__constant__ ull c_choose[kMaxN + 1][kMaxK + 1];
__constant__ Card c_known[5];

// Host side: fill the constant tables before any kernel launch.
inline void upload_constants(const Card* known) {
    ull choose[kMaxN + 1][kMaxK + 1];
    for (int n = 0; n <= kMaxN; n++) {
        for (int k = 0; k <= kMaxK; k++) choose[n][k] = n_choose_k(n, k);
    }
    CUDA_CHECK(cudaMemcpyToSymbol(c_choose, choose, sizeof(choose)));
    CUDA_CHECK(cudaMemcpyToSymbol(c_known, known, sizeof(Card) * 5));
}

// Copy the known cards into the start of board[]; return them as a card mask.
__device__ inline ull load_known_board(int known_count, Card* board) {
    ull known_mask = 0;
    for (int j = 0; j < known_count; j++) {
        board[j] = c_known[j];
        known_mask |= 1ULL << c_known[j];
    }
    return known_mask;
}

// The matchup's four hole cards as a card mask.
__device__ inline ull matchup_mask(const Matchup& mu) {
    return (1ULL << mu.h1) | (1ULL << mu.h2) | (1ULL << mu.v1) | (1ULL << mu.v2);
}

/*
 * Write the r-th lexicographic k-subset of the cards NOT in `used` straight into
 * out[]. Walks the unseen cards lowest-first with find-first-set instead of
 * building a deck array (a runtime-indexed array would live in slow local memory).
 */
__device__ inline void unrank_cards(ull r, ull used, int n, int k, Card* out) {
    ull avail = ~used & kDeckMask;
    int pos = 0;                                 // position among unseen cards
    for (int i = 0; i < k; i++) {
        while (true) {
            int c = __ffsll(static_cast<long long>(avail)) - 1;
            avail &= avail - 1;                  // consume the lowest unseen card
            ull cnt = c_choose[n - 1 - pos][k - 1 - i];
            pos++;
            if (r < cnt) {
                out[i] = static_cast<Card>(c);
                break;
            }
            r -= cnt;
        }
    }
}

} // namespace equity::gpu
