#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

enum class PlayerMode {
    Exact,   // a single known 2-card hand
    Range    // a set of hands, e.g. "AA", "AKs", "AKo", "AhKd"
};

struct PlayerRequest {
    PlayerMode mode;
    std::array<std::optional<std::string>, 2> cards;  // used when mode == Exact
    std::vector<std::string> range;                   // used when mode == Range
};

struct EquityRequest {
    PlayerRequest hero;
    PlayerRequest villain;
    std::array<std::optional<std::string>, 5> community;  // known board (0..5 cards)
};

/*
 * Result of an EXACT equity calculation.
 *
 * We no longer sample: we enumerate every possible board runout for every
 * valid hero/villain matchup and count wins/ties/losses. So the percentages
 * are exact (zero sampling error) and `evaluatedRunouts` is the exact number
 * of (matchup x board) combinations evaluated -- not a sample count.
 */
struct EquityResult {
    double heroWinPct = 0.0;
    double villainWinPct = 0.0;
    double tiePct = 0.0;
    double runtimeMs = 0.0;
    std::uint64_t evaluatedRunouts = 0;
};

// Exact enumeration, single-threaded.
EquityResult get_equity_iterative(const EquityRequest& request);

// Exact enumeration, multi-threaded (same numbers as the iterative version).
EquityResult get_equity_multithr(const EquityRequest& request);
