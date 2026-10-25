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

// ---- CUDA backend -------------------------------------------------------

enum class CudaKernel {
    GridStride,   // each thread strides through the work space, unranking every index
    Chunked       // each thread owns a contiguous chunk: unrank once, then step
};

struct CudaOptions {
    CudaKernel kernel = CudaKernel::Chunked;
    bool blockReduce = true;     // warp-shuffle block reduction (false: one atomicAdd per thread)
    bool pinnedUpload = false;   // stage the matchup upload through pinned host memory
};

// Optional timing breakdown filled in by get_equity_cuda.
struct CudaStats {
    double pinnedPrepMs = 0.0;  // pinnedUpload only: cudaMallocHost + staging memcpy
    double uploadMs = 0.0;      // host -> device copy of the matchup list (cudaEvent timed)
    double kernelMs = 0.0;      // kernel only (cudaEvent timed)
    int blocks = 0;
    int threadsPerBlock = 0;
};

// Exact enumeration on the GPU (same numbers as the CPU versions).
// Throws std::runtime_error if the engine was built without CUDA.
EquityResult get_equity_cuda(const EquityRequest& request,
                             const CudaOptions& options = {},
                             CudaStats* stats = nullptr);

// True if this build of the engine includes the CUDA backend.
bool cuda_available();
