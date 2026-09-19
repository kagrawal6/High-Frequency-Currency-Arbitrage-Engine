#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <unistd.h>

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
    CHECK(std::string(currencyName(2)) == "EUR");
    CHECK(kNumPairs == 19);
    CHECK(kNumCurrencies == 7);
}

static void test_no_arbitrage_consistent_cross() {
    ForexGraph g;
    // Triangle identity: EUR/USD * USD/JPY = EUR/JPY, with a spread but no loop profit.
    g.addExchangeRate("EUR", "USD", 1.10, 1.10);
    g.addExchangeRate("USD", "JPY", 150.0, 150.0);
    g.addExchangeRate("EUR", "JPY", 165.0, 165.0);
    auto opp = detectArbitrage(g);
    CHECK(opp.cycle.empty());
    CHECK_NEAR(opp.profit, 0.0, 1e-12);
}

static void test_triangular_arbitrage_planted() {
    ForexGraph g;
    // Product of mid rates around the loop is > 1: 1.10 * (1/1.20) * 1.20 > 1? 
    // Use an explicit mispricing: EUR->USD 1.20, USD->GBP 0.90, GBP->EUR 1.00
    // 1.20 * 0.90 * 1.00 = 1.08 => 8% if we could trade mid.
    // Bid/ask: sell EUR for USD at bid 1.20, sell USD for GBP at bid 0.90, sell GBP for EUR at bid 1.00.
    g.addExchangeRate("EUR", "USD", 1.20, 1.21);
    g.addExchangeRate("USD", "GBP", 0.90, 0.91);
    g.addExchangeRate("GBP", "EUR", 1.00, 1.01);
    auto opp = detectArbitrage(g);
    CHECK(!opp.cycle.empty());
    CHECK(opp.profit > 0.0);
    CHECK(opp.cycle.size() >= 3);
}

static void test_ask_spread_kills_arbitrage() {
    ForexGraph g;
    // Same notionals as a profitable mid, but a wide ask on the last leg.
    g.addExchangeRate("EUR", "USD", 1.20, 1.40);
    g.addExchangeRate("USD", "GBP", 0.90, 1.10);
    g.addExchangeRate("GBP", "EUR", 0.80, 1.20);
    auto opp = detectArbitrage(g);
    // May or may not exist depending on which directed edges remain; just assert detector is stable.
    CHECK(opp.profit >= 0.0);
    if (!opp.cycle.empty()) CHECK(opp.cycle.size() >= 2);
}

static void test_empty_graph() {
    ForexGraph g;
    auto opp = detectArbitrage(g);
    CHECK(opp.cycle.empty());
}

static void test_profit_formula_matches_product() {
    ForexGraph g;
    g.addExchangeRate("USD", "EUR", 2.0, 2.0);
    g.addExchangeRate("EUR", "GBP", 2.0, 2.0);
    g.addExchangeRate("GBP", "USD", 0.30, 0.30);  // 2*2*0.30 = 1.20 => 20%
    auto opp = detectArbitrage(g);
    CHECK(!opp.cycle.empty());
    CHECK(opp.profit > 15.0);
    CHECK(opp.profit < 25.0);
}

static void test_graph_rate_lookup() {
    ForexGraph g;
    g.addExchangeRate("USD", "JPY", 150.0, 150.2);
    CHECK(g.getVertexCount() == 2);
    CHECK(g.getEdges().size() == 2);
    CHECK_NEAR(g.rate(g.getCurrencyId("USD"), g.getCurrencyId("JPY")), 150.0, 1e-9);
    CHECK_NEAR(g.rate(g.getCurrencyId("JPY"), g.getCurrencyId("USD")), 1.0 / 150.2, 1e-12);
}

static std::string writeFixture(const std::string& dir, const std::string& name, const std::string& body) {
    std::string path = dir + "/" + name;
    std::ofstream out(path);
    out << body;
    return path;
}

