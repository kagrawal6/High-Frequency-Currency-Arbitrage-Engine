# Benchmarks

`make bench-csv` writes `benchmark_results.csv` in this folder.

Columns: backend, config, mean_wall_ms, stddev_wall_ms, mean_detect_ms, stddev_detect_ms, speedup, efficiency, timestamps, opportunities, mean_profit_usd, timestamps_per_sec.

Detection is the mean of 20 inner passes × 5 outer reps. Combined tables and figures live in ../../docs/.
