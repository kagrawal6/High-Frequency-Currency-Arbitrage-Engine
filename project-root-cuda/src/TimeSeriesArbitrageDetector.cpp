#include "TimeSeriesArbitrageDetector.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "CudaLaunch.hpp"
#include "Timer.hpp"

TimeSeriesArbitrageDetector::TimeSeriesArbitrageDetector(MarketData market)
    : market_(std::move(market)) {}

void TimeSeriesArbitrageDetector::analyzeAllTimestamps(bool verbose) {
    Timer tPack("Weight packing");
    PackedWeights packed = packMarketWeights(market_);
    tPack.stop();

    if (verbose) {
        std::cout << "  CUDA backend: " << cudaBackendName()
                  << "  timestamps=" << packed.nTimestamps
                  << "  edges/ts=" << packed.nEdges << "\n";
    }

    Timer tKernel("CUDA kernel");
    auto deviceHits = launchDetectKernel(packed, 128);
    tKernel.stop();

    opportunities.clear();
    opportunities.reserve(deviceHits.size());
    for (const auto& h : deviceHits) {
        ArbitrageOpportunity opp;
        opp.profit = h.profit;
        opp.cycle.assign(h.cycle, h.cycle + h.cycle_len);
        opportunities.push_back({market_.timestamps[h.timestamp_index], h.timestamp_index, std::move(opp)});
    }
    std::sort(opportunities.begin(), opportunities.end(),
              [](const TimeStampedArbitrage& a, const TimeStampedArbitrage& b) {
                  return a.timestamp_ms < b.timestamp_ms;
              });
}

void TimeSeriesArbitrageDetector::printAllOpportunities() const {
    std::cout << "\n===== ARBITRAGE OPPORTUNITIES (" << opportunities.size() << ") =====\n";
    for (const auto& tsArb : opportunities) {
        std::cout << "ts=" << tsArb.timestamp_ms << " profit=" << std::fixed << std::setprecision(6)
                  << tsArb.opportunity.profit << "%  cycle=";
        for (size_t i = 0; i < tsArb.opportunity.cycle.size(); ++i) {
            if (i) std::cout << "->";
            std::cout << currencyName(tsArb.opportunity.cycle[i]);
        }
        if (!tsArb.opportunity.cycle.empty()) {
            std::cout << "->" << currencyName(tsArb.opportunity.cycle.front());
        }
        std::cout << "\n";
    }
}

ForexGraph TimeSeriesArbitrageDetector::getGraphForTimestamp(int64_t timestamp_ms) const {
    auto it = std::lower_bound(market_.timestamps.begin(), market_.timestamps.end(), timestamp_ms);
    if (it == market_.timestamps.end() || *it != timestamp_ms) return ForexGraph();
    return market_.graphAt(static_cast<int>(it - market_.timestamps.begin()));
}
