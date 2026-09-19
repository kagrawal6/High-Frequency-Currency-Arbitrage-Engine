#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct CurrencyPairData {
    int64_t timestamp_ms;
    double bid;
    double ask;
};

/// mmap + in-place scan. Does not store currency strings per tick.
std::vector<CurrencyPairData> readCurrencyPairCsvs(
    const std::string& bidFilepath,
    const std::string& askFilepath);
