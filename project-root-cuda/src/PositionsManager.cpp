#include "PositionsManager.hpp"

#include <cmath>
#include <iomanip>
#include <iostream>
#include <limits>

#include "CurrencyConfig.hpp"

PositionsManager::PositionsManager(const std::string& base, double capital)
    : baseCurrency(base), initialCapital(capital) {
    positions[baseCurrency] = initialCapital;
    portfolioHistory.push_back({0, positions, initialCapital});
}

void PositionsManager::executeArbitrageOpportunity(const TimeStampedArbitrage& opportunity,
                                                   const ForexGraph& graph) {
    const int64_t ts = opportunity.timestamp_ms;
    const auto& cycle = opportunity.opportunity.cycle;
    if (cycle.empty()) return;

    int startIndex = 0;
    std::string startCurrency;
    double available = 0.0;
    for (size_t i = 0; i < cycle.size(); ++i) {
        const std::string ccy = graph.getCurrencyName(cycle[i]);
        auto it = positions.find(ccy);
        if (it != positions.end() && it->second > 0.0) {
            startIndex = static_cast<int>(i);
            startCurrency = ccy;
            available = it->second;
            break;
        }
    }

    if (available <= 0.0) {
        startCurrency = graph.getCurrencyName(cycle[0]);
        const double conversionAmount = positions[baseCurrency] * 0.05;
        double conversionRate = 0.0;
        try {
            conversionRate = graph.rate(graph.getCurrencyId(baseCurrency),
                                        graph.getCurrencyId(startCurrency));
        } catch (...) {
            conversionRate = 0.0;
        }
        if (conversionRate <= 0.0) return;
        const double newAmount = conversionAmount * conversionRate;
        positions[baseCurrency] -= conversionAmount;
        positions[startCurrency] += newAmount;
        available = newAmount;
        tradeHistory.push_back({ts, baseCurrency, startCurrency, conversionAmount, newAmount, conversionRate});
    }

    double current = available * 0.5;
    const int n = static_cast<int>(cycle.size());
    for (int i = 0; i < n; ++i) {
        const int from = cycle[(startIndex + i) % n];
        const int to = cycle[(startIndex + i + 1) % n];
        const std::string fromCcy = graph.getCurrencyName(from);
        const std::string toCcy = graph.getCurrencyName(to);
        const double rate = graph.rate(from, to);
        if (rate <= 0.0) break;
        const double nextAmt = current * rate;
        tradeHistory.push_back({ts, fromCcy, toCcy, current, nextAmt, rate});
        positions[fromCcy] -= current;
        positions[toCcy] += nextAmt;
        current = nextAmt;
    }
    updatePortfolioSnapshot(ts, graph);
}

void PositionsManager::updatePortfolioSnapshot(int64_t timestamp_ms, const ForexGraph& graph) {
    PortfolioSnapshot snap{timestamp_ms, positions, 0.0};
    double totalUSD = 0.0;
    for (const auto& [ccy, amount] : positions) {
        if (ccy == "USD") {
            totalUSD += amount;
            continue;
        }
        double usd = 0.0;
        try {
            usd = graph.rate(graph.getCurrencyId(ccy), graph.getCurrencyId("USD"));
        } catch (...) {
            usd = 0.0;
        }
        totalUSD += amount * (usd > 0.0 ? usd : 1.0);
    }
    snap.totalValueUSD = totalUSD;
    portfolioHistory.push_back(std::move(snap));
}

void PositionsManager::printCurrentPositions() const {
    std::cout << "\n===== CURRENT POSITIONS =====\n";
    for (const auto& [ccy, amount] : positions) {
        std::cout << ccy << ": " << std::fixed << std::setprecision(6) << amount << "\n";
    }
}

void PositionsManager::printTradeHistory() const {
    std::cout << "\n===== TRADE HISTORY =====\n";
    for (const auto& t : tradeHistory) {
        std::cout << t.timestamp_ms << ": " << std::fixed << std::setprecision(6)
                  << t.fromAmount << " " << t.fromCurrency << " -> "
                  << t.toAmount << " " << t.toCurrency << " (rate " << t.rate << ")\n";
    }
}

void PositionsManager::printPortfolioHistory() const {
    std::cout << "\n===== PORTFOLIO HISTORY =====\n";
    for (const auto& s : portfolioHistory) {
        std::cout << s.timestamp_ms << ": Total = $" << std::fixed << std::setprecision(2)
                  << s.totalValueUSD << "\n";
    }
}
