#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>
#include <chrono>

class Timer {
public:
    using Clock = std::chrono::steady_clock;
    explicit Timer(const std::string& label);
    ~Timer();
    void stop();
    static void report();
    static void reset();
    static void setEnabled(bool enabled);
    static bool enabled();

private:
    std::string label_;
    Clock::time_point start_;
    bool stopped_{false};
    static std::map<std::string, std::vector<int64_t>> records_;
    static bool enabled_;
};
