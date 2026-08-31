#pragma once

#include "engine.h"
#include "evaluator.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/*
 * simulation.h -- exact runout enumeration.
 *
 * The public API (engine.h) is exact-vs-exact / range-vs-range equity. This
 * header holds the reusable pieces both engine backends (single-thread and
 * multi-thread) share:
 *
 *   1. parsing        strings -> Card / known board
 *   2. range expansion  "AA"/"AKs"/"AhKd" -> concrete 2-card combos
 *   3. matchups       every valid (hero hand, villain hand) with no card clash
 *   4. enumeration    for each matchup, every remaining board runout
 *
 * A Card is 0..51, encoded as suit*13 + rank (see evaluator.h).
 */

// ---- parsing --------------------------------------------------------------

Card parse_card(const std::string& card_str);

std::array<std::optional<Card>, 5> parse_community(
    const std::array<std::optional<std::string>, 5>& community);

// ---- range expansion ------------------------------------------------------

struct HoleCards {
    Card a;
    Card b;
};

// Expand one player's request into concrete 2-card combos.
//   Exact -> the single hand (validated against the known board).
//   Range -> every combo the notation implies, dropping combos that use a
//            card already on the known board.
std::vector<HoleCards> expand_player(
    const PlayerRequest& player,
    const std::array<std::optional<Card>, 5>& known_community);

// ---- matchups -------------------------------------------------------------

struct Matchup {
    Card h1, h2;   // hero hole cards
    Card v1, v2;   // villain hole cards
};

// Every (hero hand, villain hand) pair whose four cards are all distinct.
std::vector<Matchup> build_matchups(
    const std::vector<HoleCards>& hero_hands,
    const std::vector<HoleCards>& villain_hands);

// Parse + expand + build, in one call. Fills `known_community_out`.
// Throws std::invalid_argument on bad input or if no valid matchup exists.
std::vector<Matchup> prepare_matchups(
    const EquityRequest& request,
    std::array<std::optional<Card>, 5>& known_community_out);

// ---- enumeration ----------------------------------------------------------

std::uint64_t n_choose_k(int n, int k);

struct EquityCounts {
    std::uint64_t hero_wins = 0;
    std::uint64_t villain_wins = 0;
    std::uint64_t ties = 0;
    std::uint64_t runouts = 0;
};

// Total work = matchups.size() * (boards per matchup).
std::uint64_t total_runouts(
    const std::vector<Matchup>& matchups,
    const std::array<std::optional<Card>, 5>& known_community);

// Evaluate the contiguous slice [global_lo, global_hi) of that work space.
// Splitting the [0, total) range across threads is how the multi-threaded
// backend parallelizes -- each slice is independent.
EquityCounts count_slice(
    const std::vector<Matchup>& matchups,
    const std::array<std::optional<Card>, 5>& known_community,
    std::uint64_t global_lo,
    std::uint64_t global_hi);

// Turn raw counts into percentages + metadata.
EquityResult finalize_result(const EquityCounts& counts, double runtimeMs);
