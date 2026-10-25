/* engine_cuda_stub.cpp -- CUDA backend placeholder for builds without nvcc */

#include "engine.h"

#include <stdexcept>

EquityResult get_equity_cuda(const EquityRequest&, const CudaOptions&, CudaStats*) {
    throw std::runtime_error(
        "get_equity_cuda: this build has no CUDA backend "
        "(no CUDA compiler was found when CMake configured the project)");
}

bool cuda_available() {
    return false;
}
