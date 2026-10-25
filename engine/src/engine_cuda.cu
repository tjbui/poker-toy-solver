/* engine_cuda.cu -- exact equity by enumeration on the GPU */

#include "engine.h"
#include "hand_eval.h"
#include "simulation.h"

#include <cuda_runtime.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

/*
 * Same work space as the CPU engines: the flat index range [0, total) over
 * (matchup x board), where board b of matchup m is the b-th lexicographic
 * k-subset of the cards not used by that matchup's holes or the known board.
 * The GPU enumerates the same set of boards with the same evaluator, so the
 * counts match the CPU exactly.
 */

#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err_ = (call);                                             \
        if (err_ != cudaSuccess) {                                             \
            throw std::runtime_error(std::string("CUDA error: ") +             \
                                     cudaGetErrorString(err_) + " (" +         \
                                     __FILE__ + ":" + std::to_string(__LINE__) + ")"); \
        }                                                                      \
    } while (0)

namespace {

using ull = unsigned long long;

constexpr int kMaxN = 52;
constexpr int kMaxK = 5;
constexpr ull kDeckMask = (1ULL << 52) - 1;

static_assert(sizeof(Matchup) == 4, "Matchup must stay 4 packed bytes for the upload");

// C(n, k) lookup for unranking, and the known board cards.
__constant__ ull c_choose[kMaxN + 1][kMaxK + 1];
__constant__ Card c_known[5];

struct Work {
    const Matchup* matchups;
    ull total;        // matchups * boards
    ull boards;       // boards per matchup
    int known_count;  // known board cards
    int k;            // unknown board cards to draw
    ull* counts;      // [hero_wins, villain_wins, ties]
};

// ---- RAII wrappers so device resources are freed even if a CUDA call throws ----

template <typename T>
struct DeviceBuffer {
    T* ptr = nullptr;
    explicit DeviceBuffer(std::size_t n) { CUDA_CHECK(cudaMalloc(&ptr, n * sizeof(T))); }
    ~DeviceBuffer() { cudaFree(ptr); }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};

template <typename T>
struct PinnedBuffer {
    T* ptr = nullptr;
    explicit PinnedBuffer(std::size_t n) { CUDA_CHECK(cudaMallocHost(&ptr, n * sizeof(T))); }
    ~PinnedBuffer() { cudaFreeHost(ptr); }
    PinnedBuffer(const PinnedBuffer&) = delete;
    PinnedBuffer& operator=(const PinnedBuffer&) = delete;
};

struct CudaEvent {
    cudaEvent_t e{};
    CudaEvent() { CUDA_CHECK(cudaEventCreate(&e)); }
    ~CudaEvent() { cudaEventDestroy(e); }
    CudaEvent(const CudaEvent&) = delete;
    CudaEvent& operator=(const CudaEvent&) = delete;
};

struct CudaStream {
    cudaStream_t s{};
    CudaStream() { CUDA_CHECK(cudaStreamCreate(&s)); }
    ~CudaStream() { cudaStreamDestroy(s); }
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
};

// ---- device helpers ----

__device__ inline ull matchup_mask(const Matchup& mu) {
    return (1ULL << mu.h1) | (1ULL << mu.h2) | (1ULL << mu.v1) | (1ULL << mu.v2);
}

/*
 * Grid-stride helper: write the r-th lexicographic k-subset of the cards NOT in
 * `used` straight into out[]. Walks the available cards with find-first-set
 * instead of building a deck array, so nothing touches local memory.
 */
__device__ inline void unrank_cards(ull r, ull used, int n, int k, Card* out) {
    ull avail = ~used & kDeckMask;
    int pos = 0;                                 // position among available cards
    for (int i = 0; i < k; i++) {
        while (true) {
            int c = __ffsll(static_cast<long long>(avail)) - 1;
            avail &= avail - 1;                  // consume the lowest available card
            ull cnt = c_choose[n - 1 - pos][k - 1 - i];
            pos++;
            if (r < cnt) {
                out[i] = static_cast<Card>(c);
                break;
            }
            r -= cnt;
        }
    }
}

// Chunked helper: r-th lexicographic k-subset of positions {0..n-1}.
__device__ inline void unrank_positions(ull r, int n, int k, int* combo) {
    int x = 0;
    for (int i = 0; i < k; i++) {
        while (true) {
            ull c = c_choose[n - 1 - x][k - 1 - i];
            if (r < c) break;
            r -= c;
            x++;
        }
        combo[i] = x;
        x++;
    }
}

__device__ inline void next_combination(int* combo, int n, int k) {
    int i = k - 1;
    while (i >= 0 && combo[i] == n - k + i) i--;
    if (i < 0) return;
    combo[i]++;
    for (int j = i + 1; j < k; j++) combo[j] = combo[j - 1] + 1;
}

__device__ inline ull warp_sum(ull v) {
    for (int offset = 16; offset > 0; offset >>= 1) {
        v += __shfl_down_sync(0xffffffffu, v, offset);
    }
    return v;
}

/*
 * Add this thread's counts into the global totals.
 * BlockReduce: warp shuffle -> shared memory -> one atomicAdd per counter per block.
 * Otherwise:   one atomicAdd per counter per thread (baseline for the benchmark).
 * Every thread of the block must call this (it contains __syncthreads()).
 */
template <bool BlockReduce>
__device__ inline void add_counts(ull hw, ull vw, ull ties, ull* out) {
    if (!BlockReduce) {
        atomicAdd(&out[0], hw);
        atomicAdd(&out[1], vw);
        atomicAdd(&out[2], ties);
        return;
    }

    __shared__ ull partial[3][32];
    const int lane = threadIdx.x & 31;
    const int warp = threadIdx.x >> 5;

    hw = warp_sum(hw);
    vw = warp_sum(vw);
    ties = warp_sum(ties);
    if (lane == 0) {
        partial[0][warp] = hw;
        partial[1][warp] = vw;
        partial[2][warp] = ties;
    }
    __syncthreads();

    if (warp == 0) {
        const int nwarps = (blockDim.x + 31) / 32;
        hw = lane < nwarps ? partial[0][lane] : 0;
        vw = lane < nwarps ? partial[1][lane] : 0;
        ties = lane < nwarps ? partial[2][lane] : 0;
        hw = warp_sum(hw);
        vw = warp_sum(vw);
        ties = warp_sum(ties);
        if (lane == 0) {
            atomicAdd(&out[0], hw);
            atomicAdd(&out[1], vw);
            atomicAdd(&out[2], ties);
        }
    }
}

// ---- kernels ----

/*
 * Grid-stride: thread t handles indices t, t + stride, t + 2*stride, ...
 * Neighbouring threads work on neighbouring indices (same matchup, adjacent
 * boards), but consecutive iterations of one thread are far apart, so every
 * index is unranked from scratch.
 */
template <bool BlockReduce>
__global__ void kernel_grid_stride(Work w) {
    Card board[5];
    ull known_mask = 0;
    for (int j = 0; j < w.known_count; j++) {
        board[j] = c_known[j];
        known_mask |= 1ULL << c_known[j];
    }
    const int n = 48 - w.known_count;

    ull hw = 0, vw = 0, ties = 0;
    const ull stride = static_cast<ull>(gridDim.x) * blockDim.x;
    for (ull i = static_cast<ull>(blockIdx.x) * blockDim.x + threadIdx.x; i < w.total; i += stride) {
        const ull m = i / w.boards;
        const ull b = i - m * w.boards;
        const Matchup mu = w.matchups[m];
        unrank_cards(b, known_mask | matchup_mask(mu), n, w.k, board + w.known_count);

        const int r = hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board);
        hw += (r > 0);
        vw += (r < 0);
        ties += (r == 0);
    }

