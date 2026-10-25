/* test_equity_cuda.cpp -- the GPU backend must match the CPU backend EXACTLY */

#include "engine.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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
    for (std::size_t i = 0; i < cards.size() && i < 5; i++) b[i] = cards[i];
    return b;
}

static bool same(const EquityResult& a, const EquityResult& b) {
    return a.evaluatedRunouts == b.evaluatedRunouts &&
           a.heroWinPct == b.heroWinPct &&
           a.villainWinPct == b.villainWinPct &&
           a.tiePct == b.tiePct;
}

struct Case {
    std::string name;
    EquityRequest req;
};

int main() {
    std::cout << "\n========== test_equity_cuda ==========\n\n";

    if (!cuda_available()) {
        std::cout << "[FAIL] no CUDA device available\n";
        return 1;
    }

    std::vector<Case> cases;
    {
        EquityRequest r{};
        r.hero = exact_hand("AS", "AH");
        r.villain = exact_hand("KS", "KD");
        cases.push_back({"exact preflop AsAh vs KsKd", r});
    }
    {
        EquityRequest r{};
        r.hero = exact_hand("AH", "KD");
        r.villain = exact_hand("2C", "2D");
        r.community = board({"7H", "8S", "9C"});
        cases.push_back({"exact flop", r});
        r.community = board({"7H", "8S", "9C", "TD"});
        cases.push_back({"exact turn", r});
        r.community = board({"7H", "8S", "9C", "TD", "3S"});
        cases.push_back({"exact river", r});
    }
    {
        EquityRequest r{};
        r.hero = range_hand({"AA", "KK", "QQ", "AKs"});
        r.villain = range_hand({"AKo", "KQs", "JJ"});
        r.community = board({"2C", "7D", "9S"});
        cases.push_back({"range vs range flop", r});
    }
    {
        EquityRequest r{};
        r.hero = range_hand({"AA", "KK"});
        r.villain = range_hand({"AKs", "QQ"});
        cases.push_back({"range vs range preflop", r});
    }

    const CudaKernel kernels[] = {CudaKernel::GridStride, CudaKernel::Chunked};
    const char* kernel_names[] = {"grid-stride", "chunked"};

    for (const Case& c : cases) {
        EquityResult cpu = get_equity_multithr(c.req);
        for (int k = 0; k < 2; k++) {
            CudaOptions opt;
            opt.kernel = kernels[k];
            EquityResult gpu = get_equity_cuda(c.req, opt);
            check(same(cpu, gpu), c.name + " [" + kernel_names[k] + "]");
        }
    }

    // Other code paths: per-thread atomics instead of block reduction, pinned upload.
    {
        const EquityRequest& r = cases[4].req;
        EquityResult cpu = get_equity_multithr(r);
        for (int k = 0; k < 2; k++) {
            CudaOptions opt;
            opt.kernel = kernels[k];
            opt.blockReduce = false;
            check(same(cpu, get_equity_cuda(r, opt)),
                  std::string("per-thread atomics [") + kernel_names[k] + "]");
            opt.blockReduce = true;
            opt.pinnedUpload = true;
            check(same(cpu, get_equity_cuda(r, opt)),
                  std::string("pinned upload [") + kernel_names[k] + "]");
        }
    }

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n========== end ==========\n";
    return g_failures == 0 ? 0 : 1;
}
