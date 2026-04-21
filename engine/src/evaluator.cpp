#include "evaluator.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

using std::array;
using std::vector;

static int card_id_to_rank_value(int card_id) {
    int local = (card_id - 1) % 13 + 1;

    switch (local) {
        case 1:  return 14; // A
        case 2:  return 13; // K
        case 3:  return 12; // Q
        case 4:  return 11; // J
        case 5:  return 10; // T
        case 6:  return 9;
        case 7:  return 8;
        case 8:  return 7;
        case 9:  return 6;
        case 10: return 5;
        case 11: return 4;
        case 12: return 3;
        case 13: return 2;
        default:
            throw std::runtime_error("Bad card_id");
    }
}

static int card_id_to_suit_index(int card_id) {
    if (card_id >= 1 && card_id <= 13) return 0;
    if (card_id >= 14 && card_id <= 26) return 1;
    if (card_id >= 27 && card_id <= 39) return 2;
    if (card_id >= 40 && card_id <= 52) return 3;
    throw std::runtime_error("Bad card_id");
}

static uint64_t pack_score(hand_rank_t rank, const vector<int>& tiebreakers) {
    uint64_t kicker_score = 0;

    for (int v : tiebreakers) {
        kicker_score = kicker_score * 15ULL + static_cast<uint64_t>(v);
    }

    for (size_t i = tiebreakers.size(); i < 5; ++i) {
        kicker_score = kicker_score * 15ULL;
    }

    return (static_cast<uint64_t>(rank) << 24) | kicker_score;
}

static int get_straight_high(const vector<int>& ranks_desc) {
    bool present[15] = {false};

    for (int r : ranks_desc) {
        present[r] = true;
    }

    if (present[14] && present[5] && present[4] && present[3] && present[2]) {
        return 5;
    }

    for (int high = 14; high >= 5; --high) {
        if (present[high] &&
            present[high - 1] &&
            present[high - 2] &&
            present[high - 3] &&
            present[high - 4]) {
            return high;
        }
    }

    return 0;
}

static HandValue evaluate_five_card_hand(const array<int, 5>& cards) {
    vector<int> ranks;
    vector<int> suits;
    ranks.reserve(5);
    suits.reserve(5);

    for (int c : cards) {
        ranks.push_back(card_id_to_rank_value(c));
        suits.push_back(card_id_to_suit_index(c));
    }

    std::sort(ranks.begin(), ranks.end(), std::greater<int>());

    bool flush = true;
    for (int i = 1; i < 5; ++i) {
        if (suits[i] != suits[0]) {
            flush = false;
            break;
        }
    }

    int straight_high = get_straight_high(ranks);

    int rank_count[15] = {0};
    for (int r : ranks) {
        rank_count[r]++;
    }

    vector<int> quads;
    vector<int> trips;
    vector<int> pairs;
    vector<int> singles;

    for (int r = 14; r >= 2; --r) {
        if (rank_count[r] == 4) quads.push_back(r);
        else if (rank_count[r] == 3) trips.push_back(r);
        else if (rank_count[r] == 2) pairs.push_back(r);
        else if (rank_count[r] == 1) singles.push_back(r);
    }

    if (flush && straight_high) {
        if (straight_high == 14) {
            return {ROYAL_FLUSH, pack_score(ROYAL_FLUSH, {14})};
        }
        return {STRAIGHT_FLUSH, pack_score(STRAIGHT_FLUSH, {straight_high})};
    }

    if (!quads.empty()) {
        return {
            FOUR_OF_A_KIND,
            pack_score(FOUR_OF_A_KIND, {quads[0], singles[0]})
        };
    }

    if (!trips.empty() && !pairs.empty()) {
        return {
            FULL_HOUSE,
            pack_score(FULL_HOUSE, {trips[0], pairs[0]})
        };
    }

    if (trips.size() >= 2) {
        return {
            FULL_HOUSE,
            pack_score(FULL_HOUSE, {trips[0], trips[1]})
        };
    }

    if (flush) {
        return {FLUSH, pack_score(FLUSH, ranks)};
    }

    if (straight_high) {
        return {STRAIGHT, pack_score(STRAIGHT, {straight_high})};
    }

    if (!trips.empty()) {
        return {
            THREE_OF_A_KIND,
            pack_score(THREE_OF_A_KIND, {trips[0], singles[0], singles[1]})
        };
    }

    if (pairs.size() >= 2) {
        return {
            TWO_PAIR,
            pack_score(TWO_PAIR, {pairs[0], pairs[1], singles[0]})
        };
    }

    if (pairs.size() == 1) {
        return {
            ONE_PAIR,
            pack_score(ONE_PAIR, {pairs[0], singles[0], singles[1], singles[2]})
        };
    }

    return {HIGH_CARD, pack_score(HIGH_CARD, ranks)};
}

static HandValue get_best_seven_card_hand(int card0, int card1, const vector<int>& community) {
    vector<int> all_cards;
    all_cards.reserve(7);

    all_cards.push_back(card0);
    all_cards.push_back(card1);
    for (int c : community) {
        all_cards.push_back(c);
    }

    if (all_cards.size() != 7) {
        throw std::runtime_error("Expected exactly 7 cards when evaluating best hand");
    }

    HandValue best{HIGH_CARD, 0};

    for (int a = 0; a < 7; ++a) {
        for (int b = a + 1; b < 7; ++b) {
            for (int c = b + 1; c < 7; ++c) {
                for (int d = c + 1; d < 7; ++d) {
                    for (int e = d + 1; e < 7; ++e) {
                        array<int, 5> combo = {
                            all_cards[a],
                            all_cards[b],
                            all_cards[c],
                            all_cards[d],
                            all_cards[e]
                        };

                        HandValue hv = evaluate_five_card_hand(combo);
                        if (hv.score > best.score) {
                            best = hv;
                        }
                    }
                }
            }
        }
    }

    return best;
}

result_t evaluate_hand(
    int hero0,
    int hero1,
    int villain0,
    int villain1,
    const vector<int>& community
) {
    HandValue hero_best = get_best_seven_card_hand(hero0, hero1, community);
    HandValue villain_best = get_best_seven_card_hand(villain0, villain1, community);

    if (hero_best.score > villain_best.score) {
        return HERO_WIN;
    }
    if (villain_best.score > hero_best.score) {
        return VILLAIN_WIN;
    }
    return TIE;
}