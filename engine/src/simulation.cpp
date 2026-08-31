/* simulation.cpp -- exact runout enumeration (no Monte Carlo) */

#include "simulation.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

// ---------------------------------------------------------------------------
// parsing
// ---------------------------------------------------------------------------

static int rank_from_char(char ch) {
    if (ch >= '2' && ch <= '9') {
        return ch - '2';               // '2'->0 ... '9'->7
    }
    switch (std::toupper(static_cast<unsigned char>(ch))) {
        case 'T': return 8;
        case 'J': return 9;
        case 'Q': return 10;
        case 'K': return 11;
        case 'A': return 12;
    }
    throw std::invalid_argument(std::string("Invalid card rank: ") + ch);
}

static int suit_from_char(char ch) {
    switch (std::toupper(static_cast<unsigned char>(ch))) {
        case 'H': return 0;
        case 'D': return 1;
        case 'C': return 2;
        case 'S': return 3;
    }
    throw std::invalid_argument(std::string("Invalid card suit: ") + ch);
}

static Card make_card(int rank, int suit) {
    return static_cast<Card>(suit * 13 + rank);
}

/*
 * Parse a 2-char card string like "AH" or "td" into a Card (0..51).
 */
Card parse_card(const std::string& card_str) {
    if (card_str.size() != 2) {
        throw std::invalid_argument("Invalid card string: " + card_str);
    }
    return make_card(rank_from_char(card_str[0]), suit_from_char(card_str[1]));
}

/*
 * Parse the up-to-5 known community cards; empty slots stay std::nullopt.
 */
std::array<std::optional<Card>, 5> parse_community(
    const std::array<std::optional<std::string>, 5>& community
) {
    std::array<std::optional<Card>, 5> parsed{};
    for (int i = 0; i < 5; i++) {
        if (community[i]) {
            parsed[i] = parse_card(*community[i]);
        }
    }
    return parsed;
}

// ---------------------------------------------------------------------------
// range expansion
// ---------------------------------------------------------------------------

/*
 * Expand a single range token into concrete 2-card combos:
 *   "AhKd" -> 1 explicit combo
 *   "AA"   -> pocket pair, C(4,2) = 6 combos
 *   "AK"   -> unspecified two ranks, all 16 combos
 *   "AKs"  -> suited, 4 combos
 *   "AKo"  -> offsuit, 12 combos
 */
static std::vector<HoleCards> expand_token(const std::string& t) {
    std::vector<HoleCards> out;

    if (t.size() == 4) {                       // explicit two cards
        Card a = parse_card(t.substr(0, 2));
        Card b = parse_card(t.substr(2, 2));
        if (a == b) {
            throw std::invalid_argument("Duplicate cards in range token: " + t);
        }
        out.push_back({a, b});
        return out;
    }

    if (t.size() == 2) {
        int r1 = rank_from_char(t[0]);
        int r2 = rank_from_char(t[1]);
        if (r1 == r2) {                        // pocket pair
            for (int s1 = 0; s1 < 4; s1++) {
                for (int s2 = s1 + 1; s2 < 4; s2++) {
                    out.push_back({make_card(r1, s1), make_card(r1, s2)});
                }
            }
        } else {                               // "AK" -> all 16 combos
            for (int s1 = 0; s1 < 4; s1++) {
                for (int s2 = 0; s2 < 4; s2++) {
                    out.push_back({make_card(r1, s1), make_card(r2, s2)});
                }
            }
        }
        return out;
    }

    if (t.size() == 3) {
        int r1 = rank_from_char(t[0]);
        int r2 = rank_from_char(t[1]);
        char so = t[2];
        if (r1 == r2) {
            throw std::invalid_argument("A pair cannot be suited/offsuit: " + t);
        }
        if (so == 's' || so == 'S') {          // suited: 4 combos
            for (int s = 0; s < 4; s++) {
                out.push_back({make_card(r1, s), make_card(r2, s)});
            }
        } else if (so == 'o' || so == 'O') {   // offsuit: 12 combos
            for (int s1 = 0; s1 < 4; s1++) {
                for (int s2 = 0; s2 < 4; s2++) {
                    if (s1 != s2) {
                        out.push_back({make_card(r1, s1), make_card(r2, s2)});
                    }
                }
            }
        } else {
            throw std::invalid_argument("Expected 's' or 'o' in range token: " + t);
        }
        return out;
    }

    throw std::invalid_argument("Unrecognized range token: " + t);
}

std::vector<HoleCards> expand_player(
    const PlayerRequest& player,
    const std::array<std::optional<Card>, 5>& known_community
) {
    bool blocked[52] = {false};
    for (const auto& c : known_community) {
        if (c) blocked[*c] = true;
    }

    std::vector<HoleCards> hands;

    if (player.mode == PlayerMode::Exact) {
        if (!player.cards[0] || !player.cards[1]) {
            throw std::invalid_argument("Exact hand requires 2 cards");
        }
        Card a = parse_card(*player.cards[0]);
        Card b = parse_card(*player.cards[1]);
        if (a == b) {
            throw std::invalid_argument("Exact hand has duplicate cards");
        }
        if (blocked[a] || blocked[b]) {
            throw std::invalid_argument("Exact hand conflicts with the board");
        }
        hands.push_back({a, b});
        return hands;
    }

    // Range mode: expand every token, dropping combos blocked by the board.
    for (const std::string& token : player.range) {
        for (const HoleCards& hc : expand_token(token)) {
            if (blocked[hc.a] || blocked[hc.b]) {
                continue;
            }
            hands.push_back(hc);
        }
    }
    return hands;
}

