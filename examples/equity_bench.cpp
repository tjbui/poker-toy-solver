/*
 * equity_bench.cpp -- benchmark the equity backends against each other.
 *
 *   ./build/equity_bench
 *
 * Every timed run is preceded by a warm-up run (this also pays the one-time
 * CUDA context creation cost outside the measurements). Times are medians.
 * "total" = whole call (parse, matchup build, upload, kernel, copy back);
 * "kernel" = GPU kernel only, timed with cudaEvents.
 */

#include "equity/equity.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <functional>
#include <optional>
#include <string>
#include <vector>

using namespace equity;

namespace {

struct Timing {
    double total_ms = 0.0;
    double kernel_ms = 0.0;
    double upload_ms = 0.0;
    double pinned_prep_ms = 0.0;
    int blocks = 0;
    int threads = 0;
    EquityResult result;
};

double median(std::vector<double> v) {
    std::sort(v.begin(), v.end());
    return v[v.size() / 2];
}

// One way of running the engine: fills `stats` when it runs on the GPU.
using Runner = std::function<EquityResult(const EquityRequest&, GpuStats*)>;

Timing measure(const Runner& fn, const EquityRequest& req, int reps) {
    GpuStats st;
    fn(req, &st);                                   // warm-up
    std::vector<double> total, kernel, upload, prep;
    Timing t;
    for (int i = 0; i < reps; i++) {
        st = GpuStats{};
        t.result = fn(req, &st);
        total.push_back(t.result.runtimeMs);
        kernel.push_back(st.kernelMs);
        upload.push_back(st.uploadMs);
        prep.push_back(st.pinnedPrepMs);
    }
    t.total_ms = median(total);
    t.kernel_ms = median(kernel);
    t.upload_ms = median(upload);
    t.pinned_prep_ms = median(prep);
    t.blocks = st.blocks;
    t.threads = st.threadsPerBlock;
    return t;
}

Runner cpu(Backend backend) {
    return [=](const EquityRequest& r, GpuStats*) {
        EquityOptions opt;
        opt.backend = backend;
        return compute_equity(r, opt);
    };
}

Runner gpu(GpuKernel kernel, bool block_reduce = true, bool pinned = false) {
    return [=](const EquityRequest& r, GpuStats* st) {
        EquityOptions opt;
        opt.backend = Backend::Gpu;
        opt.gpu.kernel = kernel;
        opt.gpu.blockReduce = block_reduce;
        opt.gpu.pinnedUpload = pinned;
        opt.gpuStats = st;
        return compute_equity(r, opt);
    };
}

PlayerRequest exact_hand(const char* a, const char* b) {
    PlayerRequest p{};
    p.mode = PlayerMode::Exact;
    p.cards = {std::optional<std::string>(a), std::optional<std::string>(b)};
    return p;
}

PlayerRequest range_hand(std::vector<std::string> tokens) {
    PlayerRequest p{};
    p.mode = PlayerMode::Range;
    p.range = std::move(tokens);
    return p;
}

// Every starting-hand class: 13 pairs + 78 suited + 78 offsuit = 169 tokens (1326 combos).
std::vector<std::string> all_hands() {
    const char* ranks = "AKQJT98765432";
    std::vector<std::string> out;
    for (int i = 0; i < 13; i++) {
        for (int j = i; j < 13; j++) {
            std::string t{ranks[i], ranks[j]};
            if (i == j) {
                out.push_back(t);
            } else {
                out.push_back(t + "s");
                out.push_back(t + "o");
            }
        }
    }
    return out;
}

struct Row {
    std::string name;
    Runner fn;
    int reps;
    bool gpu;
};

void run_case(const std::string& title, const EquityRequest& req, const std::vector<Row>& rows) {
    std::printf("\n=== %s ===\n", title.c_str());
    std::printf("%-30s %14s %11s %11s %11s %12s %8s\n",
                "backend", "runouts", "total ms", "kernel ms", "upload ms", "Mrunouts/s", "vs MT");

    double mt_ms = 0.0;
    for (const Row& row : rows) {
        if (row.gpu && !gpu_available()) {
            std::printf("%-30s %14s\n", row.name.c_str(), "(no GPU)");
            continue;
        }
        Timing t = measure(row.fn, req, row.reps);
        if (row.name == "cpu multithread") mt_ms = t.total_ms;
        double mrps = t.result.evaluatedRunouts / (t.total_ms * 1e3);
        char kernel[32] = "-", upload[32] = "-", speed[32] = "-";
        if (row.gpu) {
            std::snprintf(kernel, sizeof(kernel), "%.2f", t.kernel_ms);
            std::snprintf(upload, sizeof(upload), "%.3f", t.upload_ms);
        }
        if (mt_ms > 0.0) std::snprintf(speed, sizeof(speed), "%.1fx", mt_ms / t.total_ms);
        std::printf("%-30s %14llu %11.2f %11s %11s %12.1f %8s",
                    row.name.c_str(), static_cast<unsigned long long>(t.result.evaluatedRunouts),
                    t.total_ms, kernel, upload, mrps, speed);
        if (row.gpu) std::printf("   [%d x %d]", t.blocks, t.threads);
        if (t.pinned_prep_ms > 0.0) std::printf("   pinned prep %.3f ms", t.pinned_prep_ms);
        std::printf("   hero %.4f%%\n", t.result.heroWinPct);
    }
}

} // namespace

