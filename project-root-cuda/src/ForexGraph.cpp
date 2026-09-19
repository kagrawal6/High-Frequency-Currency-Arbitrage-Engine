#include "ForexGraph.hpp"

#include <cmath>
#include <limits>

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();
}

Edge::Edge(int s, int d, double rate)
    : src(s), dest(d), weight(-std::log(rate)) {}

ForexGraph::ForexGraph() : V(0) {}

int ForexGraph::addCurrency(const std::string& currency) {
    auto it = currencyToId.find(currency);
    if (it != currencyToId.end()) return it->second;
    int id = V++;
    currencyToId.emplace(currency, id);
    idToCurrency.push_back(currency);
    weightMatrix.clear();
    return id;
}

void ForexGraph::ensureMatrix() {
    const int n = V;
    if (static_cast<int>(weightMatrix.size()) == n * n) return;
    weightMatrix.assign(static_cast<size_t>(n) * n, kInf);
    for (const auto& e : edges) {
        weightMatrix[static_cast<size_t>(e.src) * n + e.dest] = e.weight;
    }
}

void ForexGraph::addExchangeRate(const std::string& from,
                                const std::string& to,
                                double bid,
                                double ask) {
    int src = addCurrency(from);
    int dest = addCurrency(to);
    edges.emplace_back(src, dest, bid);
    edges.emplace_back(dest, src, 1.0 / ask);
    weightMatrix.clear();
}

double ForexGraph::weight(int src, int dest) const {
    const_cast<ForexGraph*>(this)->ensureMatrix();
    return weightMatrix[static_cast<size_t>(src) * V + dest];
}

double ForexGraph::rate(int src, int dest) const {
    double w = weight(src, dest);
    if (!std::isfinite(w)) return 0.0;
    return std::exp(-w);
}
