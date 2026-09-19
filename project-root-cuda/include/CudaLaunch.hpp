#pragma once

#include "CurrencyConfig.hpp"
#include "MarketData.hpp"
#include "ArbitrageDetector.hpp"

#include <vector>

struct DeviceOpportunity {
    int timestamp_index;
    float profit;
    int cycle_len;
    int cycle[8];
};

struct PackedWeights {
    int nTimestamps = 0;
    int nEdges = 0;
    int nCurrencies = kNumCurrencies;
    std::vector<int> src;
    std::vector<int> dest;
    std::vector<double> weights;  // [t * nEdges + e], INF if missing
};

PackedWeights packMarketWeights(const MarketData& market);

/// Launch the CUDA kernel (or the host emulator of that same kernel).
/// blockSize is the CUDA block dimension; ignored by the emulator except for occupancy math.
std::vector<DeviceOpportunity> launchDetectKernel(const PackedWeights& packed, int blockSize = 128);

const char* cudaBackendName();
