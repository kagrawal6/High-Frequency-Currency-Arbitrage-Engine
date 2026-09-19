# Output

**You are in** the repo-root `output/` folder (the filled one).  
Canonical copies also live in `docs/figures`, `docs/logs`, and `docs/benchmarks`.  
How to read the project: [../README.md](../README.md) · report: [../project_description.md](../project_description.md)

Results from the close-out runs live here.

| Path | Contents |
|---|---|
| `figures/` | Benchmark PNG/PDFs (detection time, speedup, efficiency, throughput, stages, profits) |
| `logs/` | Full stdout of sequential, OpenMP, and CUDA production runs |
| `benchmarks/` | Combined and per-backend CSVs |

These are copies of `docs/figures`, `docs/logs`, and `docs/benchmarks`. Regenerating:

```bash
make plots
```
