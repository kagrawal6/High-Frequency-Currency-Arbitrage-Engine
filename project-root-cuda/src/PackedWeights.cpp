#include "CudaLaunch.hpp"
#include "ArbitrageKernel.cuh"

#include <cmath>

PackedWeights packMarketWeights(const MarketData& market) {
    PackedWeights packed;
    packed.nTimestamps = market.nTimestamps;
    packed.nEdges = market.nPairs * 2;
    packed.nCurrencies = market.nCurrencies;
    packed.src.resize(static_cast<size_t>(packed.nEdges));
    packed.dest.resize(static_cast<size_t>(packed.nEdges));
    packed.weights.assign(static_cast<size_t>(packed.nTimestamps) * packed.nEdges, fx::kInf);

    for (int p = 0; p < market.nPairs; ++p) {
        packed.src[p * 2] = market.pairBase[p];
        packed.dest[p * 2] = market.pairQuote[p];
        packed.src[p * 2 + 1] = market.pairQuote[p];
        packed.dest[p * 2 + 1] = market.pairBase[p];
    }

    for (int t = 0; t < market.nTimestamps; ++t) {
        for (int p = 0; p < market.nPairs; ++p) {
            const size_t base = static_cast<size_t>(t) * packed.nEdges + p * 2;
            if (!market.hasPair(t, p)) continue;
            const double bid = market.bidAt(t, p);
            const double ask = market.askAt(t, p);
            if (bid <= 0.0 || ask <= 0.0) continue;
            packed.weights[base] = -std::log(bid);
            packed.weights[base + 1] = -std::log(1.0 / ask);
        }
    }
    return packed;
}
