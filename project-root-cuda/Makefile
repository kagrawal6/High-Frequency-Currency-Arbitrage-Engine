# Build the CUDA engine with nvcc when present, otherwise the host emulator
# of the same kernel (one work-item per timestamp).
NVCC := $(shell command -v nvcc 2>/dev/null)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  LLVM_CXX := $(shell test -x /opt/homebrew/opt/llvm/bin/clang++ && echo /opt/homebrew/opt/llvm/bin/clang++)
  ifneq ($(LLVM_CXX),)
    CXX := $(LLVM_CXX)
  endif
endif

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Iinclude
LDFLAGS  ?=

HOST_SRC := \
    src/ArbitrageDetector.cpp \
    src/CsvParser.cpp \
    src/ForexGraph.cpp \
    src/MarketData.cpp \
    src/PackedWeights.cpp \
    src/PositionsManager.cpp \
    src/TimeSeriesArbitrageDetector.cpp \
    src/Timer.cpp

HOST_OBJ := $(HOST_SRC:.cpp=.o)

.PHONY: all run benchmark bench-csv test plots clean

ifeq ($(NVCC),)
  CXXFLAGS += -DCUDA_EMU -fopenmp
  LDFLAGS  += -fopenmp
  KERNEL_OBJ := src/arbitrage_launch_emu.o
  $(info Building CUDA emulator (nvcc not found))
else
  NVCCFLAGS ?= -O3 -std=c++17 -Iinclude
  KERNEL_OBJ := src/arbitrage_launch.o
  $(info Building with nvcc: $(NVCC))
endif

all: arbitrage

arbitrage: main.o $(HOST_OBJ) $(KERNEL_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

benchmark: benchmark.o $(HOST_OBJ) $(KERNEL_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tests/test_engine: tests/test_engine.o $(HOST_OBJ) $(KERNEL_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

src/arbitrage_launch.o: src/arbitrage_launch.cu
	$(NVCC) $(NVCCFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: arbitrage
	./arbitrage

bench-csv: benchmark
	@mkdir -p benchmarks
	./benchmark > benchmarks/benchmark_results.csv
	@cat benchmarks/benchmark_results.csv

test: tests/test_engine
	./tests/test_engine

plots: bench-csv
	python3 ../scripts/generate_plots.py --cuda benchmarks/benchmark_results.csv --out output

clean:
	rm -f $(HOST_OBJ) $(KERNEL_OBJ) main.o benchmark.o tests/test_engine.o \
	      arbitrage benchmark tests/test_engine
