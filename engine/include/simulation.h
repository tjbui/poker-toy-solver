#pragma once

#include "evaluator.h"
#include "engine.h"

#include <array>
#include <optional>
#include <random>
#include <string>
#include <vector>

Card parse_card(const std::string& card_str);

std::vector<Card> make_deck();

void remove_card_from_deck(std::vector<Card>& deck, Card card);

void remove_known_cards(
    std::vector<Card>& deck,
    Card hero1,
    Card hero2,
    Card villain1,
    Card villain2,
    const std::array<std::optional<Card>, 5>& known_community
);

std::array<std::optional<Card>, 5> parse_community(
    const std::array<std::optional<std::string>, 5>& community
);

std::array<Card, 5> fill_remaining_community(
    const std::array<std::optional<Card>, 5>& known_community,
    const std::vector<Card>& deck,
    std::mt19937& rng
);