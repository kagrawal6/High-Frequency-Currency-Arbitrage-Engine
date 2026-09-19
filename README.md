# High-Frequency Currency Arbitrage Engine

Detects locked-in FX arbitrage cycles on a full day of second-resolution bid/ask ticks, then optionally walks a paper book through every hit. Three backends run the **same** detector: sequential C++, OpenMP, and CUDA.

| | Sequential | OpenMP @ 12 | CUDA-EMU kernel |
|---|---:|---:|---:|
| Detect 86,400 timestamps | 13.92 ms | 1.68 ms (7.84×) | 0.97 ms (14.4×) |
| Opportunities found | 680 | 680 | 680 |
| Paper trades / final USD | 2,399 / $966.67 | 2,399 / $966.67 | 2,399 / $966.67 |

Measured on Apple M5 Pro (15 cores). CUDA on this Mac is the host emulator of the real `.cu` kernel — there is no NVIDIA GPU here.

---

## Start here

| If you want… | Open this |
|---|---|
| **The full write-up** (algorithm, snapshots of every run, graphs, tests) | [`project_description.md`](project_description.md) |
| **Pictures of the programs actually running** | [`output/`](output/) or [`docs/figures/`](docs/figures/) |
| **Full terminal transcripts** | [`docs/logs/`](docs/logs/) · also [`output/logs/`](output/logs/) |
| **Benchmark CSVs** | [`docs/benchmarks/`](docs/benchmarks/) |
| Sequential code + how to build it | [`project-root-sequential/README.md`](project-root-sequential/README.md) |
| OpenMP code + scaling notes | [`project-root-openmp/README.md`](project-root-openmp/README.md) |
| CUDA kernel + `nvcc` / emulator notes | [`project-root-cuda/README.md`](project-root-cuda/README.md) |

Quick commands from this directory:

```bash
make            # build all three engines
make test       # 57 assertions: sequential 34, OpenMP 15, CUDA 8
make run        # full-day detect + trade on each backend
```

One backend only:

```bash
cd project-root-sequential && make run
cd project-root-openmp     && make run-parallel PARALLEL_THREADS=8
cd project-root-cuda       && make run
```

---

## Repository structure

There is **no code at the repo root** except the umbrella `Makefile`. Everything that compiles lives in one of the three `project-root-*` folders. They are independent copies of the same pipeline; only the parallel backend changes.

```
High-Frequency-Currency-Arbitrage-Engine/
│
├── README.md                      ← you are here
├── project_description.md         ← long-form report + snapshots + graphs
├── Makefile                       ← make / test / run / plots for all three
│
├── project-root-sequential/       ← 1. baseline (one thread)
├── project-root-openmp/           ← 2. OpenMP (renamed from project-root-parallel)
├── project-root-cuda/             ← 3. CUDA kernel + host emulator
│
├── docs/
│   ├── figures/                   ← PNG + PDF plots and terminal snapshots
│   ├── logs/                      ← raw stdout of the three production runs
│   └── benchmarks/                ← sequential.csv, openmp.csv, cuda.csv, all.csv
├── output/                        ← same figures, logs, and CSVs (easy to browse)
└── scripts/                       ← generate_plots.py, generate_snapshots.py
```

Each `project-root-*` folder has the same internal layout:

```
project-root-<backend>/
├── Makefile
├── README.md
├── main.cpp                       ← CLI: load → detect → trade
├── benchmark.cpp                  ← official timing driver
├── include/                       ← headers (CurrencyConfig, MarketData, detector, …)
├── src/                           ← implementation
├── tests/test_engine.cpp          ← correctness suite
├── data/ask  data/bid             ← 19 pair CSVs (CUDA symlinks sequential)
├── benchmarks/                    ← that backend’s CSV
└── output/                        ← copied figures
```

CUDA extra files: `include/ArbitrageKernel.cuh`, `src/arbitrage_launch.cu` (real GPU), `src/arbitrage_launch_emu.cpp` (this Mac).

---

## Currencies and pairs

Seven currencies, **canonical IDs** (stable across timestamps and backends):

| ID | Code | Name |
|---:|---|---|
| 0 | AUD | Australian dollar |
| 1 | CAD | Canadian dollar |
| 2 | EUR | Euro |
| 3 | GBP | Pound sterling |
| 4 | JPY | Japanese yen |
| 5 | SGD | Singapore dollar |
| 6 | USD | United States dollar |

