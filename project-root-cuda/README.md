# CUDA backend

**You are in** `project-root-cuda/`. On this Apple Mac the binary is the host emulator of the CUDA kernel (`cuda-emu`). A machine with `nvcc` compiles `src/arbitrage_launch.cu` instead.

| Up | Sibling engines | Report |
|---|---|---|
| [Root README](../README.md) (repo map, all 19 pairs) | [Sequential](../project-root-sequential/README.md) · [OpenMP](../project-root-openmp/README.md) | [project_description.md](../project_description.md) |

Same economics as the sequential and OpenMP engines, but the hot path is a CUDA kernel: **one thread per timestamp** over a packed `T × E` weight tensor (`E = 38`).

On a machine with `nvcc`, the Makefile compiles `src/arbitrage_launch.cu`. On this Apple M5 Pro there is no NVIDIA toolchain, so the same kernel body in `include/ArbitrageKernel.cuh` is compiled as a host emulator (`src/arbitrage_launch_emu.cpp`) that keeps the CUDA mapping (one work-item per timestamp) and runs it with OpenMP.

## Build and run

```bash
make            # nvcc if present, otherwise CUDA_EMU + OpenMP
make run
make test       # kernel ≡ host detector on planted + live data
make bench-csv  # sweeps block sizes 32, 64, 128, 256
```

```bash
./arbitrage --block 128 --verbose
```

`data/` is a symlink to `../project-root-sequential/data` so the 200 MB tick dump is not tripled.

## Kernel contract

```text
detectKernel<<<ceil(T/B), B>>>(T, V, E, src, dest, weights, profits, cycleLens, cycles)
```

- `weights` is SoA, row `t` holds 38 `-log(rate)` edges (or +∞ if the pair is missing).
- `detectOne` is `__host__ __device__` dummy-source Bellman–Ford.
- Host gathers hits with `profit > 0` and reconstructs `TimeStampedArbitrage` for the book.

## Measured on Apple M5 Pro (CUDA-EMU)

| Block (logical) | Kernel (ms) | Timestamps/s |
|---|---:|---:|
| 32 | 1.07 | 81.0 M |
| 64 | 0.97 | 89.4 M |
| 128 | 0.97 | 89.2 M |
| 256 | 0.99 | 87.1 M |

The packed kernel is faster than OpenMP-on-objects even when both use host threads, because the CUDA layout has no per-timestamp heap traffic. On an NVIDIA GPU the same `.cu` file is the production path. Details: [`project_description.md`](../project_description.md).