    add_counts<BlockReduce>(hw, vw, ties, w.counts);
}

/*
 * Chunked: thread t owns the contiguous range [t*chunk, (t+1)*chunk). It
 * unranks once per matchup it touches, then steps with next_combination --
 * the same scheme the CPU count_slice() uses per thread.
 */
template <bool BlockReduce>
__global__ void kernel_chunked(Work w, ull chunk) {
    Card board[5];
    ull known_mask = 0;
    for (int j = 0; j < w.known_count; j++) {
        board[j] = c_known[j];
        known_mask |= 1ULL << c_known[j];
    }

    ull hw = 0, vw = 0, ties = 0;
    const ull tid = static_cast<ull>(blockIdx.x) * blockDim.x + threadIdx.x;
    const ull lo = tid * chunk;
    const ull hi = lo + chunk < w.total ? lo + chunk : w.total;

    ull i = lo;
    while (i < hi) {
        const ull m = i / w.boards;
        const ull b0 = i - m * w.boards;
        const ull b_end = (w.boards < b0 + (hi - i)) ? w.boards : b0 + (hi - i);
        const Matchup mu = w.matchups[m];

        Card deck[48];
        int deck_n = 0;
        ull avail = ~(known_mask | matchup_mask(mu)) & kDeckMask;
        while (avail) {
            deck[deck_n++] = static_cast<Card>(__ffsll(static_cast<long long>(avail)) - 1);
            avail &= avail - 1;
        }

        int combo[5];
        unrank_positions(b0, deck_n, w.k, combo);
        for (ull b = b0; b < b_end; b++) {
            for (int j = 0; j < w.k; j++) board[w.known_count + j] = deck[combo[j]];
            const int r = hand_eval::compare(mu.h1, mu.h2, mu.v1, mu.v2, board);
            hw += (r > 0);
            vw += (r < 0);
            ties += (r == 0);
            next_combination(combo, deck_n, w.k);
        }
        i = m * w.boards + b_end;
    }

    add_counts<BlockReduce>(hw, vw, ties, w.counts);   // every thread reaches this
}

