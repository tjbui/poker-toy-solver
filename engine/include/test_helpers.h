#pragma once

#include "evaluator.h"
#include <iostream>

inline const char* rank_to_string(int rank) {
    const char* ranks[] = {
        "2","3","4","5","6","7","8","9","T","J","Q","K","A"
    };
    return ranks[rank];
}

inline const char* suit_to_string(int suit) {
    const char* suits[] = {
        "H","D","C","S"
    };
    return suits[suit];
}

inline std::string card_to_string(Card c) {
    int rank = c % 13;
    int suit = c / 13;

    return std::string(rank_to_string(rank)) + suit_to_string(suit);
}

inline const char* result_to_string(result_t res) {
    switch (res) {
        case HERO_WIN: return "HERO_WIN";
        case VILLAIN_WIN: return "VILLAIN_WIN";
        case TIE: return "TIE";
        default: return "UNKNOWN";
    }
}

inline void print_test(const char* name, Card hero1, Card hero2, Card villain1, Card villain2, std::array<Card, 5> community, result_t expected ) {
    result_t actual = evaluate_hands(hero1, hero2, villain1, villain2, community);

    std::cout << "=== " << name << " ===\n";

    std::cout << "Hero: "
              << card_to_string(hero1) << " "
              << card_to_string(hero2) << "\n";

    std::cout << "Villain: "
              << card_to_string(villain1) << " "
              << card_to_string(villain2) << "\n";

    std::cout << "Board: ";
    for (Card c : community) {
        std::cout << card_to_string(c) << " ";
    }
    std::cout << "\n";

    std::cout << "Expected: " << result_to_string(expected)
              << " | Actual: " << result_to_string(actual) << "\n";

    if (expected == actual) {
        std::cout << "[PASS]\n\n";
    } else {
        std::cout << "[FAIL]\n\n";
    }
}