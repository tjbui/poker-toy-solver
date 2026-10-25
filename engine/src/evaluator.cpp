/* evaluator.cpp - thin wrappers over the shared hand_eval.h evaluator */

#include "evaluator.h"

/*
 * Evaluates HERO_WIN, VILLAIN_WIN, or TIE given hero, villain and community cards.
 */
result_t evaluate_hands(Card hero1, Card hero2, Card villain1, Card villain2,
                        std::array<Card, 5> community_cards) {
    int r = hand_eval::compare(hero1, hero2, villain1, villain2, community_cards.data());
    if (r > 0) return HERO_WIN;
    if (r < 0) return VILLAIN_WIN;
    return TIE;
} /* evaluate_hands() */

/*
 * Score of a 7-card hand (bigger = stronger).
 */
std::uint64_t evaluate_7_cards(std::array<Card, 7>& cards) {
    return hand_eval::evaluate7(cards.data());
} /* evaluate_7_cards() */
