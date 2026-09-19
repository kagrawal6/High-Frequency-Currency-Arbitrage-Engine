#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/// Directed FX edge. weight = -log(exchange rate).
struct Edge {
    int src;
    int dest;
    double weight;
    Edge(int s, int d, double rate);
};

class ForexGraph {
public:
    ForexGraph();
    void reserveEdges(size_t n) { edges.reserve(n); }
    int addCurrency(const std::string& currency);
    void addExchangeRate(const std::string& from, const std::string& to, double bid, double ask);

    const std::vector<Edge>& getEdges() const { return edges; }
    int getVertexCount() const { return V; }
    const std::string& getCurrencyName(int id) const { return idToCurrency[id]; }
    int getCurrencyId(const std::string& name) const { return currencyToId.at(name); }

    /// O(1) rate lookup: exp(-weight) for src->dest, or 0 if missing.
    double rate(int src, int dest) const;
    double weight(int src, int dest) const;

private:
    int V;
    std::vector<Edge> edges;
    std::unordered_map<std::string, int> currencyToId;
    std::vector<std::string> idToCurrency;
    std::vector<double> weightMatrix;
    void ensureMatrix();
};
