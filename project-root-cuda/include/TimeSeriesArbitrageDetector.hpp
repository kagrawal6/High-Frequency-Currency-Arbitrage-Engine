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
    explicit TimeSeriesArbitrageDetector(MarketData market);

    void analyzeAllTimestamps(bool verbose = false);
    void printAllOpportunities() const;

    const std::vector<TimeStampedArbitrage>& getOpportunities() const { return opportunities; }
    const MarketData& market() const { return market_; }
    ForexGraph getGraphForTimestamp(int64_t timestamp_ms) const;
    ForexGraph getGraphForIndex(int t) const { return market_.graphAt(t); }

private:
    MarketData market_;
    std::vector<TimeStampedArbitrage> opportunities;
};
