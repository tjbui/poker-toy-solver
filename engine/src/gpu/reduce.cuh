#pragma once

/*
 * reduce.cuh -- SUM: add every thread's private counts into the 3 global totals.
 *
 * Many threads doing `out[0] += x` at once would lose updates (read-add-write
 * race), so the final adds are atomicAdd. To keep atomics few:
 *   1. warp_sum:  the 32 threads of a warp combine in registers (warp shuffle)
 *   2. lane 0 of each warp writes its warp's sum to shared memory
 *   3. warp 0 sums those, and ONE thread per block does the atomicAdds
 */

#include "matchup.cuh"

namespace equity::gpu {

// Sum v over the 32 threads of a warp; lane 0 ends up with the total.
// Each step adds the value from the thread `offset` lanes higher: 16, 8, 4, 2, 1.
__device__ inline ull warp_sum(ull v) {
    for (int offset = 16; offset > 0; offset >>= 1) {
        v += __shfl_down_sync(0xffffffffu, v, offset);
    }
    return v;
}

/*
 * Add this thread's counts into out[0..2] = {hero_wins, villain_wins, ties}:
 * warp shuffle -> shared memory -> one atomicAdd per counter per block.
 * Every thread of the block must call this (it contains __syncthreads()).
 */
__device__ inline void add_counts(DeviceCounts c, ull* out) {
    __shared__ ull partial[3][32];               // one slot per warp (max 1024 threads = 32 warps)
    const int lane = threadIdx.x & 31;           // my index within my warp
    const int warp = threadIdx.x >> 5;           // my warp within the block

    c.hero_wins = warp_sum(c.hero_wins);
    c.villain_wins = warp_sum(c.villain_wins);
    c.ties = warp_sum(c.ties);
    if (lane == 0) {
        partial[0][warp] = c.hero_wins;
        partial[1][warp] = c.villain_wins;
        partial[2][warp] = c.ties;
    }
    __syncthreads();                             // wait until every warp has written

    if (warp == 0) {
        const int nwarps = (blockDim.x + 31) / 32;
        ull hw = lane < nwarps ? partial[0][lane] : 0;
        ull vw = lane < nwarps ? partial[1][lane] : 0;
        ull ties = lane < nwarps ? partial[2][lane] : 0;
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

} // namespace equity::gpu