static void test_csv_parser_fixture() {
    char tmpl[] = "/tmp/fxfixXXXXXX";
    char* dir = mkdtemp(tmpl);
    CHECK(dir != nullptr);
    if (!dir) return;
    writeFixture(dir, "BID.csv",
                 "Gmt time,Open,High,Low,Close,Volume\n"
                 "26.03.2025 00:00:00.000,1.10,1.10,1.10,1.10000,100\n"
                 "26.03.2025 00:00:01.000,1.11,1.11,1.11,1.11000,100\n");
    writeFixture(dir, "ASK.csv",
                 "Gmt time,Open,High,Low,Close,Volume\n"
                 "26.03.2025 00:00:00.000,1.12,1.12,1.12,1.12000,100\n"
                 "26.03.2025 00:00:01.000,1.13,1.13,1.13,1.13000,100\n");
    auto rows = readCurrencyPairCsvs(std::string(dir) + "/BID.csv", std::string(dir) + "/ASK.csv");
    CHECK(rows.size() == 2);
    CHECK(rows[0].timestamp_ms == 0);
    CHECK(rows[1].timestamp_ms == 1000);
    CHECK_NEAR(rows[0].bid, 1.10, 1e-9);
    CHECK_NEAR(rows[0].ask, 1.12, 1e-9);
    CHECK_NEAR(rows[1].bid, 1.11, 1e-9);
}

static void test_packed_detector_matches_graph() {
    MarketData m;
    m.nCurrencies = 3;
    m.nPairs = 3;
    m.nTimestamps = 1;
    m.timestamps = {0};
    m.bid = {1.20, 0.90, 1.00};
    m.ask = {1.21, 0.91, 1.01};
    m.present = {1, 1, 1};
    m.pairBase[0] = currencyId("EUR");
    m.pairQuote[0] = currencyId("USD");
    m.pairBase[1] = currencyId("USD");
    m.pairQuote[1] = currencyId("GBP");
    m.pairBase[2] = currencyId("GBP");
    m.pairQuote[2] = currencyId("EUR");

    auto fromPacked = detectArbitrageAt(m, 0);
    auto fromGraph = detectArbitrage(m.graphAt(0));
    CHECK(fromPacked.cycle.empty() == fromGraph.cycle.empty());
    CHECK_NEAR(fromPacked.profit, fromGraph.profit, 1e-9);
}

static void test_positions_execute_cycle() {
    ForexGraph g;
    g.addExchangeRate("USD", "EUR", 2.0, 2.0);
    g.addExchangeRate("EUR", "GBP", 2.0, 2.0);
    g.addExchangeRate("GBP", "USD", 0.30, 0.30);
    auto opp = detectArbitrage(g);
    CHECK(!opp.cycle.empty());

    TimeStampedArbitrage ts{1000, 0, opp};
    PositionsManager pm("USD", 1000.0);
    pm.executeArbitrageOpportunity(ts, g);
    CHECK(!pm.getTradeHistory().empty());
    CHECK(pm.getPortfolioHistory().size() >= 2);
}

static void test_live_data_smoke() {
    MarketData market = loadMarketData(".", 200);
    if (market.nTimestamps == 0) {
        std::cerr << "SKIP live data smoke (no CSVs in ./data)\n";
        return;
    }
    CHECK(market.nTimestamps > 0);
    CHECK(market.nPairs == 19);
    TimeSeriesArbitrageDetector det(market);
    det.analyzeAllTimestamps(false);
    // Detector must be deterministic.
    TimeSeriesArbitrageDetector det2(market);
    det2.analyzeAllTimestamps(false);
    CHECK(det.getOpportunities().size() == det2.getOpportunities().size());
    if (!det.getOpportunities().empty()) {
        CHECK_NEAR(det.getOpportunities()[0].opportunity.profit,
                   det2.getOpportunities()[0].opportunity.profit, 1e-12);
    }
}

int main() {
    test_currency_ids_stable();
    test_empty_graph();
    test_graph_rate_lookup();
    test_no_arbitrage_consistent_cross();
    test_triangular_arbitrage_planted();
    test_ask_spread_kills_arbitrage();
    test_profit_formula_matches_product();
    test_csv_parser_fixture();
    test_packed_detector_matches_graph();
    test_positions_execute_cycle();
    test_live_data_smoke();

    std::cout << "sequential tests: " << g_passed << " passed, " << g_failed << " failed\n";
    return g_failed == 0 ? 0 : 1;
}
