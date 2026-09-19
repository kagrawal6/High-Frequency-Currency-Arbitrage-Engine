#include "CudaLaunch.hpp"
#include "ArbitrageKernel.cuh"

#ifdef _OPENMP
#include <omp.h>
#endif

#include <vector>

std::vector<DeviceOpportunity> launchDetectKernel(const PackedWeights& packed, int /*blockSize*/) {
    const int T = packed.nTimestamps;
    const int E = packed.nEdges;
    const int V = packed.nCurrencies;
    std::vector<double> profits(static_cast<size_t>(T), 0.0);
    std::vector<int> lens(static_cast<size_t>(T), 0);
    std::vector<int> cyc(static_cast<size_t>(T) * 8, -1);

    // Emulates the CUDA mapping: one work item per timestamp, independent registers.
#ifdef _OPENMP
    #pragma omp parallel for schedule(static)
#endif
    for (int t = 0; t < T; ++t) {
        fx::KernelHit hit = fx::detectOne(V, E, packed.src.data(), packed.dest.data(),
                                          packed.weights.data() + static_cast<size_t>(t) * E);
        profits[t] = hit.profit;
        lens[t] = hit.cycle_len;
        for (int i = 0; i < 8; ++i) cyc[static_cast<size_t>(t) * 8 + i] = hit.cycle[i];
    }

    std::vector<DeviceOpportunity> hits;
    hits.reserve(256);
    for (int t = 0; t < T; ++t) {
        if (lens[t] <= 0 || profits[t] <= 0.0) continue;
        DeviceOpportunity o;
        o.timestamp_index = t;
        o.profit = static_cast<float>(profits[t]);
        o.cycle_len = lens[t];
        for (int i = 0; i < 8; ++i) o.cycle[i] = cyc[static_cast<size_t>(t) * 8 + i];
        hits.push_back(o);
    }
    return hits;
}

const char* cudaBackendName() { return "cuda-emu"; }
