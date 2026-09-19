#pragma once

#include <vector>

#include "ForexGraph.hpp"
#include "MarketData.hpp"

struct ArbitrageOpportunity {
    std::vector<int> cycle;
    double profit;  // percent, e.g. 0.12 means 0.12%
};

/// Single dummy-source Bellman-Ford on a live edge list. O(V*E), not O(V^2*E).
ArbitrageOpportunity detectArbitrage(const ForexGraph& graph);

/// Hot path: detect on packed market row `t` without building a ForexGraph.
ArbitrageOpportunity detectArbitrageAt(const MarketData& market, int t);
