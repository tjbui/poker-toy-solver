/* test_equity_gpu.cpp -- the GPU backend must match the CPU backend EXACTLY */

#include "equity/equity.h"

#include <array>
#include <cstdint>
#include <iostream>
#include <optional>
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
    for (std::size_t i = 0; i < cards.size() && i < 5; i++) b[i] = cards[i];
    return b;
}

static bool same(const EquityResult& a, const EquityResult& b) {
    return a.evaluatedRunouts == b.evaluatedRunouts &&
           a.heroWinPct == b.heroWinPct &&
           a.villainWinPct == b.villainWinPct &&
           a.tiePct == b.tiePct;
}

static EquityResult cpu(const EquityRequest& req) {
    EquityOptions o;
    o.backend = Backend::CpuMulti;
    return compute_equity(req, o);
}

static EquityResult gpu(const EquityRequest& req, GpuKernel kernel,
                        bool block_reduce = true, bool pinned = false) {
    EquityOptions o;
    o.backend = Backend::Gpu;
    o.gpu.kernel = kernel;
    o.gpu.blockReduce = block_reduce;
    o.gpu.pinnedUpload = pinned;
    return compute_equity(req, o);
}

struct Case {
    std::string name;
    EquityRequest req;
};

int main() {
    std::cout << "\n========== test_equity_gpu ==========\n\n";

    if (!gpu_available()) {
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

    const GpuKernel kernels[] = {GpuKernel::GridStride, GpuKernel::Chunked};
    const char* kernel_names[] = {"grid-stride", "chunked"};

    for (const Case& c : cases) {
        EquityResult ref = cpu(c.req);
        for (int k = 0; k < 2; k++) {
            check(same(ref, gpu(c.req, kernels[k])), c.name + " [" + kernel_names[k] + "]");
        }
    }

    // Other code paths: per-thread atomics instead of block reduction, pinned upload.
    {
        const EquityRequest& r = cases[5].req;
        EquityResult ref = cpu(r);
        for (int k = 0; k < 2; k++) {
            check(same(ref, gpu(r, kernels[k], false)),
                  std::string("per-thread atomics [") + kernel_names[k] + "]");
            check(same(ref, gpu(r, kernels[k], true, true)),
                  std::string("pinned upload [") + kernel_names[k] + "]");
        }
    }

    // Auto sends a large job to the GPU (and still matches the CPU).
    {
        const EquityRequest& r = cases[0].req;              // 1.7M runouts
        EquityResult aut = compute_equity(r);
        check(aut.backend == Backend::Gpu, "Auto sends a large job to the GPU");
        check(same(cpu(r), aut), "Auto (gpu) == cpu-multi");
    }

    std::cout << "\n" << (g_failures == 0 ? "ALL PASSED" : "FAILURES: " + std::to_string(g_failures))
              << "\n========== end ==========\n";
    return g_failures == 0 ? 0 : 1;
}
