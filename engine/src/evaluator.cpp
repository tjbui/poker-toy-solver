/* evaluator.cpp - functionality to evaluate winner between hero and villain*/

#include "evaluator.h"
#include <iostream>
#include <algorithm>
#include <vector>

uint64_t check_flush(std::array<Card, 7>& cards);

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
    
    /* Check Royal flush */

    /* Check Quads */

    /* Check Full House */

    /* Check flush */
    uint64_t hero_flush_score = check_flush(hero_cards);
    uint64_t villain_flush_score = check_flush(villain_cards);

    if (hero_flush_score > villain_flush_score) return HERO_WIN;
    else if (hero_flush_score < villain_flush_score) return VILLAIN_WIN;

    /* Check Straight*/

    /* Check 3 of a kind */

    /* Check Two Pair */

    /* Check One Pair */

    /* Check High Card */
    

    std::cout << "Hello world\n";
    return TIE;
}

/* 
 * Counts how many cards of each suit.
 * If no suit >= 5 cards, return 0 (no flush)
 * Else, get the 5 highest cards of that suit and convert into a score
 */

uint64_t check_flush(std::array<Card, 7>& cards) {
    std::array<std::vector<int>, 4> ranks_by_suit;

    for (Card card : cards) {
        int suit = card / 13;
        int rank = card % 13;

        ranks_by_suit[suit].push_back(rank);
    }

    for (int suit = 0; suit < 4; suit++) {
        if (ranks_by_suit[suit].size() >= 5) {
            auto& ranks = ranks_by_suit[suit];

            std::sort(ranks.begin(), ranks.end(), std::greater<int>());

            uint64_t score = 0;
            for (int i = 0; i < 5; i++) {
                score = score * 13 + ranks[i]; // The biggest rank should have the most weight
            }

            return score;
        }
    }

    return 0;
}