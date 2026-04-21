#include "simulation.h"

#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

using std::optional;
using std::string;
using std::unordered_set;
using std::vector;

static int rank_char_to_offset(char r) {
    switch (r) {
        case 'A': return 1;
        case 'K': return 2;
        case 'Q': return 3;
        case 'J': return 4;
        case 'T': return 5;
        case '9': return 6;
        case '8': return 7;
        case '7': return 8;
        case '6': return 9;
        case '5': return 10;
        case '4': return 11;
        case '3': return 12;
        case '2': return 13;
        default:
            throw std::runtime_error("Invalid rank");
    }
}

static int suit_base(char s) {
    switch (s) {
        case 'S': return 0;
        case 'H': return 13;
        case 'D': return 26;
        case 'C': return 39;
        default:
            throw std::runtime_error("Invalid suit");
    }
}

int card_code_to_int(const string& card) {
    if (card.size() != 3 || card[1] != '-') {
        throw std::runtime_error("Invalid card code: " + card);
    }

    const char suit = card[0];
    const char rank = card[2];
    return suit_base(suit) + rank_char_to_offset(rank);
}

vector<int> build_remaining_deck(const EquityRequest& request) {
    unordered_set<int> used;

    auto add_if_present = [&used](const optional<string>& maybe_card) {
        if (maybe_card.has_value()) {
            used.insert(card_code_to_int(*maybe_card));
        }
    };

    for (const auto& c : request.hero.cards) {
        add_if_present(c);
    }

    for (const auto& c : request.villain.cards) {
        add_if_present(c);
    }

    for (const auto& c : request.community) {
        add_if_present(c);
    }

    vector<int> deck;
    deck.reserve(52 - static_cast<int>(used.size()));

    for (int card = 1; card <= 52; ++card) {
        if (used.find(card) == used.end()) {
            deck.push_back(card);
        }
    }

    return deck;
}

void fill_missing_cards(
    const EquityRequest& request,
    vector<int>& hero_cards,
    vector<int>& villain_cards,
    vector<int>& community_cards,
    std::mt19937& rng
) {
    hero_cards.clear();
    villain_cards.clear();
    community_cards.clear();

    vector<int> deck = build_remaining_deck(request);
    std::shuffle(deck.begin(), deck.end(), rng);

    size_t next_idx = 0;

    auto take_known_or_draw = [&](const optional<string>& maybe_card) -> int {
        if (maybe_card.has_value()) {
            return card_code_to_int(*maybe_card);
        }
        if (next_idx >= deck.size()) {
            throw std::runtime_error("Ran out of cards while drawing");
        }
        return deck[next_idx++];
    };

    for (const auto& c : request.hero.cards) {
        hero_cards.push_back(take_known_or_draw(c));
    }

    for (const auto& c : request.villain.cards) {
        villain_cards.push_back(take_known_or_draw(c));
    }

    for (const auto& c : request.community) {
        community_cards.push_back(take_known_or_draw(c));
    }
}