int main() {
    // 1) One matchup, full preflop runout: C(48,5) = 1,712,304 boards.
    {
        EquityRequest r{};
        r.hero = exact_hand("AS", "AH");
        r.villain = exact_hand("KS", "KD");
        run_case("preflop exact: AsAh vs KsKd", r, {
            {"cpu single", cpu(Backend::CpuSingle), 3, false},
            {"cpu multithread", cpu(Backend::CpuMulti), 5, false},
            {"gpu grid-stride", gpu(GpuKernel::GridStride), 5, true},
            {"gpu chunked", gpu(GpuKernel::Chunked), 5, true},
            {"gpu grid-stride (atomics)", gpu(GpuKernel::GridStride, false), 5, true},
            {"gpu chunked (atomics)", gpu(GpuKernel::Chunked, false), 5, true},
        });
    }

    // 2) Range vs range preflop: hundreds of matchups x 1.7M boards each.
    {
        EquityRequest r{};
        r.hero = range_hand({"AA", "KK", "QQ", "AKs"});
        r.villain = range_hand({"JJ", "TT", "AQs", "KQs"});
        run_case("preflop range vs range: AA,KK,QQ,AKs vs JJ,TT,AQs,KQs", r, {
            {"cpu multithread", cpu(Backend::CpuMulti), 3, false},
            {"gpu grid-stride", gpu(GpuKernel::GridStride), 5, true},
            {"gpu chunked", gpu(GpuKernel::Chunked), 5, true},
            {"gpu grid-stride (atomics)", gpu(GpuKernel::GridStride, false), 5, true},
            {"gpu chunked (atomics)", gpu(GpuKernel::Chunked, false), 5, true},
        });
    }

    // 3) Full range vs full range on a known river: ~1M matchups, 1 board each.
    //    Little compute per matchup, so the matchup upload is a real fraction --
    //    this is where pinned memory could show up.
    {
        EquityRequest r{};
        r.hero = range_hand(all_hands());
        r.villain = range_hand(all_hands());
        r.community = {std::string("2C"), std::string("7D"), std::string("9S"),
                       std::string("TH"), std::string("3H")};
        run_case("river, full range vs full range (pageable vs pinned upload)", r, {
            {"cpu multithread", cpu(Backend::CpuMulti), 3, false},
            {"gpu chunked (pageable)", gpu(GpuKernel::Chunked, true, false), 7, true},
            {"gpu chunked (pinned)", gpu(GpuKernel::Chunked, true, true), 7, true},
            {"gpu grid-stride (pageable)", gpu(GpuKernel::GridStride, true, false), 7, true},
        });
    }

    // 4) Full range vs full range on the flop: ~1M matchups x 990 boards = ~1B runouts.
    {
        EquityRequest r{};
        r.hero = range_hand(all_hands());
        r.villain = range_hand(all_hands());
        r.community = {std::string("2C"), std::string("7D"), std::string("9S"),
                       std::nullopt, std::nullopt};
        run_case("flop, full range vs full range (~1B runouts)", r, {
            {"cpu multithread", cpu(Backend::CpuMulti), 1, false},
            {"gpu grid-stride", gpu(GpuKernel::GridStride), 3, true},
            {"gpu chunked", gpu(GpuKernel::Chunked), 3, true},
            {"gpu chunked (pinned)", gpu(GpuKernel::Chunked, true, true), 3, true},
        });
    }

    return 0;
}
