/* enumerate.cpp -- the core: one exact matchup on a known board, every runout */

#include "cpu_backend.h"

#include "common/combinatorics.h"
#include "common/hand_eval.h"

/*
 * Terms:
 *   matchup -- one exact hero hand vs one exact villain hand (4 distinct cards).
 *   runout  -- one way to deal the missing community cards. On a known flop
 *              that's a (turn, river) pair: C(45,2) = 990 runouts.
 *
 * Runouts are numbered 0 .. R-1 in lexicographic order (see combinatorics.h),
 * so a caller can ask for all of them or just a range of them.
 */

namespace equity {

namespace {

// Fill `deck` with the cards the matchup and known board leave unseen (ascending).
// Returns how many.
int build_deck(const Matchup& mu, const Card* known, int known_count, Card deck[52]) {
    bool used[52] = {false};
    used[mu.h1] = used[mu.h2] = used[mu.v1] = used[mu.v2] = true;
    for (int j = 0; j < known_count; j++) {
        used[known[j]] = true;
    }

    int deck_size = 0;
    for (int card = 0; card < 52; card++) {
        if (!used[card]) {
            deck[deck_size++] = static_cast<Card>(card);
        }
    }
    return deck_size;
}

// Add one showdown result (+1 hero, -1 villain, 0 tie) to the counts.
void tally(EquityCounts& counts, int result) {
    if (result > 0) {
        counts.heroWins++;
    } else if (result < 0) {
        counts.villainWins++;
    } else {
        counts.ties++;
    }
}

} // namespace

std::uint64_t runouts_per_matchup(int known_count) {
    return n_choose_k(48 - known_count, 5 - known_count);
}

EquityCounts cpu_count_matchup(const Matchup& mu, const Card* known, int known_count,
                               std::uint64_t runout_from, std::uint64_t runout_to) {
    EquityCounts counts;
    if (runout_from >= runout_to) return counts;

    const int k = 5 - known_count;          // community cards still to deal

    Card deck[52];
    const int deck_size = build_deck(mu, known, known_count, deck);

    // The final 5-card board: the known cards, then the current runout's cards.
    Card board[5];
    for (int j = 0; j < known_count; j++) {
        board[j] = known[j];
    }

    // The current runout as positions into deck[] (ascending), e.g. {1, 8}.
    // Jump to the first runout we were asked for, then step through the rest.
    int runout_positions[5];
    unrank_combination(runout_from, deck_size, k, runout_positions);

    for (std::uint64_t runout = runout_from; runout < runout_to; runout++) {
        for (int j = 0; j < k; j++) {
            board[known_count + j] = deck[runout_positions[j]];
        }
        tally(counts, hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board));
        next_combination(runout_positions, deck_size, k);
    }
    return counts;
}

EquityCounts cpu_count_matchup(const Matchup& mu, const Card* known, int known_count) {
    return cpu_count_matchup(mu, known, known_count, 0, runouts_per_matchup(known_count));
}

// Ranges are a loop around the core: add up every matchup's counts.
EquityCounts cpu_count_single(const Workload& w) {
    EquityCounts counts;
    for (const Matchup& mu : w.matchups) {
        counts += cpu_count_matchup(mu, w.known.data(), w.knownCount);
    }
    return counts;
}

} // namespace equity
