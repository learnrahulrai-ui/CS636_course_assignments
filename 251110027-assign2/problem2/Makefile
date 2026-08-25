# --- Compiler & Environment Settings ---
CXX = g++
CXXFLAGS = -std=c++11 -pthread -O3

# --- Benchmark & Build Options ---
THREADS ?= 1
OPS ?= 100000
CAS ?= 128

# --- Dynamic CAS Configuration ---
ifeq ($(CAS), 128)
	CAS_FLAGS = -mcx16 -DCAS_128
	QUEUE_MSG = "Building 128-bit CAS version..."
else
	CAS_FLAGS = -DCAS_64
	QUEUE_MSG = "Building 64-bit CAS version (Tagged Pointers)..."
endif

# --- Executable Names ---
TARGET_MY_QUEUE = my_queue
TARGET_BOOST = boost_queue

# --- Helper Menu (Default Target) ---
.PHONY: help all run_my_queue run_boost compare clean force_rebuild

help:
	@echo "================================================================"
	@echo "             Lock-Free Queue Benchmark Makefile                 "
	@echo "================================================================"
	@echo "Commands:"
	@echo "  make all          - Compiles both queues (Default CAS: $(CAS))"
	@echo "  make run_my_queue - Runs MyQueue queue (Default: $(THREADS) threads, $(OPS) ops)"
	@echo "  make run_boost    - Runs Boost queue (Default: $(THREADS) threads, $(OPS) ops)"
	@echo "  make compare      - Runs both back-to-back for easy terminal benchmarking"
	@echo "  make clean        - Deletes the compiled executables"
	@echo ""
	@echo "How to compile with different CAS: "
	@echo "  make all CAS=64"
	@echo ""
	@echo "How to run with custom parameters: "
	@echo "  make compare THREADS=1 OPS=100000 CAS=128"
	@echo "================================================================"

# --- Build Targets ---

all: $(TARGET_MY_QUEUE) $(TARGET_BOOST)

# We use force_rebuild so changing CAS=64 to CAS=128 always triggers a fresh compile
$(TARGET_MY_QUEUE): force_rebuild
	@echo $(QUEUE_MSG)
	$(CXX) $(CXXFLAGS) $(CAS_FLAGS) main_251110027.cpp -o $(TARGET_MY_QUEUE) -latomic

$(TARGET_BOOST): main_boost_251110027.cpp
	$(CXX) $(CXXFLAGS) main_boost_251110027.cpp -o $(TARGET_BOOST)

# --- Run Targets ---

run_my_queue: $(TARGET_MY_QUEUE)
	./$(TARGET_MY_QUEUE) $(THREADS) $(OPS)

run_boost: $(TARGET_BOOST)
	./$(TARGET_BOOST) $(THREADS) $(OPS)

compare: all
	@echo "========================================"
	@echo "   BENCHMARK: Custom $(CAS)-bit Queue vs Boost "
	@echo "   Threads: $(THREADS) | Operations: $(OPS)  "
	@echo "========================================"
	@./$(TARGET_MY_QUEUE) $(THREADS) $(OPS)
	@./$(TARGET_BOOST) $(THREADS) $(OPS)
	@echo "========================================"

# --- Cleanup ---
clean:
	rm -f $(TARGET_MY_QUEUE) $(TARGET_BOOST) *.exe *.out *.png

force_rebuild: