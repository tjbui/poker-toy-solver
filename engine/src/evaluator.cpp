/* evaluator.cpp - functionality to evaluate winner between hero and villain*/

#include "evaluator.h"
#include <iostream>
#include <algorithm>
#include <vector>

/*
 * Evaluates Hero_Win, Villain_Win, or Tie given villain, hero and community cards. Returns 
 * enum class Evaluator_result. 
 */
result_t evaluate_hands(Card hero1, Card hero2, Card villain1, Card villain2, std::array<Card, 5> community_cards) {
    std::array<Card, 7> hero_cards = {hero1, hero2,
                                      community_cards[0], community_cards[1], community_cards[2],
                                      community_cards[3], community_cards[4]};
    std::array<Card, 7> villain_cards = {villain1, villain2,
                                         community_cards[0], community_cards[1], community_cards[2],
                                         community_cards[3], community_cards[4]};
    
    uint64_t hero_score = evaluate_7_cards(hero_cards);
    uint64_t villain_score = evaluate_7_cards(villain_cards);

    if (hero_score > villain_score) return HERO_WIN;
    if (villain_score > hero_score) return VILLAIN_WIN;

    return TIE;
} /* evaluate_hands() */

/* 
 * Returns base 13 score of [category][rank1][rank2][rank3][rank3][rank5]
 * For example 10s full of 3s will be:
 * category = FULL_HOUSE,
 * rank1 = 8,
 * rank2 = 1
 */
uint64_t evaluate_7_cards(std::array<Card, 7>& cards) {
    std::array<uint8_t, 13> rank_counts = {};
    std::array<std::vector<uint8_t>, 4> ranks_by_suit; // [ranks for suit 0, ranks for suit 1, ...]

    for (Card card : cards) {
        int rank = card % 13;
        int suit = card / 13;

        rank_counts[rank]++;
        ranks_by_suit[suit].push_back(rank);
    }

    if (uint64_t score = try_straight_flush(ranks_by_suit)) return score;
    if (uint64_t score = try_quads(rank_counts)) return score;
    if (uint64_t score = try_full_house(rank_counts)) return score;
    if (uint64_t score = try_flush(ranks_by_suit)) return score;
    if (uint64_t score = try_straight(rank_counts)) return score;
    if (uint64_t score = try_three_of_a_kind(rank_counts)) return score;
    if (uint64_t score = try_two_pair(rank_counts)) return score;
    if (uint64_t score = try_one_pair(rank_counts)) return score;

    return make_high_card(rank_counts);
} /* evaluate_7_cards() */

/*
 * Compresses score into a 64 bit int, uses base 13 score of:
 * [category][rank1][rank2][rank3][rank3][rank5]
 */
uint64_t make_score(int category, std::vector<int> ranks) {
    uint64_t score = category;

    int count = 0;
    for (int r : ranks) {
        score = score * 13 + r;
        count++;
    }

    while (count < 5) {
        score = score * 13;
        count++;
    }

    return score;
} /* make_score() */

/*
 * Returns highest rank of the straight if there is a straight flush
 */
uint64_t try_straight_flush(std::array<std::vector<uint8_t>, 4>& ranks_by_suit) {
    for (int suit = 0; suit < 4; suit++) {
        if (ranks_by_suit[suit].size() >= 5) {
            std::array<int, 13> suited_counts = {};

            for (int rank : ranks_by_suit[suit]) {
                suited_counts[rank]++;
            }

            int high = find_straight_high(suited_counts);
            if (high != -1) {
                return make_score(STRAIGHT_FLUSH, {high});
            }
        }
    }

    return 0;
} /* try_straight_flush() */

/* 
 * Returns rank of quads and kicker
 */
uint64_t try_quads(std::array<uint8_t, 13>& rank_counts) {
    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] == 4) {
            int kicker = -1;

            for (int k = 12; k >= 0; k--) {
                if (k != r && rank_counts[k] > 0) {
                    kicker = k;
                    break;
                }
            }

            return make_score(FOUR_OF_A_KIND, {r, kicker});
        }
    }

    return 0;
} /* try_quads() */

/*
 * Returns rank of the trips and rank of pair if there is full house
 */
uint64_t try_full_house(std::array<uint8_t, 13>& rank_counts) {
    int trip = -1;
    int pair = -1;

    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] >= 3) {
            if (trip == -1) {
                trip = r;
            } else if (pair == -1) {
                pair = r;
            }
        }
    }

    for (int r = 12; r >= 0; r--) {
        if (r != trip && rank_counts[r] >= 2) {
            pair = r;
            break;
        }
    }

    if (trip != -1 && pair != -1) {
        return make_score(FULL_HOUSE, {trip, pair});
    }

    return 0;
} /* try_full_house() */

/*
 * Returns all five cards of flush
 */
uint64_t try_flush(std::array<std::vector<uint8_t>, 4>& ranks_by_suit) {
    for (int suit = 0; suit < 4; suit++) {
        if (ranks_by_suit[suit].size() >= 5) {
            auto ranks = ranks_by_suit[suit];
            std::sort(ranks.begin(), ranks.end(), std::greater<int>());

            return make_score(FLUSH, {
                ranks[0], ranks[1], ranks[2], ranks[3], ranks[4]
            });
        }
    }

    return 0;
} /* try_flush() */

/*
 * Returns highest card of the straight
 */
uint64_t try_straight(std::array<uint8_t, 13>& rank_counts) {
    std::array<int, 13> counts = {};

    for (int i = 0; i < 13; i++) {
        counts[i] = rank_counts[i];
    }

    int straight_high = find_straight_high(counts);
    if (straight_high != -1) {
        return make_score(STRAIGHT, {straight_high});
    }

    return 0;
} /* try_straight() */

/*
 * Returns the rank of the trips/set and 2 kickers
 */
uint64_t try_three_of_a_kind(std::array<uint8_t, 13>& rank_counts) {
    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] == 3) {
            std::vector<int> kickers;

            for (int k = 12; k >= 0; k--) {
                if (k != r && rank_counts[k] > 0) {
                    kickers.push_back(k);
                }
            }

            return make_score(THREE_OF_A_KIND, {r, kickers[0], kickers[1]});
        }
    }

    return 0;
} /* try_three_of_a_kind() */

/*
 * Returns the pairs of the two pairs plus the one kicker
 */
uint64_t try_two_pair(std::array<uint8_t, 13>& rank_counts) {
    std::vector<int> pairs;

    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] >= 2) {
            pairs.push_back(r);
        }
    }

    if (pairs.size() >= 2) {
        int kicker = -1;

        for (int k = 12; k >= 0; k--) {
            if (k != pairs[0] && k != pairs[1] && rank_counts[k] > 0) {
                kicker = k;
                break;
            }
        }

        return make_score(TWO_PAIR, {pairs[0], pairs[1], kicker});
    }

    return 0;
} /* try_two_pair() */

/*
 * Returns the pair plus three kickers
 */
uint64_t try_one_pair(std::array<uint8_t, 13>& rank_counts) {
    std::vector<int> pairs;

    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] >= 2) {
            pairs.push_back(r);
        }
    }

    if (pairs.size() == 1) {
        std::vector<int> kickers;

        for (int k = 12; k >= 0; k--) {
            if (k != pairs[0] && rank_counts[k] > 0) {
                kickers.push_back(k);
            }
        }

        return make_score(ONE_PAIR, {
            pairs[0], kickers[0], kickers[1], kickers[2]
        });
    }

    return 0;
} /* try_one_pair() */

/*
 * Returns the highest cards in order 
 */
uint64_t make_high_card(std::array<uint8_t, 13>& rank_counts) {
    std::vector<int> high_cards;

    for (int r = 12; r >= 0; r--) {
        if (rank_counts[r] > 0) {
            high_cards.push_back(r);
        }
    }

    return make_score(HIGH_CARD, {
        high_cards[0], high_cards[1], high_cards[2],
        high_cards[3], high_cards[4]
    });
} /* make_high_card() */

/*
 * Returns the highest rank of the straight
 */
int find_straight_high(const std::array<int, 13>& rank_counts) {
    /* normal straights: A-high down to 6-high */
    for (int high = 12; high >= 4; high--) {
        bool found = true;
        for (int r = high; r > high - 5; r--) {
            if (rank_counts[r] == 0) {
                found = false;
                break;
            }
        }

        if (found) return high;
    }

    /* wheel straight: A 2 3 4 5 */
    if (rank_counts[12] && rank_counts[0] && rank_counts[1] &&
        rank_counts[2] && rank_counts[3]) {
        return 3; // 5-high straight
    }

    return -1;
} /* find_straight_high() */