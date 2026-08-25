# CS636 Course Assignments

This folder contains my solutions for the two CS636 assignments. Together, they explore two complementary sides of concurrent systems:

- **Assignment 1:** observing and detecting incorrect thread interactions through dynamic data-race detection.
- **Assignment 2:** designing and evaluating synchronization mechanisms and non-blocking concurrent data structures.

The implementations are written primarily in C++ and use POSIX threads, C++ atomics, LLVM instrumentation, vector clocks, epochs, and reproducible benchmark drivers.

## Assignment 1 - Dynamic Data-Race Detection

### What the assignment means

The goal is to detect data races in multithreaded programs. A data race occurs when two threads access the same memory location concurrently, at least one access is a write, and there is no happens-before relationship ordering the accesses.

The assignment studies two approaches:

1. **DJIT+**, an offline detector that analyzes an execution trace after the program finishes.
2. **FastTrack**, an online detector that instruments a program and checks accesses while it runs.

The central challenge is distinguishing genuinely unordered accesses from accesses that are safe because of locks, thread creation, or thread joins.

### What I implemented

#### 1. DJIT+ offline analyzer

The implementation in [`djit/251110027_djit.cpp`](251110027-assign1/djit/251110027_djit.cpp) parses trace files containing memory, lock, and thread events.

The solution:

- Performs a first pass over the trace to count the number of threads.
- Allocates fixed-size vector clocks for threads, locks, and shared variables.
- Performs a second pass to analyze the trace using vector-clock joins and happens-before checks.
- Models `pthread_create`, `pthread_join`, lock acquisition, and lock release.
- Tracks the last read and write history for each memory address.
- Reports write-read, write-write, and read-write races, grouped by instruction pointers, address, access type, and thread pair.
- Handles both possible orderings of a join: the child may finish before the parent calls join, or the parent may reach join first. A pending-join counter and completed-thread queue defer the vector-clock merge until the join relationship is semantically valid.

The supplied trace corpus includes both race-free and intentionally racy examples, including fork/join cases and multiple-race cases. They are available under [`djit/logs`](251110027-assign1/djit/logs).

#### 2. FastTrack online detector

The FastTrack implementation is split into an LLVM pass and a runtime:

- [`fasttrack/251110027_llvmpass.cpp`](251110027-assign1/fasttrack/251110027_llvmpass.cpp) instruments LLVM IR.
- [`fasttrack/251110027_fasttrack.cpp`](251110027-assign1/fasttrack/251110027_fasttrack.cpp) implements the runtime race detector.
- [`fasttrack/Makefile`](251110027-assign1/fasttrack/Makefile) automates the build pipeline.

The LLVM pass inserts runtime hooks for:

- `load` and `store` instructions;
- mutex acquisition after `pthread_mutex_lock` returns;
- mutex release before `pthread_mutex_unlock` executes;
- thread joins after `pthread_join` returns; and
- thread creation through a wrapper that captures the parent context before the child starts.

The runtime uses the FastTrack representation:

- An **epoch** packs a thread ID and scalar clock into a 64-bit value for the common case.
- A variable remains in an exclusive-read state when possible.
- Concurrent readers cause a transition to a read-shared state backed by a vector clock map.
- Writes check both the most recent writer and previous readers.
- Per-thread, per-variable, and per-lock mutexes protect the detector's own metadata while application threads execute concurrently.

The Makefile builds the runtime object, LLVM pass plugin, target bitcode, instrumented bitcode, and final executable. Example usage from the FastTrack directory:

```bash
make TARGET_SRC=codes/racy1.cpp
make run
```

### What I learned from Assignment 1

- **Happens-before is the key abstraction:** vector clocks provide a practical way to represent causal ordering across threads and synchronization objects.
- **Offline and online detection make different trade-offs:** DJIT+ can pre-size state from the complete trace, while FastTrack must remain efficient during execution.
- **Epochs are an important optimization:** most accesses do not need a full vector clock, so epoch-based state reduces the common-case overhead.
- **Instrumentation must respect program semantics:** the exact placement of hooks matters. A lock is considered acquired only after the call succeeds, an unlock is modeled before release, and a join synchronizes only after the child has terminated.
- **Thread creation is more than a function call:** the child must inherit the parent's clock at the right instant, which motivated the wrapper and context-capture design.
- **The detector also needs synchronization:** race detection metadata is shared by the instrumented threads, so the runtime must protect its own state without confusing detector locks with application locks.

## Assignment 2 - Locks and Lock-Free Queues

### What the assignment means

This assignment moves from detecting concurrency errors to building concurrency mechanisms. It asks how different synchronization strategies affect correctness, fairness, scalability, and implementation complexity.

It has two parts:

1. Implement and evaluate queue locks against the standard POSIX mutex.
2. Implement an unbounded, total, lock-free queue and address the ABA problem using tagged pointers and CAS.

### Problem 1 - ABQ and MCS queue locks

#### What I implemented

Two lock-protected workloads were implemented in three versions each:

- **Shared sum:** threads repeatedly add values to one shared `long`.
- **Shared vector append:** threads repeatedly append their thread IDs to a shared `std::vector`.
- **Pthreads baseline:** uses `pthread_mutex_t`.
- **ABQ version:** uses the custom Array-Based Queue Lock in [`abql_251110027.h`](251110027-assign2/problem1/abql_251110027.h).
- **MCS version:** uses the custom MCS queue lock in [`mcs_251110027.h`](251110027-assign2/problem1/mcs_251110027.h).

