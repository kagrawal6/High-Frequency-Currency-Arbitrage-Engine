#include "CudaLaunch.hpp"
#include "ArbitrageKernel.cuh"

#include <cuda_runtime.h>
#include <iostream>
#include <vector>

__global__ void detectKernel(int T, int V, int E,
                             const int* src, const int* dest, const double* weights,
                             double* profits, int* cycleLens, int* cycles) {
    const int t = blockIdx.x * blockDim.x + threadIdx.x;
    if (t >= T) return;
    fx::KernelHit hit = fx::detectOne(V, E, src, dest, weights + static_cast<size_t>(t) * E);
    profits[t] = hit.profit;
    cycleLens[t] = hit.cycle_len;
    for (int i = 0; i < 8; ++i) cycles[t * 8 + i] = hit.cycle[i];
}

static void check(cudaError_t err, const char* what) {
    if (err != cudaSuccess) {
        std::cerr << "CUDA error at " << what << ": " << cudaGetErrorString(err) << "\n";
        std::exit(1);
    }
}

std::vector<DeviceOpportunity> launchDetectKernel(const PackedWeights& packed, int blockSize) {
    const int T = packed.nTimestamps;
    const int E = packed.nEdges;
    const int V = packed.nCurrencies;
    std::vector<DeviceOpportunity> hits;
    if (T <= 0) return hits;
    if (blockSize < 32) blockSize = 32;

    int *dSrc = nullptr, *dDest = nullptr, *dLens = nullptr, *dCyc = nullptr;
    double *dW = nullptr, *dProfit = nullptr;
    check(cudaMalloc(&dSrc, sizeof(int) * E), "malloc src");
    check(cudaMalloc(&dDest, sizeof(int) * E), "malloc dest");
    check(cudaMalloc(&dW, sizeof(double) * T * E), "malloc weights");
    check(cudaMalloc(&dProfit, sizeof(double) * T), "malloc profit");
    check(cudaMalloc(&dLens, sizeof(int) * T), "malloc lens");
    check(cudaMalloc(&dCyc, sizeof(int) * T * 8), "malloc cycles");

    check(cudaMemcpy(dSrc, packed.src.data(), sizeof(int) * E, cudaMemcpyHostToDevice), "H2D src");
    check(cudaMemcpy(dDest, packed.dest.data(), sizeof(int) * E, cudaMemcpyHostToDevice), "H2D dest");
    check(cudaMemcpy(dW, packed.weights.data(), sizeof(double) * T * E, cudaMemcpyHostToDevice), "H2D w");

    const int grid = (T + blockSize - 1) / blockSize;
    detectKernel<<<grid, blockSize>>>(T, V, E, dSrc, dDest, dW, dProfit, dLens, dCyc);
    check(cudaGetLastError(), "launch");
    check(cudaDeviceSynchronize(), "sync");

    std::vector<double> profits(T);
    std::vector<int> lens(T), cyc(T * 8);
    check(cudaMemcpy(profits.data(), dProfit, sizeof(double) * T, cudaMemcpyDeviceToHost), "D2H p");
    check(cudaMemcpy(lens.data(), dLens, sizeof(int) * T, cudaMemcpyDeviceToHost), "D2H n");
    check(cudaMemcpy(cyc.data(), dCyc, sizeof(int) * T * 8, cudaMemcpyDeviceToHost), "D2H c");

    cudaFree(dSrc);
    cudaFree(dDest);
    cudaFree(dW);
    cudaFree(dProfit);
    cudaFree(dLens);
    cudaFree(dCyc);

    hits.reserve(256);
    for (int t = 0; t < T; ++t) {
        if (lens[t] <= 0 || profits[t] <= 0.0) continue;
        DeviceOpportunity o;
        o.timestamp_index = t;
        o.profit = static_cast<float>(profits[t]);
        o.cycle_len = lens[t];
        for (int i = 0; i < 8; ++i) o.cycle[i] = cyc[t * 8 + i];
        hits.push_back(o);
    }
    return hits;
}

const char* cudaBackendName() { return "cuda"; }
