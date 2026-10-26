/* gpu_backend.cu -- exact equity by enumeration on the GPU: the host side */

#include "gpu_backend.h"

#include <cuda_runtime.h>

#include <algorithm>
#include <chrono>
#include <cstring>

#include "cuda_utils.cuh"
#include "kernels.cuh"

/*
 * Same work and same results as the CPU backends, split by layer:
 *   runouts.cuh     ENUMERATE  runout number -> cards (constant-memory C(n,k) table)
 *   matchup.cuh     CORE       one matchup: one runout or a slice -> showdown -> counts
 *   reduce.cuh      SUM        per-thread counts -> warp shuffle -> block -> atomicAdd
 *   kernels.cuh     SCHEDULE   which indices each thread does (grid-stride / chunked)
 *   cuda_utils.cuh  PLUMBING   CUDA_CHECK + RAII wrappers
 *   this file       HOST       upload, launch config, timing, copy the counts back
 * The showdown itself is the evaluator shared with the CPU (common/hand_eval.h).
 *
 * The .cuh files are header-only __device__ code included into this ONE .cu:
 * device functions can't be called across .cu files without relocatable device
 * code (-rdc), which would also block inlining.
 */

namespace equity {

namespace {

using gpu::ull;

static_assert(sizeof(Matchup) == 4, "Matchup must stay 4 packed bytes for the upload");

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
void launch(const GpuOptions& opt, const gpu::Work& w, GpuStats& st, cudaStream_t stream) {
    int blocks = 0, threads = 0;
    if (opt.kernel == GpuKernel::GridStride) {
        pick_launch(gpu::kernel_grid_stride<BlockReduce>, w.total, blocks, threads);
        gpu::kernel_grid_stride<BlockReduce><<<blocks, threads, 0, stream>>>(w);
    } else {
        pick_launch(gpu::kernel_chunked<BlockReduce>, w.total, blocks, threads);
        const ull nthreads = static_cast<ull>(blocks) * threads;
        const ull chunk = (w.total + nthreads - 1) / nthreads;
        gpu::kernel_chunked<BlockReduce><<<blocks, threads, 0, stream>>>(w, chunk);
    }
    CUDA_CHECK(cudaGetLastError());
    st.blocks = blocks;
    st.threadsPerBlock = threads;
}

} // namespace

bool gpu_device_available() {
    static const bool available = [] {
        int count = 0;
        return cudaGetDeviceCount(&count) == cudaSuccess && count > 0;
    }();
    return available;
}

EquityCounts gpu_count(const Workload& wl, const GpuOptions& options, GpuStats* stats) {
    GpuStats st;

    // Constant memory: C(n, k) table and the known board.
    gpu::upload_constants(wl.known.data());

    gpu::CudaStream stream;
    gpu::DeviceBuffer<Matchup> d_matchups(wl.matchups.size());
    gpu::DeviceBuffer<ull> d_counts(3);
    CUDA_CHECK(cudaMemsetAsync(d_counts.ptr, 0, 3 * sizeof(ull), stream.s));

    // Upload the matchup list: pageable cudaMemcpy, or staged through pinned memory.
    const std::size_t bytes = wl.matchups.size() * sizeof(Matchup);
    gpu::CudaEvent up_start, up_stop, k_start, k_stop;
    if (options.pinnedUpload) {
        auto prep0 = std::chrono::steady_clock::now();
        gpu::PinnedBuffer<Matchup> staging(wl.matchups.size());
        std::memcpy(staging.ptr, wl.matchups.data(), bytes);
        auto prep1 = std::chrono::steady_clock::now();
        st.pinnedPrepMs = std::chrono::duration<double, std::milli>(prep1 - prep0).count();

        CUDA_CHECK(cudaEventRecord(up_start.e, stream.s));
        CUDA_CHECK(cudaMemcpyAsync(d_matchups.ptr, staging.ptr, bytes, cudaMemcpyHostToDevice, stream.s));
        CUDA_CHECK(cudaEventRecord(up_stop.e, stream.s));
        CUDA_CHECK(cudaStreamSynchronize(stream.s));   // staging must outlive the copy
    } else {
        CUDA_CHECK(cudaEventRecord(up_start.e, stream.s));
        CUDA_CHECK(cudaMemcpyAsync(d_matchups.ptr, wl.matchups.data(), bytes, cudaMemcpyHostToDevice, stream.s));
        CUDA_CHECK(cudaEventRecord(up_stop.e, stream.s));
    }

    gpu::Work w{};
    w.matchups = d_matchups.ptr;
    w.total = wl.total;
    w.runouts_per_matchup = wl.runoutsPerMatchup;
    w.known_count = wl.knownCount;
    w.k = wl.k;
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
    if (stats) *stats = st;

    EquityCounts result;
    result.heroWins = counts[0];
    result.villainWins = counts[1];
    result.ties = counts[2];
    return result;
}

} // namespace equity
