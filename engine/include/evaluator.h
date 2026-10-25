#pragma once

/*
 * evaluator.h -- public hand-evaluation API.
 *
 * Thin wrappers over the shared, allocation-free evaluator in hand_eval.h
 * (the same code the CUDA kernel uses).
 */

#include <array>
#include <cstdint>

#include "hand_eval.h"

enum result_t {
    HERO_WIN,
    VILLAIN_WIN,
    TIE
};

result_t evaluate_hands(Card hero1, Card hero2, Card villain1, Card villain2,
                        std::array<Card, 5> community_cards);

// Score of a 7-card hand; only comparisons between scores are meaningful.
std::uint64_t evaluate_7_cards(std::array<Card, 7>& cards);