Nineteen spot pairs. Each pair is two files: bid Close and ask Close. Source: [Dukascopy Bank](https://www.dukascopy.com/swiss/english/marketwatch/historical/), 26 March 2025, ~one row per second GMT.

| Pair | Ask file | Bid file |
|---|---|---|
| AUD/CAD | `data/ask/AUDCAD_ASK.csv` | `data/bid/AUDCAD_BID.csv` |
| AUD/JPY | `data/ask/AUDJPY_ASK.csv` | `data/bid/AUDJPY_BID.csv` |
| AUD/SGD | `data/ask/AUDSGD_ASK.csv` | `data/bid/AUDSGD_BID.csv` |
| AUD/USD | `data/ask/AUDUSD_ASK.csv` | `data/bid/AUDUSD_BID.csv` |
| CAD/JPY | `data/ask/CADJPY_ASK.csv` | `data/bid/CADJPY_BID.csv` |
| EUR/AUD | `data/ask/EURAUD_ASK.csv` | `data/bid/EURAUD_BID.csv` |
| EUR/CAD | `data/ask/EURCAD_ASK.csv` | `data/bid/EURCAD_BID.csv` |
| EUR/GBP | `data/ask/EURGBP_ASK.csv` | `data/bid/EURGBP_BID.csv` |
| EUR/JPY | `data/ask/EURJPY_ASK.csv` | `data/bid/EURJPY_BID.csv` |
| EUR/SGD | `data/ask/EURSGD_ASK.csv` | `data/bid/EURSGD_BID.csv` |
| EUR/USD | `data/ask/EURUSD_ASK.csv` | `data/bid/EURUSD_BID.csv` |
| GBP/AUD | `data/ask/GBPAUD_ASK.csv` | `data/bid/GBPAUD_BID.csv` |
| GBP/CAD | `data/ask/GBPCAD_ASK.csv` | `data/bid/GBPCAD_BID.csv` |
| GBP/JPY | `data/ask/GBPJPY_ASK.csv` | `data/bid/GBPJPY_BID.csv` |
| GBP/USD | `data/ask/GBPUSD_ASK.csv` | `data/bid/GBPUSD_BID.csv` |
| SGD/JPY | `data/ask/SGDJPY_ASK.csv` | `data/bid/SGDJPY_BID.csv` |
| USD/CAD | `data/ask/USDCAD_ASK.csv` | `data/bid/USDCAD_BID.csv` |
| USD/JPY | `data/ask/USDJPY_ASK.csv` | `data/bid/USDJPY_BID.csv` |
| USD/SGD | `data/ask/USDSGD_ASK.csv` | `data/bid/USDSGD_BID.csv` |

That is 38 directed edges per timestamp (`bid` one way, `1/ask` the other) and 86,400 aligned seconds.

---

## What the program does

1. **Load** every bid/ask CSV (mmap parse, no `strtok` / `stod`).
2. **Align** ticks onto a packed `T × 19` panel.
3. **Detect** a negative cycle at each second with dummy-source Bellman–Ford on `-log(rate)` weights. A negative cycle means the product of executable rates around the loop is greater than 1.
4. **Trade** (optional): start with $1,000 USD and execute every hit.

It is a detector and paper simulator, not a live order gateway.

---

## How to run (details)

```bash
# all backends
make
make test
make run

# refresh figures after a new bench
python3 -m venv .venv
.venv/bin/pip install matplotlib numpy
make plots
```

Flags (all three `./arbitrage` binaries):

| Flag | Meaning |
|---|---|
| `--max-ts N` | Use only the first N aligned timestamps |
| `--verbose` | Print progress every 10% |
| `--no-trade` | Detect only, skip the book |
| `--capital X` | Starting USD (default 1000) |
| `--threads N` | OpenMP only (or `./arbitrage 8`) |
| `--block B` | CUDA launch block size (default 128) |

macOS OpenMP needs Homebrew LLVM (`brew install llvm`). The OpenMP Makefile already points at `/opt/homebrew/opt/llvm/bin/clang++`. Apple clang has no `-fopenmp`.

---

## Documentation map

1. This file — structure, currencies, how to run.
2. [`project_description.md`](project_description.md) — algorithm, optimizations, **snapshots of every backend run**, benchmark graphs, tests, P&L.
3. Per-backend READMEs in the three `project-root-*` folders.
4. `docs/` and `output/` — artifacts, not source.
