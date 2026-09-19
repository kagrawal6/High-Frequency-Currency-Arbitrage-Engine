#include "Timer.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numeric>

std::map<std::string, std::vector<int64_t>> Timer::records_;
bool Timer::enabled_ = true;

Timer::Timer(const std::string& label)
    : label_(label), start_(Clock::now()) {}

Timer::~Timer() {
    if (!stopped_) stop();
}

void Timer::stop() {
    if (stopped_) return;
    auto end = Clock::now();
    if (enabled_) {
        int64_t ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start_).count();
        records_[label_].push_back(ns);
    }
    stopped_ = true;
}

void Timer::report() {
    if (records_.empty()) return;
    std::cout << "\n===== TIMING SUMMARY =====\n"
              << std::fixed << std::setprecision(3)
              << std::left << std::setw(28) << "Stage"
              << std::right << std::setw(8) << "Count"
              << std::setw(14) << "Total ms"
              << std::setw(12) << "Avg ms"
              << std::setw(12) << "Min ms"
              << std::setw(12) << "Max ms"
              << "\n" << std::string(86, '-') << "\n";

    for (auto& [label, vec] : records_) {
        int cnt = static_cast<int>(vec.size());
        int64_t sum_ns = std::accumulate(vec.begin(), vec.end(), int64_t(0));
        auto [mn, mx] = std::minmax_element(vec.begin(), vec.end());
        double total_ms = sum_ns / 1e6;
        std::cout << std::left << std::setw(28) << label
                  << std::right << std::setw(8) << cnt
                  << std::setw(14) << total_ms
                  << std::setw(12) << (total_ms / cnt)
                  << std::setw(12) << (*mn / 1e6)
                  << std::setw(12) << (*mx / 1e6)
                  << "\n";
    }
    std::cout << std::endl;
}

void Timer::reset() { records_.clear(); }
void Timer::setEnabled(bool enabled) { enabled_ = enabled; }
bool Timer::enabled() { return enabled_; }
