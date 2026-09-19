#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

#include <omp.h>

#include "MarketData.hpp"
#include "PositionsManager.hpp"
#include "TimeSeriesArbitrageDetector.hpp"
#include "Timer.hpp"

struct Metrics {
    double wall_ms = 0;
    double detect_ms = 0;
    double profit_usd = 0;
    int opportunities = 0;
};

static Metrics measure(const MarketData& market, int threads, double capital) {
    Timer::setEnabled(false);
    omp_set_dynamic(0);
    omp_set_num_threads(threads);

    auto wc0 = std::chrono::steady_clock::now();
    auto d0 = std::chrono::steady_clock::now();
    TimeSeriesArbitrageDetector detector(market, threads);
    const int inner = 20;
    for (int k = 0; k < inner; ++k) detector.analyzeAllTimestamps(false);
    auto d1 = std::chrono::steady_clock::now();

    PositionsManager pm("USD", capital);
    for (const auto& opp : detector.getOpportunities()) {
        pm.executeArbitrageOpportunity(opp, detector.getGraphForIndex(opp.timestamp_index));
    }
    const auto& hist = pm.getPortfolioHistory();
    auto wc1 = std::chrono::steady_clock::now();

    Metrics m;
    m.wall_ms = std::chrono::duration<double, std::milli>(wc1 - wc0).count();
    m.detect_ms = std::chrono::duration<double, std::milli>(d1 - d0).count() / 20.0;
    m.profit_usd = hist.empty() ? 0.0 : hist.back().totalValueUSD - capital;
    m.opportunities = static_cast<int>(detector.getOpportunities().size());
    return m;
}

int main(int argc, char** argv) {
    int maxTs = 0;
    int reps = 5;
    double capital = 1000.0;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--max-ts" && i + 1 < argc) maxTs = std::atoi(argv[++i]);
        else if (a == "--reps" && i + 1 < argc) reps = std::atoi(argv[++i]);
    }

    Timer::setEnabled(false);
    const int hw = std::max(1, omp_get_max_threads());
    MarketData market = loadMarketData(".", maxTs, hw);

    std::vector<int> thread_counts = {1, 2, 4, 8};
    if (hw >= 12) thread_counts.push_back(12);
    if (hw >= 15) thread_counts.push_back(15);

    measure(market, 1, capital);
    Metrics base = measure(market, 1, capital);

    std::cout << "backend,config,mean_wall_ms,stddev_wall_ms,mean_detect_ms,stddev_detect_ms,"
                 "speedup,efficiency,timestamps,opportunities,mean_profit_usd,timestamps_per_sec\n";

    for (int p : thread_counts) {
        measure(market, p, capital);
        std::vector<double> walls, detects, profits;
        int opps = 0;
        for (int i = 0; i < reps; ++i) {
            Metrics m = measure(market, p, capital);
            walls.push_back(m.wall_ms);
            detects.push_back(m.detect_ms);
            profits.push_back(m.profit_usd);
            opps = m.opportunities;
        }
        auto mean = [](const std::vector<double>& v) {
            return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
        };
        auto stdev = [&](const std::vector<double>& v) {
            double mu = mean(v);
            double acc = 0;
            for (double x : v) acc += (x - mu) * (x - mu);
            return std::sqrt(acc / v.size());
        };
        const double mean_wall = mean(walls);
        const double mean_detect = mean(detects);
        const double S = base.detect_ms / mean_detect;
        const double E = S / p;
        const double tps = market.nTimestamps * 1000.0 / mean_detect;
        std::cout << std::fixed << std::setprecision(6)
                  << "openmp," << p << ","
                  << mean_wall << "," << stdev(walls) << ","
                  << mean_detect << "," << stdev(detects) << ","
                  << S << "," << E << ","
                  << market.nTimestamps << "," << opps << ","
                  << mean(profits) << "," << tps << "\n";
    }
    return 0;
}
