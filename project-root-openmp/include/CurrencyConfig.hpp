#pragma once

#include <cstring>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

/// Canonical currency universe. IDs are stable across timestamps and backends.
inline constexpr const char* kCurrencies[] = {
    "AUD", "CAD", "EUR", "GBP", "JPY", "SGD", "USD"
};
inline constexpr int kNumCurrencies = 7;

struct PairFile {
    const char* ask;
    const char* bid;
    const char* base;
    const char* quote;
};

inline constexpr PairFile kPairs[] = {
    {"data/ask/AUDCAD_ASK.csv", "data/bid/AUDCAD_BID.csv", "AUD", "CAD"},
    {"data/ask/AUDJPY_ASK.csv", "data/bid/AUDJPY_BID.csv", "AUD", "JPY"},
    {"data/ask/AUDSGD_ASK.csv", "data/bid/AUDSGD_BID.csv", "AUD", "SGD"},
    {"data/ask/AUDUSD_ASK.csv", "data/bid/AUDUSD_BID.csv", "AUD", "USD"},
    {"data/ask/CADJPY_ASK.csv", "data/bid/CADJPY_BID.csv", "CAD", "JPY"},
    {"data/ask/EURAUD_ASK.csv", "data/bid/EURAUD_BID.csv", "EUR", "AUD"},
    {"data/ask/EURCAD_ASK.csv", "data/bid/EURCAD_BID.csv", "EUR", "CAD"},
    {"data/ask/EURGBP_ASK.csv", "data/bid/EURGBP_BID.csv", "EUR", "GBP"},
    {"data/ask/EURJPY_ASK.csv", "data/bid/EURJPY_BID.csv", "EUR", "JPY"},
    {"data/ask/EURSGD_ASK.csv", "data/bid/EURSGD_BID.csv", "EUR", "SGD"},
    {"data/ask/EURUSD_ASK.csv", "data/bid/EURUSD_BID.csv", "EUR", "USD"},
    {"data/ask/GBPAUD_ASK.csv", "data/bid/GBPAUD_BID.csv", "GBP", "AUD"},
    {"data/ask/GBPCAD_ASK.csv", "data/bid/GBPCAD_BID.csv", "GBP", "CAD"},
    {"data/ask/GBPJPY_ASK.csv", "data/bid/GBPJPY_BID.csv", "GBP", "JPY"},
    {"data/ask/GBPUSD_ASK.csv", "data/bid/GBPUSD_BID.csv", "GBP", "USD"},
    {"data/ask/SGDJPY_ASK.csv", "data/bid/SGDJPY_BID.csv", "SGD", "JPY"},
    {"data/ask/USDCAD_ASK.csv", "data/bid/USDCAD_BID.csv", "USD", "CAD"},
    {"data/ask/USDJPY_ASK.csv", "data/bid/USDJPY_BID.csv", "USD", "JPY"},
    {"data/ask/USDSGD_ASK.csv", "data/bid/USDSGD_BID.csv", "USD", "SGD"},
};
inline constexpr int kNumPairs = 19;
inline constexpr int kMaxEdges = kNumPairs * 2;

inline int currencyId(const char* name) {
    for (int i = 0; i < kNumCurrencies; ++i) {
        if (std::strcmp(kCurrencies[i], name) == 0) return i;
    }
    throw std::runtime_error(std::string("unknown currency: ") + name);
}

inline int currencyId(const std::string& name) {
    return currencyId(name.c_str());
}

inline const char* currencyName(int id) {
    if (id < 0 || id >= kNumCurrencies) throw std::runtime_error("bad currency id");
    return kCurrencies[id];
}

inline std::vector<std::tuple<std::string, std::string, std::string, std::string>>
defaultCurrencyFiles() {
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> out;
    out.reserve(kNumPairs);
    for (int i = 0; i < kNumPairs; ++i) {
        out.emplace_back(kPairs[i].ask, kPairs[i].bid, kPairs[i].base, kPairs[i].quote);
    }
    return out;
}
