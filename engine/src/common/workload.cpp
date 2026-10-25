/* workload.cpp -- parse an EquityRequest into the Workload every backend runs */

#include "workload.h"

#include <cctype>
#include <optional>
#include <stdexcept>
#include <string>

#include "combinatorics.h"

namespace equity {

namespace {

struct HoleCards {
    Card a;
    Card b;
};

// ---- parsing ---------------------------------------------------------------

int rank_from_char(char ch) {
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

int suit_from_char(char ch) {
    switch (std::toupper(static_cast<unsigned char>(ch))) {
        case 'H': return 0;
        case 'D': return 1;
        case 'C': return 2;
        case 'S': return 3;
    }
    throw std::invalid_argument(std::string("Invalid card suit: ") + ch);
}

Card make_card(int rank, int suit) {
    return static_cast<Card>(suit * 13 + rank);
}

// Parse a 2-char card string like "AH" or "td" into a Card (0..51).
Card parse_card(const std::string& card_str) {
    if (card_str.size() != 2) {
        throw std::invalid_argument("Invalid card string: " + card_str);
    }
    return make_card(rank_from_char(card_str[0]), suit_from_char(card_str[1]));
}

// ---- range expansion -------------------------------------------------------

/*
 * Expand a single range token into concrete 2-card combos:
 *   "AhKd" -> 1 explicit combo
 *   "AA"   -> pocket pair, C(4,2) = 6 combos
 *   "AK"   -> unspecified two ranks, all 16 combos
 *   "AKs"  -> suited, 4 combos
 *   "AKo"  -> offsuit, 12 combos
 */
std::vector<HoleCards> expand_token(const std::string& t) {
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

/*
 * One player's request -> concrete 2-card combos.
 *   Exact -> the single hand (validated against the known board).
 *   Range -> every combo the tokens imply, minus combos using a board card.
 */
std::vector<HoleCards> expand_player(const PlayerRequest& player, const bool (&blocked)[52]) {
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

    for (const std::string& token : player.range) {
        for (const HoleCards& hc : expand_token(token)) {
            if (!blocked[hc.a] && !blocked[hc.b]) {
                hands.push_back(hc);
            }
        }
    }
    return hands;
}

} // namespace

Workload prepare_workload(const EquityRequest& request) {
    Workload w;

    // Known board: compact the given cards and mark them as used.
    bool blocked[52] = {false};
    for (const auto& slot : request.community) {
        if (!slot) continue;
        Card c = parse_card(*slot);
        if (blocked[c]) {
            throw std::invalid_argument("Duplicate board card: " + *slot);
        }
        blocked[c] = true;
        w.known[w.knownCount++] = c;
    }
    if (w.knownCount == 1 || w.knownCount == 2) {
        throw std::invalid_argument("Board must have 0, 3, 4 or 5 cards");
    }

    std::vector<HoleCards> hero = expand_player(request.hero, blocked);
    std::vector<HoleCards> villain = expand_player(request.villain, blocked);

    // Every (hero, villain) pair with four distinct hole cards.
    w.matchups.reserve(hero.size() * villain.size());
    for (const HoleCards& h : hero) {
        for (const HoleCards& v : villain) {
            if (h.a == v.a || h.a == v.b || h.b == v.a || h.b == v.b) {
                continue;
            }
            w.matchups.push_back({h.a, h.b, v.a, v.b});
        }
    }
    if (w.matchups.empty()) {
        throw std::invalid_argument(
            "No valid hero/villain matchups (empty range or total card conflict)");
    }

    w.k = 5 - w.knownCount;
    w.deckSize = 48 - w.knownCount;
    w.boards = n_choose_k(w.deckSize, w.k);
    w.total = static_cast<std::uint64_t>(w.matchups.size()) * w.boards;
    return w;
}

} // namespace equity
