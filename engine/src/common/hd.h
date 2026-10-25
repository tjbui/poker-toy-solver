#pragma once

/*
 * hd.h -- mark a function as runnable on both CPU and GPU.
 *
 * Under nvcc, HD expands to __host__ __device__ so the function is compiled
 * for both sides. Under a plain C++ compiler it expands to nothing. Shared
 * code (hand_eval.h, combinatorics.h) must stay header-only and `inline`.
 */

#ifdef __CUDACC__
#define HD __host__ __device__
#else
#define HD
#endif
