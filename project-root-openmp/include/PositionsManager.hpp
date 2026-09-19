#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "TimeSeriesArbitrageDetector.hpp"

struct TradeExecution {
    int64_t timestamp_ms;
    std::string fromCurrency;
    std::string toCurrency;
    double fromAmount;
    double toAmount;
    double rate;
};

struct PortfolioSnapshot {
    int64_t timestamp_ms;
    std::unordered_map<std::string, double> positions;
    double totalValueUSD;
};

class PositionsManager {
public:
    PositionsManager(const std::string& base = "USD", double capital = 1000.0);

    void executeArbitrageOpportunity(const TimeStampedArbitrage& opportunity, const ForexGraph& graph);
    void printCurrentPositions() const;
    void printTradeHistory() const;
    void printPortfolioHistory() const;

    const std::vector<PortfolioSnapshot>& getPortfolioHistory() const { return portfolioHistory; }
    const std::vector<TradeExecution>& getTradeHistory() const { return tradeHistory; }

private:
    void updatePortfolioSnapshot(int64_t timestamp_ms, const ForexGraph& graph);

    std::unordered_map<std::string, double> positions;
    std::vector<TradeExecution> tradeHistory;
    std::vector<PortfolioSnapshot> portfolioHistory;
    std::string baseCurrency;
    double initialCapital;
};
