#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

#include <omp.h>

#include "ArbitrageDetector.hpp"
#include "CsvParser.hpp"
#include "CurrencyConfig.hpp"
#include "ForexGraph.hpp"
#include "MarketData.hpp"
#include "PositionsManager.hpp"
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

static void test_currency_ids_stable() {
    CHECK(currencyId("AUD") == 0);
    CHECK(currencyId("USD") == 6);
    CHECK(kNumPairs == 19);
}

static void test_no_arbitrage_consistent_cross() {
    ForexGraph g;
    g.addExchangeRate("EUR", "USD", 1.10, 1.10);
    g.addExchangeRate("USD", "JPY", 150.0, 150.0);
    g.addExchangeRate("EUR", "JPY", 165.0, 165.0);
    auto opp = detectArbitrage(g);
    CHECK(opp.cycle.empty());
}

static void test_triangular_arbitrage_planted() {
    ForexGraph g;
    g.addExchangeRate("EUR", "USD", 1.20, 1.21);
    g.addExchangeRate("USD", "GBP", 0.90, 0.91);
    g.addExchangeRate("GBP", "EUR", 1.00, 1.01);
    auto opp = detectArbitrage(g);
    CHECK(!opp.cycle.empty());
    CHECK(opp.profit > 0.0);
}

static void test_profit_formula_matches_product() {
    ForexGraph g;
    g.addExchangeRate("USD", "EUR", 2.0, 2.0);
    g.addExchangeRate("EUR", "GBP", 2.0, 2.0);
    g.addExchangeRate("GBP", "USD", 0.30, 0.30);
    auto opp = detectArbitrage(g);
    CHECK(!opp.cycle.empty());
    CHECK(opp.profit > 15.0);
    CHECK(opp.profit < 25.0);
}

static void test_openmp_matches_serial_on_live_slice() {
    MarketData market = loadMarketData(".", 400, 4);
    if (market.nTimestamps == 0) {
        std::cerr << "SKIP openmp vs serial (no CSVs)\n";
        return;
    }
    TimeSeriesArbitrageDetector serial(market, 1);
    serial.analyzeAllTimestamps(false);
    TimeSeriesArbitrageDetector parallel(market, std::max(2, omp_get_max_threads()));
    parallel.analyzeAllTimestamps(false);
    CHECK(serial.getOpportunities().size() == parallel.getOpportunities().size());
    const auto& a = serial.getOpportunities();
    const auto& b = parallel.getOpportunities();
    for (size_t i = 0; i < a.size(); ++i) {
        CHECK(a[i].timestamp_ms == b[i].timestamp_ms);
        CHECK_NEAR(a[i].opportunity.profit, b[i].opportunity.profit, 1e-9);
        CHECK(a[i].opportunity.cycle == b[i].opportunity.cycle);
    }
}

static void test_csv_parser_fixture() {
    char tmpl[] = "/tmp/fxompXXXXXX";
    char* dir = mkdtemp(tmpl);
    CHECK(dir != nullptr);
    if (!dir) return;
    {
        std::ofstream bid(std::string(dir) + "/BID.csv");
        bid << "Gmt time,Open,High,Low,Close,Volume\n"
            << "26.03.2025 00:00:00.000,1.10,1.10,1.10,1.10000,100\n";
        std::ofstream ask(std::string(dir) + "/ASK.csv");
        ask << "Gmt time,Open,High,Low,Close,Volume\n"
            << "26.03.2025 00:00:00.000,1.12,1.12,1.12,1.12000,100\n";
    }
    auto rows = readCurrencyPairCsvs(std::string(dir) + "/BID.csv", std::string(dir) + "/ASK.csv");
    CHECK(rows.size() == 1);
    CHECK(rows[0].timestamp_ms == 0);
    CHECK_NEAR(rows[0].bid, 1.10, 1e-9);
}

static void test_thread_count_binding() {
    TimeSeriesArbitrageDetector d(MarketData{}, 8);
    CHECK(d.threadCount() == 8);
}

int main() {
    test_currency_ids_stable();
    test_no_arbitrage_consistent_cross();
    test_triangular_arbitrage_planted();
    test_profit_formula_matches_product();
    test_csv_parser_fixture();
    test_thread_count_binding();
    test_openmp_matches_serial_on_live_slice();

    std::cout << "openmp tests: " << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