// ---------------------------------------------------------------------------
// matchups
// ---------------------------------------------------------------------------

std::vector<Matchup> build_matchups(
    const std::vector<HoleCards>& hero_hands,
    const std::vector<HoleCards>& villain_hands
) {
    std::vector<Matchup> matchups;
    for (const HoleCards& h : hero_hands) {
        for (const HoleCards& v : villain_hands) {
            // skip if hero and villain share any card
            if (h.a == v.a || h.a == v.b || h.b == v.a || h.b == v.b) {
                continue;
            }
            matchups.push_back({h.a, h.b, v.a, v.b});
        }
    }
    return matchups;
}

std::vector<Matchup> prepare_matchups(
    const EquityRequest& request,
    std::array<std::optional<Card>, 5>& known_community_out
) {
    known_community_out = parse_community(request.community);

    std::vector<HoleCards> hero = expand_player(request.hero, known_community_out);
    std::vector<HoleCards> villain = expand_player(request.villain, known_community_out);

    std::vector<Matchup> matchups = build_matchups(hero, villain);
    if (matchups.empty()) {
        throw std::invalid_argument(
            "No valid hero/villain matchups (empty range or total card conflict)"
        );
    }
    return matchups;
}

// ---------------------------------------------------------------------------
// combinatorics
// ---------------------------------------------------------------------------

std::uint64_t n_choose_k(int n, int k) {
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
 * Combinatorial number system: write the r-th (0-indexed) k-combination of
 * {0..n-1}, in lexicographic order, into combo[0..k-1] (ascending).
 * This lets any thread jump straight to the start of its slice.
 */
static void unrank_combination(std::uint64_t r, int n, int k, int* combo) {
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
static bool next_combination(int* combo, int n, int k) {
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

// ---------------------------------------------------------------------------
// enumeration
// ---------------------------------------------------------------------------

std::uint64_t total_runouts(
    const std::vector<Matchup>& matchups,
    const std::array<std::optional<Card>, 5>& known_community
) {
    int known_count = 0;
    for (const auto& c : known_community) {
        if (c) known_count++;
    }
    int k = 5 - known_count;                   // unknown board cards
    int n = 48 - known_count;                  // deck after removing 4 holes + board
    return static_cast<std::uint64_t>(matchups.size()) * n_choose_k(n, k);
}

EquityCounts count_slice(
    const std::vector<Matchup>& matchups,
    const std::array<std::optional<Card>, 5>& known_community,
    std::uint64_t global_lo,
    std::uint64_t global_hi
) {
    EquityCounts counts;

    // gather the fixed board cards
    std::array<Card, 5> known_cards{};
    int known_count = 0;
    for (const auto& c : known_community) {
        if (c) known_cards[known_count++] = *c;
    }

    const int k = 5 - known_count;             // unknown board cards to draw
    const int n = 48 - known_count;            // per-matchup deck size
    const std::uint64_t B = n_choose_k(n, k);  // boards per matchup
    if (B == 0) return counts;

    std::uint64_t i = global_lo;
    while (i < global_hi) {
        const std::uint64_t m = i / B;         // which matchup
        const std::uint64_t b0 = i % B;        // starting board within it
        const std::uint64_t bEnd =
            std::min<std::uint64_t>(B, b0 + (global_hi - i));

        const Matchup& mu = matchups[m];

        // build this matchup's deck: the cards not used by holes or board
        bool used[52] = {false};
        used[mu.h1] = used[mu.h2] = used[mu.v1] = used[mu.v2] = true;
        for (int j = 0; j < known_count; j++) {
            used[known_cards[j]] = true;
        }
        Card deck[52];
        int deck_n = 0;
        for (int c = 0; c < 52; c++) {
            if (!used[c]) deck[deck_n++] = static_cast<Card>(c);
        }

        // jump to board b0, then walk boards up to bEnd
        int combo[5];
        unrank_combination(b0, deck_n, k, combo);

        for (std::uint64_t b = b0; b < bEnd; b++) {
            std::array<Card, 5> board{};
            for (int j = 0; j < known_count; j++) {
                board[j] = known_cards[j];
            }
            for (int j = 0; j < k; j++) {
                board[known_count + j] = deck[combo[j]];
            }

            result_t r = evaluate_hands(mu.h1, mu.h2, mu.v1, mu.v2, board);
            if (r == HERO_WIN) {
                counts.hero_wins++;
            } else if (r == VILLAIN_WIN) {
                counts.villain_wins++;
            } else {
                counts.ties++;
            }

            if (b + 1 < bEnd) {
                next_combination(combo, deck_n, k);
            }
        }

        counts.runouts += (bEnd - b0);
        i = m * B + bEnd;                       // == global_hi when this was the last chunk
    }

    return counts;
}

EquityResult finalize_result(const EquityCounts& counts, double runtimeMs) {
    EquityResult result{};
    result.runtimeMs = runtimeMs;
    result.evaluatedRunouts = counts.runouts;

    if (counts.runouts > 0) {
        double total = static_cast<double>(counts.runouts);
        result.heroWinPct = 100.0 * static_cast<double>(counts.hero_wins) / total;
        result.villainWinPct = 100.0 * static_cast<double>(counts.villain_wins) / total;
        result.tiePct = 100.0 * static_cast<double>(counts.ties) / total;
    }
    return result;
}
