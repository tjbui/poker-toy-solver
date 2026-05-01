#pragma once

#include <cstdint>
#include <array>
#include <vector>

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
uint64_t evaluate_7_cards(std::array<Card, 7>& cards);
int find_straight_high(const std::array<int, 13>& rank_counts);
uint64_t try_straight_flush(std::array<std::vector<uint8_t>, 4>& ranks_by_suit);
uint64_t try_quads(std::array<uint8_t, 13>& rank_counts);
uint64_t try_full_house(std::array<uint8_t, 13>& rank_counts);
uint64_t try_flush(std::array<std::vector<uint8_t>, 4>& ranks_by_suit);
uint64_t try_straight(std::array<uint8_t, 13>& rank_counts);
uint64_t try_three_of_a_kind(std::array<uint8_t, 13>& rank_counts);
uint64_t try_two_pair(std::array<uint8_t, 13>& rank_counts);
uint64_t try_one_pair(std::array<uint8_t, 13>& rank_counts);
uint64_t make_high_card(std::array<uint8_t, 13>& rank_counts);