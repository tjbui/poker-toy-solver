#pragma once

/*
 * cuda_utils.cuh -- host-side CUDA plumbing: error checking + RAII wrappers.
 *
 * Every wrapper frees its resource in the destructor, so nothing leaks when a
 * CUDA_CHECK throws halfway through gpu_count(). Copying is deleted (two
 * owners would free the same pointer twice).
 */

#include <cuda_runtime.h>

#include <cstddef>
#include <stdexcept>
#include <string>

#define CUDA_CHECK(call)                                                       \
    do {                                                                       \
        cudaError_t err_ = (call);                                             \
        if (err_ != cudaSuccess) {                                             \
            throw std::runtime_error(std::string("CUDA error: ") +             \
                                     cudaGetErrorString(err_) + " (" +         \
                                     __FILE__ + ":" + std::to_string(__LINE__) + ")"); \
        }                                                                      \
    } while (0)

namespace equity::gpu {

// n elements of T in device (GPU) memory.
template <typename T>
struct DeviceBuffer {
    T* ptr = nullptr;
    explicit DeviceBuffer(std::size_t n) { CUDA_CHECK(cudaMalloc(&ptr, n * sizeof(T))); }
    ~DeviceBuffer() { cudaFree(ptr); }
    DeviceBuffer(const DeviceBuffer&) = delete;
    DeviceBuffer& operator=(const DeviceBuffer&) = delete;
};

// A timestamp recorded into a stream (for timing GPU work).
struct CudaEvent {
    cudaEvent_t e{};
    CudaEvent() { CUDA_CHECK(cudaEventCreate(&e)); }
    ~CudaEvent() { cudaEventDestroy(e); }
    CudaEvent(const CudaEvent&) = delete;
    CudaEvent& operator=(const CudaEvent&) = delete;
};

// A queue of GPU work (copies, kernels) that runs in order.
struct CudaStream {
    cudaStream_t s{};
    CudaStream() { CUDA_CHECK(cudaStreamCreate(&s)); }
    ~CudaStream() { cudaStreamDestroy(s); }
    CudaStream(const CudaStream&) = delete;
    CudaStream& operator=(const CudaStream&) = delete;
};

} // namespace equity::gpu
