#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ArbitrageDetector.hpp"
#include "MarketData.hpp"

struct TimeStampedArbitrage {
    int64_t timestamp_ms;
    int timestamp_index;
    ArbitrageOpportunity opportunity;
};

class TimeSeriesArbitrageDetector {
public:
    TimeSeriesArbitrageDetector(MarketData market, int thread_count = 1);

    void analyzeAllTimestamps(bool verbose = false);
    void printAllOpportunities() const;

    const std::vector<TimeStampedArbitrage>& getOpportunities() const { return opportunities; }
    const MarketData& market() const { return market_; }
    ForexGraph getGraphForTimestamp(int64_t timestamp_ms) const;
    ForexGraph getGraphForIndex(int t) const { return market_.graphAt(t); }
    int threadCount() const { return thread_count_; }

private:
    MarketData market_;
    int thread_count_;
    std::vector<TimeStampedArbitrage> opportunities;
};
