# Plot scripts

`generate_plots.py` reads the three official CSVs and writes PNG + PDF figures into `docs/figures/`.

```bash
python3 -m venv .venv
.venv/bin/pip install matplotlib numpy
.venv/bin/python scripts/generate_plots.py --out docs/figures
```

Inputs default to `docs/benchmarks/{sequential,openmp,cuda}.csv` and `docs/benchmarks/opportunities.json`.
