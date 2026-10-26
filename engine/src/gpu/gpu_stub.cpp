/* gpu_stub.cpp -- GPU backend placeholder for builds without a CUDA compiler */

#include "gpu_backend.h"

#include <stdexcept>

namespace equity {

bool gpu_device_available() {
    return false;
}

EquityCounts gpu_count(const Workload&, GpuStats*) {
    throw std::runtime_error(
        "GPU backend unavailable: this build has no CUDA support "
        "(no CUDA compiler was found when CMake configured the project)");
}

} // namespace equity