// Launch config: the block size / grid size that maximizes occupancy for this kernel.
template <typename Kernel>
void pick_launch(Kernel kernel, ull total, int& blocks, int& threads) {
    int min_grid = 0;
    CUDA_CHECK(cudaOccupancyMaxPotentialBlockSize(&min_grid, &threads, kernel, 0, 0));
    ull needed = (total + threads - 1) / threads;   // don't launch more blocks than there is work
    blocks = static_cast<int>(std::min<ull>(static_cast<ull>(min_grid), needed));
    if (blocks < 1) blocks = 1;
}

template <bool BlockReduce>
void launch(const CudaOptions& opt, const Work& w, CudaStats& st, cudaStream_t stream) {
    int blocks = 0, threads = 0;
    if (opt.kernel == CudaKernel::GridStride) {
        pick_launch(kernel_grid_stride<BlockReduce>, w.total, blocks, threads);
        kernel_grid_stride<BlockReduce><<<blocks, threads, 0, stream>>>(w);
    } else {
        pick_launch(kernel_chunked<BlockReduce>, w.total, blocks, threads);
        const ull nthreads = static_cast<ull>(blocks) * threads;
        const ull chunk = (w.total + nthreads - 1) / nthreads;
        kernel_chunked<BlockReduce><<<blocks, threads, 0, stream>>>(w, chunk);
    }
    CUDA_CHECK(cudaGetLastError());
    st.blocks = blocks;
    st.threadsPerBlock = threads;
}

} // namespace

bool cuda_available() {
    int count = 0;
    return cudaGetDeviceCount(&count) == cudaSuccess && count > 0;
}

