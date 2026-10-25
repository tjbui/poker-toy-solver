#pragma once

/*
 * hand_eval.h -- allocation-free 7-card hand evaluator shared by CPU and GPU.
 *
 * Header-only and marked HD (__host__ __device__ under nvcc), so the exact same
 * code runs in the CPU backends and inside the CUDA kernels. No std::vector, no
 * <algorithm>, no exceptions: just fixed arrays and bitmasks. This is the
 * reference evaluator -- a backend-specific evaluator is only worth adding if a
 * benchmark shows it is faster, and it must still match this one exactly.
 *
 * Card encoding: card = suit * 13 + rank
 *   suit: H=0, D=1, C=2, S=3        rank: 2=0, 3=1, ..., K=11, A=12
 *
 * Score: (category << 20) | five 4-bit ranks, most significant first.
 * Only comparisons between scores are meaningful (bigger = stronger).
 */

#include <cstdint>

#include "hd.h"

namespace equity {

using Card = std::uint8_t;

namespace hand_eval {

enum Category : std::uint32_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8
};

HD inline int popcount(std::uint32_t x) {
#ifdef __CUDA_ARCH__
    return __popc(x);
#else
    return __builtin_popcount(x);
#endif
}

// Index of the highest set bit. x must be non-zero.
HD inline int high_bit(std::uint32_t x) {
#ifdef __CUDA_ARCH__
    return 31 - __clz(x);
#else
    return 31 - __builtin_clz(x);
#endif
}

/*
 * Highest rank of a straight in a 13-bit rank mask, or -1 if none.
 * Shift up by one and copy the ace into bit 0 so A-2-3-4-5 is handled;
 * a bit survives the AND of five shifts only if it starts 5 in a row.
 */
HD inline int straight_high(std::uint32_t rank_mask) {
    std::uint32_t m = (rank_mask << 1) | ((rank_mask >> 12) & 1u);
    std::uint32_t run = m & (m >> 1) & (m >> 2) & (m >> 3) & (m >> 4);
    if (run == 0) return -1;
    return high_bit(run) + 3;
}

// Append the top `n` ranks of `mask` (highest first) to `score`, 4 bits each.
HD inline std::uint32_t pack_top(std::uint32_t score, std::uint32_t mask, int n) {
    for (int i = 0; i < n; i++) {
        int r = high_bit(mask);
        score = (score << 4) | static_cast<std::uint32_t>(r);
        mask &= ~(1u << r);
    }
    return score;
}

// Build a final score: category, then the given ranks, padded to 5 rank slots.
HD inline std::uint32_t finish(std::uint32_t partial, int ranks_packed) {
    for (int i = ranks_packed; i < 5; i++) {
        partial <<= 4;
    }
    return partial;
}

HD inline std::uint32_t evaluate7(const Card* cards) {
    std::uint32_t rank_mask = 0;
    std::uint32_t suit_mask[4] = {0, 0, 0, 0};
    std::uint8_t counts[13] = {0};

    for (int i = 0; i < 7; i++) {
        int rank = cards[i] % 13;
        int suit = cards[i] / 13;
        counts[rank]++;
        suit_mask[suit] |= 1u << rank;
        rank_mask |= 1u << rank;
    }

    // Flush / straight flush (at most one suit can have 5+ of 7 cards).
    std::uint32_t flush_mask = 0;
    for (int s = 0; s < 4; s++) {
        if (popcount(suit_mask[s]) >= 5) {
            flush_mask = suit_mask[s];
        }
    }
    if (flush_mask) {
        int sf = straight_high(flush_mask);
        if (sf >= 0) {
            return finish((STRAIGHT_FLUSH << 4) | static_cast<std::uint32_t>(sf), 1);
        }
    }

    std::uint32_t quads = 0, trips = 0, pairs = 0;
    for (int r = 0; r < 13; r++) {
        if (counts[r] == 4) quads |= 1u << r;
        else if (counts[r] == 3) trips |= 1u << r;
        else if (counts[r] == 2) pairs |= 1u << r;
    }

    if (quads) {
        int q = high_bit(quads);
        std::uint32_t s = (FOUR_OF_A_KIND << 4) | static_cast<std::uint32_t>(q);
        return finish(pack_top(s, rank_mask & ~(1u << q), 1), 2);
    }

    if (trips) {
        int t = high_bit(trips);
        std::uint32_t rest = (trips & ~(1u << t)) | pairs;   // a second trips counts as the pair
        if (rest) {
            int p = high_bit(rest);
            return finish((((FULL_HOUSE << 4) | t) << 4) | static_cast<std::uint32_t>(p), 2);
        }
    }

    if (flush_mask) {
        return pack_top(FLUSH, flush_mask, 5);
    }

    int st = straight_high(rank_mask);
    if (st >= 0) {
        return finish((STRAIGHT << 4) | static_cast<std::uint32_t>(st), 1);
    }

    if (trips) {
        int t = high_bit(trips);
        std::uint32_t s = (THREE_OF_A_KIND << 4) | static_cast<std::uint32_t>(t);
        return finish(pack_top(s, rank_mask & ~(1u << t), 2), 3);
    }

    if (popcount(pairs) >= 2) {
        int p1 = high_bit(pairs);
        int p2 = high_bit(pairs & ~(1u << p1));
        std::uint32_t s = (((TWO_PAIR << 4) | p1) << 4) | static_cast<std::uint32_t>(p2);
        return finish(pack_top(s, rank_mask & ~((1u << p1) | (1u << p2)), 1), 3);
    }

    if (pairs) {
        int p = high_bit(pairs);
        std::uint32_t s = (ONE_PAIR << 4) | static_cast<std::uint32_t>(p);
        return finish(pack_top(s, rank_mask & ~(1u << p), 3), 4);
    }

    return pack_top(HIGH_CARD, rank_mask, 5);
}

/*
 * Compare hero vs villain on a 5-card board.
 * Returns +1 if hero wins, -1 if villain wins, 0 for a tie.
 */
HD inline int compare(Card h1, Card h2, Card v1, Card v2, const Card* board) {
    Card hero[7] = {h1, h2, board[0], board[1], board[2], board[3], board[4]};
    Card vill[7] = {v1, v2, board[0], board[1], board[2], board[3], board[4]};
    std::uint32_t hs = evaluate7(hero);
    std::uint32_t vs = evaluate7(vill);
    return (hs > vs) - (hs < vs);
}

} // namespace hand_eval
} // namespace equity
