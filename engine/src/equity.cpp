/* equity.cpp -- public entry point: prepare the work, pick a backend, build the result */

#include "equity/equity.h"

#include <chrono>
#include <stdexcept>
#include <string>

#include "common/workload.h"
#include "cpu/cpu_backend.h"
#include "gpu/gpu_backend.h"

namespace equity {

namespace {

Backend choose_backend(const Workload& w, const EquityOptions& options) {
    if (options.backend != Backend::Auto) {
        return options.backend;
    }
    // Check the size first: small jobs never touch the CUDA runtime at all.
    if (w.total >= options.autoGpuMinRunouts && gpu_device_available()) {
        return Backend::Gpu;
    }
    return Backend::CpuMulti;
}

EquityCounts run_backend(Backend backend, const Workload& w, const EquityOptions& options) {
    switch (backend) {
        case Backend::CpuSingle:
            return cpu_count_single(w);
        case Backend::CpuMulti:
            return cpu_count_multi(w, options.cpuThreads);
        case Backend::Gpu:
            if (!gpu_device_available()) {
                throw std::runtime_error(
                    "GPU backend requested but unavailable "
                    "(engine built without CUDA, or no CUDA device found)");
            }
            return gpu_count(w, options.gpuStats);
        case Backend::Auto:
            break;
    }
    throw std::logic_error("run_backend: unresolved backend");
}

} // namespace

const char* backend_name(Backend backend) {
    switch (backend) {
        case Backend::Auto:      return "auto";
        case Backend::CpuSingle: return "cpu-single";
        case Backend::CpuMulti:  return "cpu-multi";
        case Backend::Gpu:       return "gpu";
    }
    return "unknown";
}

bool gpu_available() {
    return gpu_device_available();
}

EquityResult compute_equity(const EquityRequest& request, const EquityOptions& options) {
    const auto start = std::chrono::steady_clock::now();

    const Workload w = prepare_workload(request);
    const Backend backend = choose_backend(w, options);
    const EquityCounts counts = run_backend(backend, w, options);

    // Every backend must account for every runout exactly once.
    if (counts.total() != w.total) {
        throw std::runtime_error(std::string("compute_equity: ") + backend_name(backend) +
                                 " counted " + std::to_string(counts.total()) +
                                 " runouts, expected " + std::to_string(w.total));
    }

    EquityResult result;
    result.backend = backend;
    result.evaluatedRunouts = w.total;
    const double total = static_cast<double>(w.total);
    result.heroWinPct = 100.0 * static_cast<double>(counts.heroWins) / total;
    result.villainWinPct = 100.0 * static_cast<double>(counts.villainWins) / total;
    result.tiePct = 100.0 * static_cast<double>(counts.ties) / total;

    const auto end = std::chrono::steady_clock::now();
    result.runtimeMs = std::chrono::duration<double, std::milli>(end - start).count();
    return result;
}

} // namespace equity
