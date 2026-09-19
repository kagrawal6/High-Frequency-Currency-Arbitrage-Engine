#include "ArbitrageDetector.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

constexpr int kMaxV = 16;
constexpr double kInf = 1e300;
constexpr double kEps = 1e-15;

ArbitrageOpportunity detectFromEdges(int V, int E, const int* src, const int* dest, const double* w) {
    ArbitrageOpportunity none{{}, 0.0};
    if (V < 3 || E < 3) return none;

    double dist[kMaxV];
    int pred[kMaxV];
    for (int i = 0; i < V; ++i) {
        dist[i] = 0.0;  // dummy source connected to every currency
        pred[i] = -1;
    }

    for (int iter = 0; iter < V; ++iter) {
        bool changed = false;
        for (int e = 0; e < E; ++e) {
            if (w[e] >= kInf / 2) continue;
            const int u = src[e];
            const int v = dest[e];
            const double nd = dist[u] + w[e];
            if (nd + kEps < dist[v]) {
                dist[v] = nd;
                pred[v] = u;
                changed = true;
            }
        }
        if (!changed) return none;
    }

    double W[kMaxV][kMaxV];
    for (int i = 0; i < V; ++i)
        for (int j = 0; j < V; ++j) W[i][j] = kInf;
    for (int e = 0; e < E; ++e) {
        if (w[e] < kInf / 2) W[src[e]][dest[e]] = w[e];
    }

    ArbitrageOpportunity best{{}, 0.0};
    bool seenStart[kMaxV] = {};

    for (int e = 0; e < E; ++e) {
        if (w[e] >= kInf / 2) continue;
        const int u = src[e];
        const int v = dest[e];
        if (!(dist[u] + w[e] + kEps < dist[v])) continue;

        int x = v;
        for (int k = 0; k < V; ++k) {
            if (pred[x] < 0) {
                x = -1;
                break;
            }
            x = pred[x];
        }
        if (x < 0 || seenStart[x]) continue;
        seenStart[x] = true;

        int cyc[kMaxV];
        int n = 0;
        int cur = x;
        double logSum = 0.0;
        bool ok = true;
        do {
            int p = pred[cur];
            if (p < 0 || W[p][cur] >= kInf / 2) {
                ok = false;
                break;
            }
            logSum += W[p][cur];
            cyc[n++] = cur;
            cur = p;
        } while (cur != x && n < V);

        if (!ok || cur != x || n < 2) continue;
        std::reverse(cyc, cyc + n);
        const double profit = (std::exp(-logSum) - 1.0) * 100.0;
        if (profit > best.profit) {
            best.profit = profit;
            best.cycle.assign(cyc, cyc + n);
        }
    }
    return best;
}

}  // namespace

ArbitrageOpportunity detectArbitrage(const ForexGraph& graph) {
    const auto& edges = graph.getEdges();
    const int V = graph.getVertexCount();
    const int E = static_cast<int>(edges.size());
    if (V <= 0 || E <= 0) return {{}, 0.0};

    int src[kMaxEdges];
    int dest[kMaxEdges];
    double w[kMaxEdges];
    const int use = std::min(E, kMaxEdges);
    for (int e = 0; e < use; ++e) {
        src[e] = edges[e].src;
        dest[e] = edges[e].dest;
        w[e] = edges[e].weight;
    }
    return detectFromEdges(V, use, src, dest, w);
}

ArbitrageOpportunity detectArbitrageAt(const MarketData& market, int t) {
    int src[kMaxEdges];
    int dest[kMaxEdges];
    double w[kMaxEdges];
    int E = 0;
    for (int p = 0; p < market.nPairs; ++p) {
        if (!market.hasPair(t, p)) continue;
        const int a = market.pairBase[p];
        const int b = market.pairQuote[p];
        const double bid = market.bidAt(t, p);
        const double ask = market.askAt(t, p);
        if (bid <= 0.0 || ask <= 0.0) continue;
        src[E] = a;
        dest[E] = b;
        w[E] = -std::log(bid);
        ++E;
        src[E] = b;
        dest[E] = a;
        w[E] = -std::log(1.0 / ask);
        ++E;
    }
    return detectFromEdges(market.nCurrencies, E, src, dest, w);
}
