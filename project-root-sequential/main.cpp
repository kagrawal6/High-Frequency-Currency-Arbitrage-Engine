#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include "CurrencyConfig.hpp"
#include "MarketData.hpp"
#include "PositionsManager.hpp"
#include "TimeSeriesArbitrageDetector.hpp"
#include "Timer.hpp"

struct Options {
    int maxTimestamps = 0;
    bool verbose = false;
    bool trade = true;
    double capital = 1000.0;
};

static Options parseArgs(int argc, char** argv) {
    Options opt;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--verbose") opt.verbose = true;
        else if (a == "--no-trade") opt.trade = false;
        else if (a == "--max-ts" && i + 1 < argc) opt.maxTimestamps = std::atoi(argv[++i]);
        else if (a == "--capital" && i + 1 < argc) opt.capital = std::atof(argv[++i]);
        else if (a == "--help") {
            std::cout << "Usage: ./arbitrage [--max-ts N] [--verbose] [--no-trade] [--capital X]\n";
            std::exit(0);
        }
    }
    return opt;
}

int main(int argc, char** argv) {
    const Options opt = parseArgs(argc, argv);

    std::cout << "========== FOREX ARBITRAGE DETECTOR (SEQUENTIAL) ==========\n\n";
    std::cout << "Currencies:";
    for (int i = 0; i < kNumCurrencies; ++i) std::cout << " " << kCurrencies[i];
    std::cout << "\nPairs: " << kNumPairs << "\n\n";

    MarketData market = loadMarketData(".", opt.maxTimestamps);
    std::cout << "Loaded " << market.nTimestamps << " aligned timestamps across "
              << market.nPairs << " pairs.\n";

    std::cout << "\n====== SINGLE TIMESTAMP ANALYSIS ======\n";
    if (market.nTimestamps > 0) {
        Timer tDetect("Single-timestamp detection");
        ForexGraph g0 = market.graphAt(0);
        auto one = detectArbitrage(g0);
        tDetect.stop();
        std::cout << "t0 currencies=" << g0.getVertexCount()
                  << " edges=" << g0.getEdges().size() << "\n";
        if (one.cycle.empty()) std::cout << "No arbitrage at first timestamp.\n";
        else {
            std::cout << "First-timestamp profit " << std::fixed << std::setprecision(6)
                      << one.profit << "%\n";
        }
    }

    std::cout << "\n====== TIME SERIES ANALYSIS ======\n";
    TimeSeriesArbitrageDetector detector(std::move(market));
    detector.analyzeAllTimestamps(opt.verbose);
    detector.printAllOpportunities();

    if (opt.trade && !detector.getOpportunities().empty()) {
        std::cout << "\n====== TRADING SIMULATION ======\n";
        PositionsManager pm("USD", opt.capital);
        Timer tTrade("Trade execution");
        for (const auto& opp : detector.getOpportunities()) {
            ForexGraph g = detector.getGraphForIndex(opp.timestamp_index);
            pm.executeArbitrageOpportunity(opp, g);
        }
        tTrade.stop();
        pm.printCurrentPositions();
        if (opt.verbose) {
            pm.printTradeHistory();
            pm.printPortfolioHistory();
        } else {
            const auto& hist = pm.getPortfolioHistory();
            std::cout << "Trades executed: " << pm.getTradeHistory().size() << "\n";
            std::cout << "Final mark-to-USD: $" << std::fixed << std::setprecision(2)
                      << hist.back().totalValueUSD << "\n";
        }
    }

    Timer::report();
    return 0;
}