Each program accepts the number of threads and total work as command-line arguments. Correctness is checked with assertions: the final sum must match the arithmetic expectation, and the vector must have the expected size and exact count for every thread ID.

The custom lock implementations include:

- 64-byte alignment and padding to reduce false sharing;
- acquire/release memory ordering for lock handoff; and
- `std::this_thread::yield()` in spin loops to reduce scheduler convoying when the machine is oversubscribed.

The experiment script [`run_experiments_251110027.sh`](251110027-assign2/problem1/run_experiments_251110027.sh) evaluates strong and weak scaling for 1 through 32 threads. [`plot_results_251110027.py`](251110027-assign2/problem1/plot_results_251110027.py) generates the comparison plots.

#### Main observations

- With up to the machine's available logical cores, ABQ and MCS behave reasonably for the tested workloads.
- Beyond 16 threads, the busy-waiting queue locks show a pronounced slowdown because of oversubscription and the convoy effect.
- Yielding allows preempted lock holders to run again, but it does not remove the fundamental cost of spinning under oversubscription.
- The POSIX mutex remains the most efficient option in these experiments across the higher thread counts.

### Problem 2 - Unbounded lock-free queue

#### What I implemented

The queue implementation is based on a linked list with atomic head and tail state and supports:

```cpp
bool enq(int v);
int deq();
void print();
int size();
```

`enq` uses CAS to attach a new node, while `deq` advances the head without taking a lock. An empty queue returns `-1` instead of blocking.

Two ABA-resistant versions were implemented:

- [`lfq_64CAS_251110027.h`](251110027-assign2/problem2/lfq_64CAS_251110027.h) packs a pointer and a counter into one 64-bit atomic value. The pointer occupies the lower 48 bits and the counter occupies the upper 16 bits.
- [`lfq_128CAS_251110027.h`](251110027-assign2/problem2/lfq_128CAS_251110027.h) stores a pointer and 64-bit counter together in an aligned 16-byte structure and uses 128-bit CAS with `-mcx16`.

The driver [`main_251110027.cpp`](251110027-assign2/problem2/main_251110027.cpp) includes deterministic sequential tests, generates a shared operation sequence with `std::mt19937` seeded with `42`, distributes the same workload across threads, and compares the custom queue with the Boost lock-free queue through [`main_boost_251110027.cpp`](251110027-assign2/problem2/main_boost_251110027.cpp).

The benchmark suite tests 10^5, 10^6, and 10^7 operations with 1, 2, 4, 8, and 16 threads. The available result image is [`scalability_plot.png`](251110027-assign2/problem2/scalability_plot.png).

One important implementation trade-off is memory reclamation. Immediately deleting a dequeued node can cause use-after-free in another thread that still holds a reference to it. The current implementation therefore does not reclaim every removed node during `deq`; this avoids unsafe reclamation for the assignment's correctness and performance tests, at the cost of leaked memory until process termination.

### What I learned from Assignment 2

- **Fairness and scalability are different properties:** ABQ and MCS provide queue-based ordering, but fair spinning can still perform poorly when threads outnumber CPU cores.
- **False sharing can dominate lock performance:** cache-line padding is a low-level optimization with a direct effect on concurrent lock handoff.
- **Memory ordering must match the synchronization protocol:** acquire/release operations can reduce unnecessary barriers while preserving visibility between lock owners.
- **Lock-free does not mean simple:** a queue can avoid locks and still require careful helping, CAS retry loops, pointer consistency, and ABA protection.
- **ABA is a representation problem as well as an algorithm problem:** tagging the pointer with a version counter makes a stale CAS observation distinguishable from a newer reuse of the same address.
- **Hardware and toolchain support matter:** the 128-bit version depends on 16-byte alignment, compiler support, `-mcx16`, and linking with `-latomic`.
- **Memory reclamation is separate from lock-free progress:** safe deletion requires techniques such as hazard pointers, epochs, or a suitable reclamation scheme; CAS correctness alone is not enough.
- **Reproducible benchmarks are essential:** using the same seeded operation sequence for the custom and Boost queues makes the comparison more meaningful.

## Overall takeaway

These assignments connected the theory and practice of concurrent programming. Assignment 1 focused on **reasoning about ordering**: reconstructing happens-before relationships and identifying accesses that are not ordered. Assignment 2 focused on **enforcing and exploiting ordering**: building locks, reducing contention costs, and using atomic primitives to achieve non-blocking progress.

The combined experience reinforced that concurrent-system performance depends on more than the nominal algorithm. Correctness, memory ordering, cache behavior, scheduler behavior, compiler and hardware support, instrumentation overhead, and memory reclamation all influence the final design.

## Repository map

```text
course_assignments/
├── 251110027-assign1/
│   ├── 251110027-assign1.pdf
│   ├── djit/
│   │   ├── 251110027_djit.cpp
│   │   └── logs/
│   └── fasttrack/
│       ├── 251110027_fasttrack.cpp
│       ├── 251110027_llvmpass.cpp
│       ├── Makefile
│       └── codes/
└── 251110027-assign2/
    ├── 251110027-assign2.pdf
    ├── Makefile
    ├── problem1/
    └── problem2/
```

