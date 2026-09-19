#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string>

#include <omp.h>

#include "CurrencyConfig.hpp"
#include "MarketData.hpp"
#include "PositionsManager.hpp"
#include "TimeSeriesArbitrageDetector.hpp"
#include "Timer.hpp"

struct Options {
    int threads = 4;
    int maxTimestamps = 0;
    bool verbose = false;
    bool trade = true;
    double capital = 1000.0;
};

static Options parseArgs(int argc, char** argv) {
    Options opt;
    // Backward compatible: ./arbitrage 8
    if (argc > 1 && argv[1][0] != '-') opt.threads = std::atoi(argv[1]);
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--threads" && i + 1 < argc) opt.threads = std::atoi(argv[++i]);
        else if (a == "--verbose") opt.verbose = true;
        else if (a == "--no-trade") opt.trade = false;
        else if (a == "--max-ts" && i + 1 < argc) opt.maxTimestamps = std::atoi(argv[++i]);
        else if (a == "--capital" && i + 1 < argc) opt.capital = std::atof(argv[++i]);
        else if (a == "--help") {
            std::cout << "Usage: ./arbitrage [threads] [--threads N] [--max-ts N] [--verbose] [--no-trade]\n";
            std::exit(0);
        }
    }
    if (opt.threads < 1) opt.threads = 1;
    return opt;
}

int main(int argc, char** argv) {
    const Options opt = parseArgs(argc, argv);
    omp_set_dynamic(0);
    omp_set_num_threads(opt.threads);

    std::cout << "========== FOREX ARBITRAGE DETECTOR (OPENMP) ==========\n\n";
    std::cout << "Threads: " << opt.threads << " (max available " << omp_get_max_threads() << ")\n";
    std::cout << "Currencies:";
    for (int i = 0; i < kNumCurrencies; ++i) std::cout << " " << kCurrencies[i];
    std::cout << "\nPairs: " << kNumPairs << "\n\n";

    MarketData market = loadMarketData(".", opt.maxTimestamps, opt.threads);
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
    TimeSeriesArbitrageDetector detector(std::move(market), opt.threads);
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
        const auto& hist = pm.getPortfolioHistory();
        std::cout << "Trades executed: " << pm.getTradeHistory().size() << "\n";
        std::cout << "Final mark-to-USD: $" << std::fixed << std::setprecision(2)
                  << hist.back().totalValueUSD << "\n";
        if (opt.verbose) {
            pm.printTradeHistory();
            pm.printPortfolioHistory();
        }
    }

    Timer::report();
    return 0;
}
