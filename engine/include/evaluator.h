#pragma once

#include <vector>
#include <cstdint>

enum result_t {
    HERO_WIN = 0,
    VILLAIN_WIN = 1,
    TIE = 2
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

struct HandValue {
    hand_rank_t rank;
    uint64_t score;
};

// core evaluator
result_t evaluate_hand(
    int hero0,
    int hero1,
    int villain0,
    int villain1,
    const std::vector<int>& community
);