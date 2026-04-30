#pragma once

#include <optional>
#include <string>
#include <vector>
#include <array>

enum class PlayerMode {
    Exact,
    Range
};

struct PlayerRequest {
    PlayerMode mode;
    std::array<std::optional<std::string>, 2> cards; 
    std::vector<std::string> range;
};

struct EquityRequest {
    PlayerRequest hero;
    PlayerRequest villain;
    std::array<std::optional<std::string>, 5> community;
    int simulations;
};

struct EquityResult {
    double heroWinPct;
    double villainWinPct;
    double tiePct;
    double runtimeMs;
    int simulatedHands;
};

EquityResult get_equity_iterative(const EquityRequest& request);
EquityResult get_equity_multithr(const EquityRequest& request);