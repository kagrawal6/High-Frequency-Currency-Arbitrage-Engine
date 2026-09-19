#include <cmath>
#include <iostream>
#include <vector>

#include "ArbitrageDetector.hpp"
#include "ArbitrageKernel.cuh"
#include "CudaLaunch.hpp"
#include "CurrencyConfig.hpp"
#include "ForexGraph.hpp"
#include "MarketData.hpp"
#include "TimeSeriesArbitrageDetector.hpp"

static int g_failed = 0;
static int g_passed = 0;

#define CHECK(cond) \
    do { \
        if (cond) { \
            ++g_passed; \
        } else { \
            ++g_failed; \
            std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " : " #cond "\n"; \
        } \
    } while (0)

#define CHECK_NEAR(a, b, eps) CHECK(std::fabs((a) - (b)) < (eps))

static void test_kernel_matches_host_on_planted_triangle() {
    ForexGraph g;
    g.addExchangeRate("USD", "EUR", 2.0, 2.0);
    g.addExchangeRate("EUR", "GBP", 2.0, 2.0);
    g.addExchangeRate("GBP", "USD", 0.30, 0.30);
    auto host = detectArbitrage(g);
    CHECK(!host.cycle.empty());
    CHECK(host.profit > 15.0);

    const auto& edges = g.getEdges();
    std::vector<int> src, dest;
    std::vector<double> w;
    for (const auto& e : edges) {
        src.push_back(e.src);
        dest.push_back(e.dest);
        w.push_back(e.weight);
    }
    auto hit = fx::detectOne(g.getVertexCount(), static_cast<int>(edges.size()),
                             src.data(), dest.data(), w.data());
    CHECK(hit.cycle_len >= 3);
    CHECK_NEAR(hit.profit, host.profit, 1e-8);
}

static void test_kernel_no_cycle() {
    ForexGraph g;
    g.addExchangeRate("EUR", "USD", 1.10, 1.10);
    g.addExchangeRate("USD", "JPY", 150.0, 150.0);
    g.addExchangeRate("EUR", "JPY", 165.0, 165.0);
    const auto& edges = g.getEdges();
    std::vector<int> src, dest;
    std::vector<double> w;
    for (const auto& e : edges) {
        src.push_back(e.src);
        dest.push_back(e.dest);
        w.push_back(e.weight);
    }
    auto hit = fx::detectOne(g.getVertexCount(), static_cast<int>(edges.size()),
                             src.data(), dest.data(), w.data());
    CHECK(hit.cycle_len == 0);
}

static void test_packed_kernel_matches_host_slice() {
    MarketData market = loadMarketData(".", 300);
    if (market.nTimestamps == 0) {
        std::cerr << "SKIP cuda packed vs host (no CSVs)\n";
        return;
    }
    PackedWeights packed = packMarketWeights(market);
    auto deviceHits = launchDetectKernel(packed, 128);

    TimeSeriesArbitrageDetector host(market);
    // Host path in this folder also goes through the kernel, so compare against detectArbitrageAt.
    int expected = 0;
    for (int t = 0; t < market.nTimestamps; ++t) {
        auto opp = detectArbitrageAt(market, t);
        if (!opp.cycle.empty() && opp.profit > 0.0) ++expected;
    }
    CHECK(static_cast<int>(deviceHits.size()) == expected);

    for (const auto& h : deviceHits) {
        auto opp = detectArbitrageAt(market, h.timestamp_index);
        CHECK(!opp.cycle.empty());
        CHECK_NEAR(opp.profit, h.profit, 1e-4);
    }
}

static void test_backend_name() {
    const char* name = cudaBackendName();
    CHECK(name != nullptr);
    CHECK(name[0] != '\0');
}

int main() {
    test_kernel_matches_host_on_planted_triangle();
    test_kernel_no_cycle();
    test_backend_name();
    test_packed_kernel_matches_host_slice();

    std::cout << "cuda tests: " << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
