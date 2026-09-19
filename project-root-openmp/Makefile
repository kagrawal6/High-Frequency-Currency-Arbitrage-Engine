# Prefer Homebrew LLVM on macOS (Apple clang has no -fopenmp).
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LLVM_CXX := $(shell test -x /opt/homebrew/opt/llvm/bin/clang++ && echo /opt/homebrew/opt/llvm/bin/clang++)
  ifneq ($(LLVM_CXX),)
    CXX := $(LLVM_CXX)
  endif
endif

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Iinclude -fopenmp
LDFLAGS  ?= -fopenmp

SRC := \
    src/ArbitrageDetector.cpp \
    src/CsvParser.cpp \
    src/ForexGraph.cpp \
    src/MarketData.cpp \
    src/PositionsManager.cpp \
    src/TimeSeriesArbitrageDetector.cpp \
    src/Timer.cpp

OBJ := $(SRC:.cpp=.o)
PARALLEL_THREADS ?= 4

.PHONY: all run run-seq run-parallel benchmark bench-csv test plots clean

all: arbitrage

arbitrage: main.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

benchmark: benchmark.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tests/test_engine: tests/test_engine.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: run-parallel

run-seq: arbitrage
	./arbitrage 1

run-parallel: arbitrage
	./arbitrage $(PARALLEL_THREADS)

bench-csv: benchmark
	@mkdir -p benchmarks
	./benchmark > benchmarks/benchmark_results.csv
	@cat benchmarks/benchmark_results.csv

test: tests/test_engine
	./tests/test_engine

plots: bench-csv
	python3 ../scripts/generate_plots.py --openmp benchmarks/benchmark_results.csv --out output

clean:
	rm -f $(OBJ) main.o benchmark.o tests/test_engine.o arbitrage benchmark tests/test_engine
