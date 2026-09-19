#include "MarketData.hpp"

#include <algorithm>
#include <iostream>
#include <unordered_map>

#include <omp.h>

#include "CsvParser.hpp"
#include "Timer.hpp"

ForexGraph MarketData::graphAt(int t) const {
    ForexGraph g;
    g.reserveEdges(static_cast<size_t>(nPairs) * 2);
    for (int p = 0; p < nPairs; ++p) {
        if (!hasPair(t, p)) continue;
        g.addExchangeRate(kCurrencies[pairBase[p]], kCurrencies[pairQuote[p]],
                          bidAt(t, p), askAt(t, p));
    }
    return g;
}

MarketData loadMarketData(const std::string& dataRoot, int maxTimestamps, int thread_count) {
    Timer tLoad("CSV load + align");
    MarketData market;
    std::string prefix = dataRoot;
    if (!prefix.empty() && prefix.back() != '/') prefix.push_back('/');

    if (thread_count < 1) thread_count = 1;
    omp_set_dynamic(0);
    omp_set_num_threads(thread_count);

    std::vector<std::vector<CurrencyPairData>> pairTicks(kNumPairs);
    for (int p = 0; p < kNumPairs; ++p) {
        market.pairBase[p] = currencyId(kPairs[p].base);
        market.pairQuote[p] = currencyId(kPairs[p].quote);
    }

    // Parallelize across files only. Do not nest OpenMP inside the parser.
    #pragma omp parallel for num_threads(thread_count) schedule(dynamic)
    for (int p = 0; p < kNumPairs; ++p) {
        pairTicks[p] = readCurrencyPairCsvs(prefix + kPairs[p].bid, prefix + kPairs[p].ask);
    }
    for (int p = 0; p < kNumPairs; ++p) {
        if (pairTicks[p].empty()) {
            std::cerr << "Warning: no ticks for " << kPairs[p].base << "/" << kPairs[p].quote << "\n";
        }
    }

    std::vector<int64_t> allTs;
    allTs.reserve(90000);
    for (int p = 0; p < kNumPairs; ++p) {
        for (const auto& tick : pairTicks[p]) allTs.push_back(tick.timestamp_ms);
    }
    std::sort(allTs.begin(), allTs.end());
    allTs.erase(std::unique(allTs.begin(), allTs.end()), allTs.end());
    if (maxTimestamps > 0 && static_cast<int>(allTs.size()) > maxTimestamps) {
        allTs.resize(static_cast<size_t>(maxTimestamps));
    }

    market.nTimestamps = static_cast<int>(allTs.size());
    market.timestamps = std::move(allTs);
    const size_t cells = static_cast<size_t>(market.nTimestamps) * kNumPairs;
    market.bid.assign(cells, 0.0);
    market.ask.assign(cells, 0.0);
    market.present.assign(cells, 0);

    std::unordered_map<int64_t, int> index;
    index.reserve(static_cast<size_t>(market.nTimestamps) * 2);
    for (int t = 0; t < market.nTimestamps; ++t) index[market.timestamps[t]] = t;

    #pragma omp parallel for num_threads(thread_count) schedule(static)
    for (int p = 0; p < kNumPairs; ++p) {
        for (const auto& tick : pairTicks[p]) {
            auto it = index.find(tick.timestamp_ms);
            if (it == index.end()) continue;
            const size_t cell = static_cast<size_t>(it->second) * kNumPairs + p;
            market.bid[cell] = tick.bid;
            market.ask[cell] = tick.ask;
            market.present[cell] = 1;
        }
    }
    tLoad.stop();
    return market;
}
