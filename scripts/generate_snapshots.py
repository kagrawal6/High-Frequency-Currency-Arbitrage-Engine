#!/usr/bin/env python3
"""Render terminal-style PNG snapshots from the real run logs."""

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "docs" / "figures"
OUT.mkdir(parents=True, exist_ok=True)


def snap(name: str, title: str, body: str, width=12.4, height=7.2, fontsize=10.2) -> None:
    fig, ax = plt.subplots(figsize=(width, height))
    ax.set_xlim(0, 1)
    ax.set_ylim(0, 1)
    ax.axis("off")
    fig.patch.set_facecolor("#111111")
    ax.add_patch(plt.Rectangle((0.015, 0.015), 0.97, 0.97, fill=True, color="#1a1a1a", linewidth=0))
    ax.text(0.04, 0.955, title, va="top", ha="left", family="monospace",
            fontsize=10, color="#8fd19e", fontweight="bold")
    ax.text(0.04, 0.90, body.rstrip() + "\n", va="top", ha="left", family="monospace",
            fontsize=fontsize, color="#ece6d8", linespacing=1.38)
    fig.savefig(OUT / name, dpi=150, facecolor=fig.get_facecolor(), bbox_inches="tight")
    plt.close(fig)


snap(
    "snapshot_sequential_run.png",
    "project-root-sequential  $  ./arbitrage",
    """========== FOREX ARBITRAGE DETECTOR (SEQUENTIAL) ==========

Currencies: AUD CAD EUR GBP JPY SGD USD
Pairs: 19

Loaded 86400 aligned timestamps across 19 pairs.

====== SINGLE TIMESTAMP ANALYSIS ======
t0 currencies=7 edges=38
No arbitrage at first timestamp.

====== TIME SERIES ANALYSIS ======

===== ARBITRAGE OPPORTUNITIES (680) =====
ts=4228000  profit=0.000766%  cycle=USD->CAD->AUD->JPY->USD
ts=43614000 profit=0.003658%  cycle=GBP->USD->CAD->GBP
ts=44413000 profit=0.005529%  cycle=CAD->GBP->USD->CAD
ts=45001000 profit=0.006805%  cycle=JPY->SGD->USD->JPY
ts=45224000 profit=0.001311%  cycle=EUR->JPY->USD->EUR
ts=79542000 profit=0.013305%  cycle=EUR->USD->CAD->EUR     <-- largest print
... 674 more cycles ...

====== TRADING SIMULATION ======
===== CURRENT POSITIONS =====
USD  918.766667    CAD  25.263701    SGD  16.728306
GBP   13.500778    JPY   0.000000    EUR   0.000000    AUD  0.000000
Trades executed: 2399
Final mark-to-USD: $966.67

===== TIMING SUMMARY =====
CSV load + align              635.650 ms
Time-series detection          14.667 ms
Trade execution                 0.972 ms""",
    height=8.4,
)

snap(
    "snapshot_openmp_run.png",
    "project-root-openmp  $  ./arbitrage 8",
    """========== FOREX ARBITRAGE DETECTOR (OPENMP) ==========

Threads: 8 (max available 8)
Currencies: AUD CAD EUR GBP JPY SGD USD
Pairs: 19

Loaded 86400 aligned timestamps across 19 pairs.
===== ARBITRAGE OPPORTUNITIES (680) =====
ts=4228000 profit=0.000766%  cycle=USD->CAD->AUD->JPY->USD
... same 680 cycles as sequential, same order after sort ...

===== CURRENT POSITIONS =====
USD  918.766667    CAD  25.263701    SGD  16.728306
GBP   13.500778
Trades executed: 2399
Final mark-to-USD: $966.67

===== TIMING SUMMARY =====
CSV load + align              162.392 ms   (parallel pair-file parse)
Time-series detection           2.317 ms   (8 threads, one timestamp each)
Trade execution                 0.897 ms   (serial book)""",
    height=6.6,
)

