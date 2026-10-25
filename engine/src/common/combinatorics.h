#pragma once

/*
 * combinatorics.h -- n choose k and lexicographic k-combinations, HD.
 *
 * The work space is indexed by "board number b of matchup m", where board b
 * is the b-th k-subset (lexicographic order) of the cards left in the deck.
 * unrank_combination() turns a board number into its subset (the combinatorial
 * number system), so any thread can jump straight to the start of its slice;
 * next_combination() steps to the following board.
 *
 * The GPU kernels use a __constant__ lookup table instead of n_choose_k() for
 * unranking (a GPU-specific speedup); the table itself is filled from n_choose_k().
 */

#include <cstdint>

#include "hd.h"

namespace equity {

HD inline std::uint64_t n_choose_k(int n, int k) {
    if (k < 0 || n < 0 || k > n) return 0;
    if (k == 0 || k == n) return 1;
    if (k > n - k) k = n - k;                  // symmetry: C(n,k) == C(n,n-k)

    std::uint64_t result = 1;
    for (int i = 0; i < k; i++) {
        result = result * static_cast<std::uint64_t>(n - i) / static_cast<std::uint64_t>(i + 1);
    }
    return result;
}

/*
 * Write the r-th (0-indexed) k-combination of {0..n-1}, in lexicographic
 * order, into combo[0..k-1] (ascending).
 */
HD inline void unrank_combination(std::uint64_t r, int n, int k, int* combo) {
    int x = 0;
    for (int i = 0; i < k; i++) {
        while (true) {
            std::uint64_t c = n_choose_k(n - 1 - x, k - 1 - i);
            if (r < c) break;
            r -= c;
            x++;
        }
        combo[i] = x;
        x++;                                   // next element strictly larger
    }
}

/*
 * Advance combo[] to the next lexicographic k-combination of {0..n-1}.
 * Returns false when there is no next combination.
 */
HD inline bool next_combination(int* combo, int n, int k) {
    int i = k - 1;
    while (i >= 0 && combo[i] == n - k + i) {
        i--;
    }
    if (i < 0) return false;
    combo[i]++;
    for (int j = i + 1; j < k; j++) {
        combo[j] = combo[j - 1] + 1;
    }
    return true;
}

} // namespace equity
