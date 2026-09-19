#include "TimeSeriesArbitrageDetector.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

#include "Timer.hpp"

TimeSeriesArbitrageDetector::TimeSeriesArbitrageDetector(MarketData market)
    : market_(std::move(market)) {}

void TimeSeriesArbitrageDetector::analyzeAllTimestamps(bool verbose) {
    Timer tAnalyze("Time-series detection");
    opportunities.clear();
    opportunities.reserve(256);
    const int T = market_.nTimestamps;
    for (int t = 0; t < T; ++t) {
        if (verbose && (t % std::max(1, T / 10) == 0)) {
            std::cout << "  [" << t << "/" << T << "] ts=" << market_.timestamps[t] << "\n";
        }
        auto arb = detectArbitrageAt(market_, t);
        if (!arb.cycle.empty() && arb.profit > 0.0) {
            opportunities.push_back({market_.timestamps[t], t, std::move(arb)});
        }
    }
    tAnalyze.stop();
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
    if (it == market_.timestamps.end() || *it != timestamp_ms) {
        return ForexGraph();
    }
    return market_.graphAt(static_cast<int>(it - market_.timestamps.begin()));
}