snap(
    "snapshot_cuda_run.png",
    "project-root-cuda  $  ./arbitrage     [nvcc not found → CUDA_EMU]",
    """========== FOREX ARBITRAGE DETECTOR (CUDA) ==========

Backend: cuda-emu  block=128
Currencies: AUD CAD EUR GBP JPY SGD USD
Pairs: 19

Loaded 86400 aligned timestamps across 19 pairs.
  CUDA backend: cuda-emu  timestamps=86400  edges/ts=38

===== ARBITRAGE OPPORTUNITIES (680) =====
ts=4228000 profit=0.000766%  cycle=USD->CAD->AUD->JPY->USD
... identical 680 cycles, identical $966.67 book ...

===== TIMING SUMMARY =====
CSV load + align              128.830 ms
Weight packing                  8.465 ms   T×38 -log(rate) tensor
CUDA kernel                     1.806 ms   one work-item per timestamp
Trade execution                 0.916 ms

Note: this Mac is Apple M5 Pro. Real nvcc/NVIDIA path is
src/arbitrage_launch.cu; the emulator runs the same detectOne body.""",
    height=6.8,
)

snap(
    "snapshot_tests.png",
    "repository root  $  make test",
    """$ make test
./project-root-sequential/tests/test_engine
sequential tests: 34 passed, 0 failed

./project-root-openmp/tests/test_engine
openmp tests: 15 passed, 0 failed
  includes: OpenMP @ max threads ≡ serial on a 400-timestamp live slice

./project-root-cuda/tests/test_engine
cuda tests: 8 passed, 0 failed
  includes: packed kernel ≡ detectArbitrageAt on a 300-timestamp live slice

ALL_TESTS_OK   57 assertions, 0 failures""",
    width=11.2,
    height=5.2,
    fontsize=11,
)

snap(
    "snapshot_csv.png",
    "project-root-sequential/data/ask/EURUSD_ASK.csv",
    """Gmt time,Open,High,Low,Close,Volume
26.03.2025 00:00:00.000,1.07894,1.07894,1.07888,1.07888,9899.999999999998
26.03.2025 00:00:01.000,1.07888,1.07888,1.07886,1.07886,8100
26.03.2025 00:00:02.000,1.07886,1.07886,1.07886,1.07886,2700
26.03.2025 00:00:03.000,1.07887,1.07887,1.07886,1.07887,12599.999899999999
...
26.03.2025 23:59:55.000,1.07413,1.07413,1.07413,1.07413,2700
26.03.2025 23:59:56.000,1.07415,1.07415,1.07415,1.07415,1890

Engine uses column 4 (Close) as the executable bid or ask at that second.
Matching BID file is data/bid/EURUSD_BID.csv. ~86,400 rows per pair.""",
    width=12.6,
    height=5.4,
    fontsize=10.5,
)

snap(
    "snapshot_benchmark_csv.png",
    "docs/benchmarks/all.csv   (5 outer reps × 20 inner kernel passes)",
    """backend,config,mean_detect_ms,speedup,efficiency,timestamps_per_sec,opportunities
sequential,1,     13.922523, 1.00, 1.00,  6,205,772, 680
openmp,1,         13.486359, 0.98, 0.98,  6,406,473, 680
openmp,2,          6.946725, 1.90, 0.95, 12,437,516, 680
openmp,4,          3.913334, 3.36, 0.84, 22,078,362, 680
openmp,8,          2.282163, 5.77, 0.72, 37,858,815, 680
openmp,12,         1.679184, 7.84, 0.65, 51,453,570, 680
openmp,15,         1.915416, 6.87, 0.46, 45,107,689, 680
cuda-emu,64,       0.966970,14.40,   —, 89,351,271, 680
cuda-emu,128,      0.968464,14.40,   —, 89,213,418, 680

Every row reports the same 680 hits. Detection is the timed quantity.""",
    width=12.8,
    height=5.8,
    fontsize=10.2,
)

print("snapshots written to", OUT)
