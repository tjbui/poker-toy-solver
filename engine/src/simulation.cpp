#include "simulation.h"

#include <algorithm>
#include <stdexcept>

/*
 * Parse card as string from EquityRequest as Card (uint8_t)
 */
Card parse_card(const std::string& card_str) {
    if (card_str.size() != 2) {
        throw std::invalid_argument("Invalid card string: " + card_str);
    }

    char rank_char = card_str[0];
    char suit_char = card_str[1];

    int rank = -1;
    if (rank_char >= '2' && rank_char <= '9') {
        rank = rank_char - '2';
    } else if (rank_char == 'T' || rank_char == 't') {
        rank = 8;
    } else if (rank_char == 'J' || rank_char == 'j') {
        rank = 9;
    } else if (rank_char == 'Q' || rank_char == 'q') {
        rank = 10;
    } else if (rank_char == 'K' || rank_char == 'k') {
        rank = 11;
    } else if (rank_char == 'A' || rank_char == 'a') {
        rank = 12;
    } else {
        throw std::invalid_argument("Invalid card rank: " + card_str);
    }

    int suit = -1;
    if (suit_char == 'H' || suit_char == 'h') {
        suit = 0;
    } else if (suit_char == 'D' || suit_char == 'd') {
        suit = 1;
    } else if (suit_char == 'C' || suit_char == 'c') {
        suit = 2;
    } else if (suit_char == 'S' || suit_char == 's') {
        suit = 3;
    } else {
        throw std::invalid_argument("Invalid card suit: " + card_str);
    }

    return static_cast<Card>(suit * 13 + rank);
} /* parse_card() */

/*
 * Create deck (vector of 52 Cards)
 */
std::vector<Card> make_deck() {
    std::vector<Card> deck;
    deck.reserve(52);

    for (int c = 0; c < 52; c++) {
        deck.push_back(static_cast<Card>(c));
    }

    return deck;
} /* make_deck() */

/*
 * Remove card from deck (vector) using erase() 
 */
void remove_card_from_deck(std::vector<Card>& deck, Card card) {
    auto it = std::find(deck.begin(), deck.end(), card);

    if (it == deck.end()) {
        throw std::invalid_argument("Duplicate or invalid card detected");
    }

    deck.erase(it); // TODO: later optimize by making deck an array instead of vector
} /* remove_card_from_deck() */


/*
 * Removes hero, villain, community cards from the deck
 */
void remove_known_cards(
    std::vector<Card>& deck,
    Card hero1,
    Card hero2,
    Card villain1,
    Card villain2,
    const std::array<std::optional<Card>, 5>& known_community
) {
    remove_card_from_deck(deck, hero1);
    remove_card_from_deck(deck, hero2);
    remove_card_from_deck(deck, villain1);
    remove_card_from_deck(deck, villain2);

    for (const auto& card : known_community) {
        if (card) {
            remove_card_from_deck(deck, *card);
        }
    }
} /* remove_known_cards() */

/*
 * Parse community card strings into array of Cards (optional for null if no entry)
 */
std::array<std::optional<Card>, 5> parse_community(
    const std::array<std::optional<std::string>, 5>& community
) {
    std::array<std::optional<Card>, 5> parsed{};

    for (int i = 0; i < 5; i++) {
        if (community[i]) {
            parsed[i] = parse_card(*community[i]);
        }
    }

    return parsed;
} /* parse_community() */

/*
 * Randomly fill out remaining community cards and for simulating hand evaluation 
 */
std::array<Card, 5> fill_remaining_community(
    const std::array<std::optional<Card>, 5>& known_community,
    const std::vector<Card>& deck,
    std::mt19937& rng
) {
    std::vector<Card> sim_deck = deck;
    std::shuffle(sim_deck.begin(), sim_deck.end(), rng);

    std::array<Card, 5> final_community{};
    int draw_index = 0;

    for (int i = 0; i < 5; i++) {
        if (known_community[i]) {
            final_community[i] = *known_community[i];
        } else {
            final_community[i] = sim_deck[draw_index];
            draw_index++;
        }
    }

    return final_community;
} /* fill_remaining_community() */