#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "CurrencyConfig.hpp"
#include "ForexGraph.hpp"

/// Packed, timestamp-aligned FX panel. Layout is SoA: bid/ask/present are T*P.
struct MarketData {
    int nCurrencies = kNumCurrencies;
    int nPairs = kNumPairs;
    int nTimestamps = 0;
    std::vector<int64_t> timestamps;
    std::vector<double> bid;      // [t * nPairs + p]
    std::vector<double> ask;
    std::vector<uint8_t> present;

    int pairBase[kNumPairs]{};
    int pairQuote[kNumPairs]{};

    double bidAt(int t, int p) const { return bid[static_cast<size_t>(t) * nPairs + p]; }
    double askAt(int t, int p) const { return ask[static_cast<size_t>(t) * nPairs + p]; }
    bool hasPair(int t, int p) const { return present[static_cast<size_t>(t) * nPairs + p] != 0; }

    ForexGraph graphAt(int t) const;
};

/// Load every pair CSV and align on the union of timestamps.
/// maxTimestamps <= 0 means "all".
MarketData loadMarketData(const std::string& dataRoot = ".", int maxTimestamps = 0);
