# OpenMP backend

**You are in** `project-root-openmp/` (renamed from `project-root-parallel`).

| Up | Sibling engines | Report |
|---|---|---|
| [Root README](../README.md) (repo map, all 19 pairs) | [Sequential](../project-root-sequential/README.md) · [CUDA](../project-root-cuda/README.md) | [project_description.md](../project_description.md) |

Timestamp-parallel engine. The detector itself is serial *per thread*; OpenMP only fans out across independent seconds and across the 19 pair files. There is **no nested team** inside Bellman–Ford (that was the old speed killer).

## Build and run

macOS needs Homebrew LLVM (`brew install llvm`) because Apple clang has no `-fopenmp`. The Makefile picks `/opt/homebrew/opt/llvm/bin/clang++` automatically.

```bash
make
make run-seq                      # 1 thread
make run-parallel                 # default 4
make run-parallel PARALLEL_THREADS=12
make test                         # includes OpenMP ≡ serial on a live slice
make bench-csv
```

```bash
./arbitrage 8
./arbitrage --threads 12 --max-ts 10000
```

## Parallel strategy

- **Load:** `#pragma omp parallel for schedule(dynamic)` over 19 pair files. The parser itself is serial.
- **Detect:** `schedule(static)` over `T` timestamps. Each thread owns a private hit list; results are merged and sorted.
- **Trade:** sequential. The book is shared mutable state.

## Measured scaling (Apple M5 Pro, 15 cores)

| Threads | Detect (ms) | Speedup | Efficiency | Timestamps/s |
|---|---:|---:|---:|---:|
| 1 | 13.49 | 0.98× | 0.98 | 6.41 M |
| 2 | 6.95 | 1.90× | 0.95 | 12.4 M |
| 4 | 3.91 | 3.36× | 0.84 | 22.1 M |
| 8 | 2.28 | 5.77× | 0.72 | 37.9 M |
| 12 | 1.68 | 7.84× | 0.65 | 51.5 M |
| 15 | 1.92 | 6.87× | 0.46 | 45.1 M |

Peak at 12 threads. 15 threads oversubscribe relative to the 1.7 ms kernel. Karp–Flatt serial fraction at 12 threads is about **4.8%**. Full discussion and figures: [`project_description.md`](../project_description.md).
