#pragma once

#include <cstdint>
#include <array>

enum result_t {
    HERO_WIN,
    VILLAIN_WIN,
    TIE
};

enum hand_rank_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8,
    ROYAL_FLUSH = 9
};

using Card = std::uint8_t;

result_t evaluate_hands(Card hero1, Card hero2, Card villain1, Card villain2, std::array<Card, 5> community_cards);