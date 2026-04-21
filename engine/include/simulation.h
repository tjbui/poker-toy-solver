#pragma once

#include <random>
#include <string>
#include <vector>

#include "engine.h"

int card_code_to_int(const std::string& card);

std::vector<int> build_remaining_deck(const EquityRequest& request);

void fill_missing_cards(
    const EquityRequest& request,
    std::vector<int>& hero_cards,
    std::vector<int>& villain_cards,
    std::vector<int>& community_cards,
    std::mt19937& rng
);