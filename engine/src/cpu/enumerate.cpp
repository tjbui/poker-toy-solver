/* enumerate.cpp -- CPU enumeration of a slice of the work space */

#include "cpu_backend.h"

#include <algorithm>

#include "common/combinatorics.h"
#include "common/hand_eval.h"

namespace equity {

/*
 * Walk [lo, hi) matchup by matchup: build that matchup's deck once, unrank the
 * first board of the slice, then step board to board with next_combination.
 */
EquityCounts cpu_count_range(const Workload& w, std::uint64_t lo, std::uint64_t hi) {
    EquityCounts counts;
    if (w.boards == 0) return counts;

    Card board[5];
    for (int j = 0; j < w.knownCount; j++) {
        board[j] = w.known[j];
    }

    std::uint64_t i = lo;
    while (i < hi) {
        const std::uint64_t m = i / w.boards;          // which matchup
        const std::uint64_t b0 = i % w.boards;         // first board of this matchup in the slice
        const std::uint64_t b_end = std::min<std::uint64_t>(w.boards, b0 + (hi - i));
        const Matchup& mu = w.matchups[m];

        // this matchup's deck: the cards not used by the holes or the known board
        bool used[52] = {false};
        used[mu.h1] = used[mu.h2] = used[mu.v1] = used[mu.v2] = true;
        for (int j = 0; j < w.knownCount; j++) {
            used[w.known[j]] = true;
        }
        Card deck[52];
        int deck_n = 0;
        for (int c = 0; c < 52; c++) {
            if (!used[c]) deck[deck_n++] = static_cast<Card>(c);
        }

        int combo[5];
        unrank_combination(b0, deck_n, w.k, combo);

        for (std::uint64_t b = b0; b < b_end; b++) {
            for (int j = 0; j < w.k; j++) {
                board[w.knownCount + j] = deck[combo[j]];
            }

            int r = hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board);
            if (r > 0) {
                counts.heroWins++;
            } else if (r < 0) {
                counts.villainWins++;
            } else {
                counts.ties++;
            }

            next_combination(combo, deck_n, w.k);
        }

        i = m * w.boards + b_end;                       // == hi when this was the last chunk
    }

    return counts;
}

EquityCounts cpu_count_single(const Workload& w) {
    return cpu_count_range(w, 0, w.total);
}

} // namespace equity
