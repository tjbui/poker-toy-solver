#pragma once

/*
 * equity.h -- public API of the equity engine.
 *
 * Exact hand-vs-hand / range-vs-range equity: every possible board runout is
 * enumerated and evaluated, so the percentages have zero sampling error.
 *
 * One entry point, compute_equity(), with interchangeable backends
 * (single-thread CPU, multithreaded CPU, CUDA GPU) that all return the exact
 * same counts. Callers pick a backend with EquityOptions::backend, or leave
 * it on Auto.
 */

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace equity {

enum class PlayerMode {
    Exact,   // a single known 2-card hand
    Range    // a set of hands, e.g. "AA", "AKs", "AKo", "AhKd"
};

struct PlayerRequest {
    PlayerMode mode = PlayerMode::Exact;
    std::array<std::optional<std::string>, 2> cards;  // used when mode == Exact
    std::vector<std::string> range;                   // used when mode == Range
};

struct EquityRequest {
    PlayerRequest hero;
    PlayerRequest villain;
    std::array<std::optional<std::string>, 5> community;  // known board: 0, 3, 4 or 5 cards
};

enum class Backend {
    Auto,       // GPU for large jobs when one is available, otherwise CpuMulti
    CpuSingle,  // one thread (the reference implementation)
    CpuMulti,   // the work space sliced across std::threads
    Gpu         // CUDA; throws if the build has no CUDA or no device is present
};

// Optional timing breakdown, filled in when the GPU backend runs.
struct GpuStats {
    double uploadMs = 0.0;      // host -> device copy of the matchup list
    double kernelMs = 0.0;      // kernel only
    int blocks = 0;
    int threadsPerBlock = 0;
};

struct EquityOptions {
    Backend backend = Backend::Auto;
    unsigned cpuThreads = 0;                   // CpuMulti: 0 = std::thread::hardware_concurrency()
    std::uint64_t autoGpuMinRunouts = 200000;  // Auto: smaller jobs stay on the CPU (GPU launch/setup overhead)
    GpuStats* gpuStats = nullptr;              // if set, filled in when the GPU runs
};

struct EquityResult {
    double heroWinPct = 0.0;
    double villainWinPct = 0.0;
    double tiePct = 0.0;
    double runtimeMs = 0.0;                // whole call: parsing, setup, enumeration
    std::uint64_t evaluatedRunouts = 0;    // exact number of (matchup x board) evaluated
    Backend backend = Backend::CpuSingle;  // backend that actually ran (never Auto)
};

/*
 * Exact equity for the request.
 * Throws std::invalid_argument for bad input (bad card/token, card conflicts,
 * no valid matchup) and std::runtime_error if the GPU is requested but not
 * available or a CUDA call fails.
 */
EquityResult compute_equity(const EquityRequest& request, const EquityOptions& options = {});

// True if this build includes the CUDA backend and a CUDA device is present.
bool gpu_available();

// Short name for printing: "auto", "cpu-single", "cpu-multi", "gpu".
const char* backend_name(Backend backend);

} // namespace equity
