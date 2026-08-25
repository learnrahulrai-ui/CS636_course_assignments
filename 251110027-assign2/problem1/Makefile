CXX = g++
CXXFLAGS = -O2 -std=c++17 -pthread

# Default parameters for running (can be overridden from command line)
THREADS ?= 4
OPS ?= 1000000

TARGETS = pthreads_sum pthreads_vec abq_sum abq_vec mcs_sum mcs_vec

..PHONY: default help all clean experiment run_pthreads_sum run_pthreads_vec run_abq_sum run_abq_vec run_mcs_sum run_mcs_vec

default: help

all: $(TARGETS)

# --- Compile Targets ---

pthreads_sum: target_program1_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

pthreads_vec: target_program2_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

abq_sum: target_with_abq1_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

abq_vec: target_with_abq2_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

mcs_sum: target_with_mcs1_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

mcs_vec: target_with_mcs2_251110027.cpp
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Run Targets ---

run_pthreads_sum: pthreads_sum
	./pthreads_sum $(THREADS) $(OPS)

run_pthreads_vec: pthreads_vec
	./pthreads_vec $(THREADS) $(OPS)

run_abq_sum: abq_sum
	./abq_sum $(THREADS) $(OPS)

run_abq_vec: abq_vec
	./abq_vec $(THREADS) $(OPS)

run_mcs_sum: mcs_sum
	./mcs_sum $(THREADS) $(OPS)

run_mcs_vec: mcs_vec
	./mcs_vec $(THREADS) $(OPS)

# --- Automation Target ---
experiment:
	@echo "Running full automated scaling experiments..."
	bash run_experiments_251110027.sh

# --- Help Target ---
help:
	@echo "================================================================"
	@echo "                  Concurrency Lock Evaluator                    "
	@echo "================================================================"
	@echo ""
	@echo "Available commands:"
	@echo "  make all              : Compile all 6 programs"
	@echo "  make clean            : Remove all executables and outputs"
	@echo "  make help             : Show this help message"
	@echo ""
	@echo "Run Commands (automatically compiles if needed):"
	@echo "  make run_pthreads_sum : Run Pthreads Shared Sum"
	@echo "  make run_pthreads_vec : Run Pthreads Shared Vector"
	@echo "  make run_abq_sum      : Run ABQ Shared Sum"
	@echo "  make run_abq_vec      : Run ABQ Shared Vector"
	@echo "  make run_mcs_sum      : Run MCS Shared Sum"
	@echo "  make run_mcs_vec      : Run MCS Shared Vector"
	@echo ""
	@echo "  make experiment       : Run the full 1-32 thread with weak and strong scaling bash script"
	@echo ""
	@echo "Customizing Parameters:"
	@echo "  You can override the default THREADS (4) and OPS (1000000)."
	@echo "  Example: make run_mcs_sum THREADS=16 OPS=5000000"
	@echo "================================================================"

clean:
	rm -f $(TARGETS) *.png *.exe *.out *.csv