#pragma once

#include <cmath>

// Shared dummy-source Bellman-Ford used by both nvcc device code and the host emulator.
// One caller (thread or lane) owns one timestamp.

#ifndef FX_HD
#ifdef __CUDACC__
#define FX_HD __host__ __device__
#else
#define FX_HD
#endif
#endif

namespace fx {

constexpr int kMaxV = 16;
constexpr int kMaxE = 40;
constexpr double kInf = 1.0e300;
constexpr double kEps = 1.0e-15;

struct KernelHit {
    double profit;
    int cycle_len;
    int cycle[8];
};

FX_HD inline KernelHit detectOne(int V, int E, const int* src, const int* dest, const double* w) {
    KernelHit hit;
    hit.profit = 0.0;
    hit.cycle_len = 0;
    for (int i = 0; i < 8; ++i) hit.cycle[i] = -1;
    if (V < 3 || E < 3) return hit;

    double dist[kMaxV];
    int pred[kMaxV];
    for (int i = 0; i < V; ++i) {
        dist[i] = 0.0;
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
        if (!changed) return hit;
    }

    double W[kMaxV][kMaxV];
    for (int i = 0; i < V; ++i)
        for (int j = 0; j < V; ++j) W[i][j] = kInf;
    for (int e = 0; e < E; ++e) {
        if (w[e] < kInf / 2) W[src[e]][dest[e]] = w[e];
    }

    bool seenStart[kMaxV];
    for (int i = 0; i < V; ++i) seenStart[i] = false;

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
            const int p = pred[cur];
            if (p < 0 || W[p][cur] >= kInf / 2) {
                ok = false;
                break;
            }
            logSum += W[p][cur];
            cyc[n++] = cur;
            cur = p;
        } while (cur != x && n < V);

        if (!ok || cur != x || n < 2) continue;
        // reverse
        for (int i = 0, j = n - 1; i < j; ++i, --j) {
            int tmp = cyc[i];
            cyc[i] = cyc[j];
            cyc[j] = tmp;
        }
        double profit = std::exp(-logSum) - 1.0;
        profit *= 100.0;
        if (profit > hit.profit) {
            hit.profit = profit;
            hit.cycle_len = n < 8 ? n : 8;
            for (int i = 0; i < hit.cycle_len; ++i) hit.cycle[i] = cyc[i];
        }
    }
    return hit;
}

}  // namespace fx
