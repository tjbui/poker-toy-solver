#pragma once

#include <optional>
#include <string>
#include <vector>

struct PlayerRequest {
    std::string mode;
    std::vector<std::optional<std::string>> cards;
    std::vector<std::string> range;
};

struct EquityRequest {
    PlayerRequest hero;
    PlayerRequest villain;
    std::vector<std::optional<std::string>> community;
    int simulations;
    std::string engine;
};

struct EquityResult {
    double heroWinPct;
    double villainWinPct;
    double tiePct;
    int simulatedHands;
    double runtimeMs;
    std::string engine;
};

EquityResult run_equity_simulation_iterative(const EquityRequest& request);