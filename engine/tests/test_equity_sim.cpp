/* test_equity_sim.cpp -- exact-enumeration equity tests (no tolerances) */

#include "engine.h"

#include <cstdint>
#include <iostream>
#include <optional>
#include <string>

static int g_failures = 0;

static void check(bool cond, const std::string& name) {
    std::cout << (cond ? "[PASS] " : "[FAIL] ") << name << "\n";
    if (!cond) g_failures++;
}

static PlayerRequest exact_hand(const char* c1, const char* c2) {
    PlayerRequest p{};
    p.mode = PlayerMode::Exact;
    p.cards = {std::optional<std::string>(c1), std::optional<std::string>(c2)};
    return p;
}

static PlayerRequest range_hand(std::vector<std::string> tokens) {
    PlayerRequest p{};
    p.mode = PlayerMode::Range;
    p.range = std::move(tokens);
    return p;
}

static std::array<std::optional<std::string>, 5> board(
    std::vector<std::string> cards
) {
    std::array<std::optional<std::string>, 5> b{};
    for (std::size_t i = 0; i < cards.size() && i < 5; i++) {
        b[i] = cards[i];
    }
    return b;
}

int main() {
    std::cout << "\n========== test_equity_sim ==========\n\n";

    // T1: fully-known board, deterministic winner.
    // Hero has Th -> uses Ah Kh Qh Jh Th = royal flush. Villain AcAd loses.
    {
        EquityRequest req{};
        req.hero = exact_hand("TH", "3C");
        req.villain = exact_hand("AC", "AD");
        req.community = board({"AH", "KH", "QH", "JH", "2C"});

        EquityResult r = get_equity_iterative(req);
        check(r.evaluatedRunouts == 1, "T1 river-known -> 1 runout");
        check(r.heroWinPct == 100.0, "T1 hero royal flush wins 100%");
    }

    // T2: flop-known exact vs exact. Iterative and multithreaded must agree
    //     exactly, and enumerate C(45,2) = 990 boards.
    {
        EquityRequest req{};
        req.hero = exact_hand("AH", "KD");
        req.villain = exact_hand("2C", "2D");
        req.community = board({"7H", "8S", "9C"});

        EquityResult it = get_equity_iterative(req);
        EquityResult mt = get_equity_multithr(req);

        check(it.evaluatedRunouts == 990, "T2 flop -> C(45,2) = 990 runouts");
        check(mt.evaluatedRunouts == 990, "T2 multithr -> 990 runouts");
        check(it.heroWinPct == mt.heroWinPct &&
              it.villainWinPct == mt.villainWinPct &&
              it.tiePct == mt.tiePct,
              "T2 iterative == multithreaded (exact)");
    }

    // T3: preflop exact vs exact. Count = C(48,5) = 1,712,304. AsKs vs AhKh
    //     is symmetric under swapping the two suits, so hero% == villain%.
    {
        EquityRequest req{};
        req.hero = exact_hand("AS", "KS");
        req.villain = exact_hand("AH", "KH");

        EquityResult r = get_equity_multithr(req);
        check(r.evaluatedRunouts == 1712304, "T3 preflop -> C(48,5) = 1712304 runouts");
        check(r.heroWinPct == r.villainWinPct, "T3 suit symmetry -> hero% == villain%");
    }

    // T4: range vs range. AA (6 combos) vs KK (6 combos) on a dry flop.
    //     36 matchups x C(45,2) = 36 * 990 = 35,640 runouts; AA is ahead.
    {
        EquityRequest req{};
        req.hero = range_hand({"AA"});
        req.villain = range_hand({"KK"});
        req.community = board({"2C", "7D", "9S"});

        EquityResult r = get_equity_multithr(req);
        check(r.evaluatedRunouts == 35640, "T4 range vs range -> 36 * 990 = 35640 runouts");
        check(r.heroWinPct > r.villainWinPct, "T4 AA ahead of KK");
    }

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n========== end ==========\n";
    return g_failures == 0 ? 0 : 1;
}
