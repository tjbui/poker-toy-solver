/* test_equity_cpu.cpp -- exact-enumeration equity on the CPU backends + dispatch */

#include "equity/equity.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

using namespace equity;

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

static std::array<std::optional<std::string>, 5> board(std::vector<std::string> cards) {
    std::array<std::optional<std::string>, 5> b{};
    for (std::size_t i = 0; i < cards.size() && i < 5; i++) {
        b[i] = cards[i];
    }
    return b;
}

static EquityOptions on(Backend backend) {
    EquityOptions o;
    o.backend = backend;
    return o;
}

static bool same(const EquityResult& a, const EquityResult& b) {
    return a.evaluatedRunouts == b.evaluatedRunouts && a.heroWinPct == b.heroWinPct &&
           a.villainWinPct == b.villainWinPct && a.tiePct == b.tiePct;
}

static bool throws_invalid(const EquityRequest& req) {
    try {
        compute_equity(req, on(Backend::CpuSingle));
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

int main() {
    std::cout << "\n========== test_equity_cpu ==========\n\n";

    // T1: fully-known board, deterministic winner.
    // Hero has Th -> Ah Kh Qh Jh Th = royal flush. Villain AcAd loses.
    {
        EquityRequest req{};
        req.hero = exact_hand("TH", "3C");
        req.villain = exact_hand("AC", "AD");
        req.community = board({"AH", "KH", "QH", "JH", "2C"});

        EquityResult r = compute_equity(req, on(Backend::CpuSingle));
        check(r.evaluatedRunouts == 1, "T1 river-known -> 1 runout");
        check(r.heroWinPct == 100.0, "T1 hero royal flush wins 100%");
        check(r.backend == Backend::CpuSingle, "T1 ran on cpu-single");
    }

    // T2: flop-known exact vs exact. Single and multithreaded must agree
    //     exactly, and enumerate C(45,2) = 990 boards.
    {
        EquityRequest req{};
        req.hero = exact_hand("AH", "KD");
        req.villain = exact_hand("2C", "2D");
        req.community = board({"7H", "8S", "9C"});

        EquityResult st = compute_equity(req, on(Backend::CpuSingle));
        EquityResult mt = compute_equity(req, on(Backend::CpuMulti));

        check(st.evaluatedRunouts == 990, "T2 flop -> C(45,2) = 990 runouts");
        check(same(st, mt), "T2 cpu-single == cpu-multi (exact)");
    }

    // T3: preflop exact vs exact. Count = C(48,5) = 1,712,304. AsKs vs AhKh
    //     is symmetric under swapping the two suits, so hero% == villain%.
    {
        EquityRequest req{};
        req.hero = exact_hand("AS", "KS");
        req.villain = exact_hand("AH", "KH");

        EquityResult r = compute_equity(req, on(Backend::CpuMulti));
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

        EquityResult r = compute_equity(req, on(Backend::CpuMulti));
        check(r.evaluatedRunouts == 35640, "T4 range vs range -> 36 * 990 = 35640 runouts");
        check(r.heroWinPct > r.villainWinPct, "T4 AA ahead of KK");
    }

    // T5: dispatch. Auto picks the CPU for small jobs, and always matches cpu-single.
    {
        EquityRequest req{};
        req.hero = exact_hand("AH", "KD");
        req.villain = exact_hand("2C", "2D");
        req.community = board({"7H", "8S", "9C", "TD"});   // 44 runouts: far below the GPU threshold

        EquityResult ref = compute_equity(req, on(Backend::CpuSingle));
        EquityResult aut = compute_equity(req);            // default options = Auto
        check(aut.backend == Backend::CpuMulti, "T5 Auto keeps a small job on the CPU");
        check(same(ref, aut), "T5 Auto == cpu-single");

        if (!gpu_available()) {
            bool threw = false;
            try {
                compute_equity(req, on(Backend::Gpu));
            } catch (const std::runtime_error&) {
                threw = true;
            }
            check(threw, "T5 Backend::Gpu throws when no GPU is available");
        }
    }

    // T6: bad input is rejected with std::invalid_argument.
    {
        EquityRequest dup{};
        dup.hero = exact_hand("AH", "KD");
        dup.villain = exact_hand("AH", "2D");                // card shared by both players
        check(throws_invalid(dup), "T6 hero/villain card conflict rejected");

        EquityRequest two_board{};
        two_board.hero = exact_hand("AH", "KD");
        two_board.villain = exact_hand("2C", "2D");
        two_board.community = board({"7H", "8S"});           // 2-card board is not a street
        check(throws_invalid(two_board), "T6 2-card board rejected");

        EquityRequest bad_token{};
        bad_token.hero = range_hand({"AKx"});
        bad_token.villain = exact_hand("2C", "2D");
        check(throws_invalid(bad_token), "T6 bad range token rejected");
    }

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n========== end ==========\n";
    return g_failures == 0 ? 0 : 1;
}