EquityResult get_equity_cuda(const EquityRequest& request, const CudaOptions& options,
                             CudaStats* stats) {
    auto start = std::chrono::high_resolution_clock::now();
    CudaStats st;

    std::array<std::optional<Card>, 5> known_community;
    std::vector<Matchup> matchups = prepare_matchups(request, known_community);
    const ull total = total_runouts(matchups, known_community);

    // Constant memory: C(n, k) table and the known board.
    ull choose[kMaxN + 1][kMaxK + 1];
    for (int n = 0; n <= kMaxN; n++) {
        for (int k = 0; k <= kMaxK; k++) choose[n][k] = n_choose_k(n, k);
    }
    CUDA_CHECK(cudaMemcpyToSymbol(c_choose, choose, sizeof(choose)));

    Card known[5] = {0, 0, 0, 0, 0};
    int known_count = 0;
    for (const auto& c : known_community) {
        if (c) known[known_count++] = *c;
    }
    CUDA_CHECK(cudaMemcpyToSymbol(c_known, known, sizeof(known)));

    CudaStream stream;
    DeviceBuffer<Matchup> d_matchups(matchups.size());
    DeviceBuffer<ull> d_counts(3);
    CUDA_CHECK(cudaMemsetAsync(d_counts.ptr, 0, 3 * sizeof(ull), stream.s));

    // Upload the matchup list: pageable cudaMemcpy, or staged through pinned memory.
    const std::size_t bytes = matchups.size() * sizeof(Matchup);
    CudaEvent up_start, up_stop, k_start, k_stop;
    if (options.pinnedUpload) {
        auto prep0 = std::chrono::high_resolution_clock::now();
        PinnedBuffer<Matchup> staging(matchups.size());
        std::memcpy(staging.ptr, matchups.data(), bytes);
        auto prep1 = std::chrono::high_resolution_clock::now();
        st.pinnedPrepMs = std::chrono::duration<double, std::milli>(prep1 - prep0).count();

        CUDA_CHECK(cudaEventRecord(up_start.e, stream.s));
        CUDA_CHECK(cudaMemcpyAsync(d_matchups.ptr, staging.ptr, bytes, cudaMemcpyHostToDevice, stream.s));
        CUDA_CHECK(cudaEventRecord(up_stop.e, stream.s));
        CUDA_CHECK(cudaStreamSynchronize(stream.s));   // staging must outlive the copy
    } else {
        CUDA_CHECK(cudaEventRecord(up_start.e, stream.s));
        CUDA_CHECK(cudaMemcpyAsync(d_matchups.ptr, matchups.data(), bytes, cudaMemcpyHostToDevice, stream.s));
        CUDA_CHECK(cudaEventRecord(up_stop.e, stream.s));
    }

    Work w{};
    w.matchups = d_matchups.ptr;
    w.total = total;
    w.boards = n_choose_k(48 - known_count, 5 - known_count);
    w.known_count = known_count;
    w.k = 5 - known_count;
    w.counts = d_counts.ptr;

    CUDA_CHECK(cudaEventRecord(k_start.e, stream.s));
    if (options.blockReduce) {
        launch<true>(options, w, st, stream.s);
    } else {
        launch<false>(options, w, st, stream.s);
    }
    CUDA_CHECK(cudaEventRecord(k_stop.e, stream.s));

    ull counts[3] = {0, 0, 0};
    CUDA_CHECK(cudaMemcpyAsync(counts, d_counts.ptr, sizeof(counts), cudaMemcpyDeviceToHost, stream.s));
    CUDA_CHECK(cudaStreamSynchronize(stream.s));

    float up_ms = 0.0f, k_ms = 0.0f;
    CUDA_CHECK(cudaEventElapsedTime(&up_ms, up_start.e, up_stop.e));
    CUDA_CHECK(cudaEventElapsedTime(&k_ms, k_start.e, k_stop.e));
    st.uploadMs = up_ms;
    st.kernelMs = k_ms;

    if (counts[0] + counts[1] + counts[2] != total) {
        throw std::runtime_error("get_equity_cuda: kernel counted " +
                                 std::to_string(counts[0] + counts[1] + counts[2]) +
                                 " runouts, expected " + std::to_string(total));
    }

    EquityCounts ec;
    ec.hero_wins = counts[0];
    ec.villain_wins = counts[1];
    ec.ties = counts[2];
    ec.runouts = total;

    auto end = std::chrono::high_resolution_clock::now();
    if (stats) *stats = st;
    return finalize_result(ec, std::chrono::duration<double, std::milli>(end - start).count());
}
