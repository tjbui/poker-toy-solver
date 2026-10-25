#pragma once

/*
 * gpu_backend.h -- CUDA backend.
 *
 * Implemented in gpu_backend.cu when the engine is built with a CUDA compiler,
 * otherwise by gpu_stub.cpp (no device, every call throws).
 */

#include "common/workload.h"

namespace equity {

// True if built with CUDA and at least one CUDA device is present (cached).
bool gpu_device_available();

// Count results for the whole work space on the GPU.
// Throws std::runtime_error on any CUDA error, or if built without CUDA.
EquityCounts gpu_count(const Workload& w, const GpuOptions& options, GpuStats* stats);

} // namespace equity
