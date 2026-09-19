.PHONY: all sequential openmp cuda test bench plots run clean

all: sequential openmp cuda

sequential:
	$(MAKE) -C project-root-sequential

openmp:
	$(MAKE) -C project-root-openmp

cuda:
	$(MAKE) -C project-root-cuda

test:
	$(MAKE) -C project-root-sequential test
	$(MAKE) -C project-root-openmp test
	$(MAKE) -C project-root-cuda test

run:
	$(MAKE) -C project-root-sequential run
	$(MAKE) -C project-root-openmp run-parallel
	$(MAKE) -C project-root-cuda run

bench:
	$(MAKE) -C project-root-sequential bench-csv
	$(MAKE) -C project-root-openmp bench-csv
	$(MAKE) -C project-root-cuda bench-csv

plots: bench
	.venv/bin/python scripts/generate_plots.py --out docs/figures

clean:
	$(MAKE) -C project-root-sequential clean
	$(MAKE) -C project-root-openmp clean
	$(MAKE) -C project-root-cuda clean
