#include "TimeSeriesArbitrageDetector.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>

#include <omp.h>

#include "Timer.hpp"

TimeSeriesArbitrageDetector::TimeSeriesArbitrageDetector(MarketData market, int thread_count)
    : market_(std::move(market)), thread_count_(thread_count < 1 ? 1 : thread_count) {}

void TimeSeriesArbitrageDetector::analyzeAllTimestamps(bool verbose) {
    Timer tAnalyze("Time-series detection");
    const int T = market_.nTimestamps;
    const int threads = thread_count_;
    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    std::vector<std::vector<TimeStampedArbitrage>> local(static_cast<size_t>(threads));

    // One timestamp per OpenMP iteration. The detector itself is serial per thread
    // so we never oversubscribe with nested teams.
    #pragma omp parallel num_threads(threads)
    {
        const int tid = omp_get_thread_num();
        local[tid].reserve(static_cast<size_t>(T) / threads + 8);

        #pragma omp for schedule(static)
        for (int t = 0; t < T; ++t) {
            if (verbose && (t % std::max(1, T / 10) == 0)) {
                #pragma omp critical
                std::cout << "  [" << t << "/" << T << "] ts=" << market_.timestamps[t] << "\n";
            }
            auto arb = detectArbitrageAt(market_, t);
            if (!arb.cycle.empty() && arb.profit > 0.0) {
                local[tid].push_back({market_.timestamps[t], t, std::move(arb)});
            }
        }
    }

    opportunities.clear();
    size_t total = 0;
    for (const auto& bucket : local) total += bucket.size();
    opportunities.reserve(total);
    for (auto& bucket : local) {
        opportunities.insert(opportunities.end(),
                             std::make_move_iterator(bucket.begin()),
                             std::make_move_iterator(bucket.end()));
    }
    std::sort(opportunities.begin(), opportunities.end(),
              [](const TimeStampedArbitrage& a, const TimeStampedArbitrage& b) {
                  return a.timestamp_ms < b.timestamp_ms;
              });
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
    if (it == market_.timestamps.end() || *it != timestamp_ms) return ForexGraph();
    return market_.graphAt(static_cast<int>(it - market_.timestamps.begin()));
}
