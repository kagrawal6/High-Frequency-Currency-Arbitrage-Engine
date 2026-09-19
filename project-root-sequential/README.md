# Sequential backend

**You are in** `project-root-sequential/` — the one-thread baseline.

| Up | Sibling engines | Report |
|---|---|---|
| [Root README](../README.md) (repo map, all 19 pairs) | [OpenMP](../project-root-openmp/README.md) · [CUDA](../project-root-cuda/README.md) | [project_description.md](../project_description.md) |

Single-thread reference engine. Same packed market panel and dummy-source Bellman–Ford as the OpenMP and CUDA trees, with no parallel regions. Use this folder as the correctness baseline.

## Build and run

```bash
make            # ./arbitrage
make run        # full day, detect + trade
make test       # planted graphs, parser fixture, live-data determinism
make bench-csv  # writes benchmarks/benchmark_results.csv
make clean
```

```bash
./arbitrage --max-ts 2000 --no-trade
./benchmark --reps 5
```

Requires a C++17 compiler. The sequential Makefile does **not** link OpenMP.

## Layout

| Path | Role |
|---|---|
| `include/`, `src/` | Packed loader, detector, portfolio simulator, timers |
| `tests/test_engine.cpp` | 11 named checks, 34 assertions |
| `benchmark.cpp` | 20 inner detection passes × 5 outer reps |
| `data/` | Dukascopy bid/ask CSVs |
| `benchmarks/`, `output/` | Measured CSVs and copied figures |

## What this backend does

1. mmap-parse every pair CSV without `strtok` / `std::stod`.
2. Align the union of timestamps into a `T × P` SoA (`bid`, `ask`, `present`).
3. For each timestamp, relax 38 `-log(rate)` edges from a dummy source (`O(VE)`, not `O(V²E)`).
4. Reconstruct the most profitable simple negative cycle.
5. Optionally execute every hit on a $1,000 USD book.

Measured on Apple M5 Pro, full 86,400 timestamps: **13.92 ± 0.31 ms** detection, **6.21 M timestamps/s**, **680** opportunities. See the root [`project_description.md`](../project_description.md).
