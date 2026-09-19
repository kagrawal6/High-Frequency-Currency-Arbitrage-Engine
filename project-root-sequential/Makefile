CXX      ?= clang++
CXXFLAGS ?= -std=c++17 -O3 -Wall -Wextra -Iinclude
LDFLAGS  ?=

SRC := \
    src/ArbitrageDetector.cpp \
    src/CsvParser.cpp \
    src/ForexGraph.cpp \
    src/MarketData.cpp \
    src/PositionsManager.cpp \
    src/TimeSeriesArbitrageDetector.cpp \
    src/Timer.cpp

OBJ := $(SRC:.cpp=.o)

.PHONY: all run benchmark bench-csv test plots clean

all: arbitrage

arbitrage: main.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

benchmark: benchmark.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

tests/test_engine: tests/test_engine.o $(OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LDFLAGS)

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
	python3 ../scripts/generate_plots.py --sequential benchmarks/benchmark_results.csv --out output

clean:
	rm -f $(OBJ) main.o benchmark.o tests/test_engine.o arbitrage benchmark tests/test_engine
