#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <pthread.h>
#include <chrono>
#include <cassert>

// Dynamically choose the correct header based on Makefile flags
#ifdef CAS_64
    #include "lfq_64CAS_251110027.h"
#else
    #include "lfq_128CAS_251110027.h"
#endif

struct Operation {
    int type;
    int value;
};

struct ThreadArgs {
    LFQueue* queue;
    const std::vector<Operation>* global_ops;
    int start_idx;
    int end_idx;
};

void test1() {
    LFQueue q;
    assert(q.enq(10) == true);
    assert(q.enq(20) == true);
    q.print();
    std::cout << "Size after 2 enqueues: " << q.size() << std::endl;
    assert(q.deq() == 10);
    assert(q.deq() == 20);
    assert(q.deq() == -1);
    std::cout << "[OK] test1() passed: Basic sequential enq/deq.\n";
}

void test2() {
    LFQueue q;
    assert(q.deq() == -1);
    for(int i = 0; i < 100; i++) q.enq(i);
    for(int i = 0; i < 100; i++) assert(q.deq() == i);
    assert(q.deq() == -1);
    std::cout << "[OK] test2() passed: Edge cases and bulk sequential processing.\n";
}

void* worker(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    LFQueue* queue = args->queue;
    const std::vector<Operation>& ops = *(args->global_ops);

    for (int i = args->start_idx; i < args->end_idx; ++i) {
        if (ops[i].type == 1) {
            queue->enq(ops[i].value);
        } else {
            queue->deq();
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    // Run Unit Tests First
    std::cout << "--- Running Unit Tests ---\n";
    test1();
    test2();
    std::cout << "--------------------------\n\n";

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <total_operations>\n";
        return 1;
    }

    int num_threads = std::atoi(argv[1]);
    int total_operations = std::atoi(argv[2]);

    // Pre-generate the sequence of operations same seed for both main and boost version
    std::vector<Operation> sequence(total_operations);
    std::mt19937 gen(42);
    std::uniform_int_distribution<> action_dist(0, 1);
    std::uniform_int_distribution<> value_dist(1, 1000);

    for (int i = 0; i < total_operations; ++i) {
        sequence[i].type = action_dist(gen);
        sequence[i].value = value_dist(gen);
    }

    LFQueue queue;
    std::vector<pthread_t> threads(num_threads);
    std::vector<ThreadArgs> thread_args(num_threads);
    int ops_per_thread = total_operations / num_threads;
    int remaining_ops = total_operations % num_threads;

    int current_idx = 0;
    for (int i = 0; i < num_threads; ++i) {
        int ops_for_this_thread = ops_per_thread + (i == 0 ? remaining_ops : 0);
        thread_args[i] = {&queue, &sequence, current_idx, current_idx + ops_for_this_thread};
        current_idx += ops_for_this_thread;
    }

    std::cout << "Starting Benchmark with " << num_threads << " threads and " << total_operations << " ops...\n";
    std::cout << "Number of operations per thread: " << ops_per_thread << " (with " << remaining_ops << " extra ops for thread 0)\n";

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, worker, &thread_args[i]);
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    std::cout << "[MY QUEUE] Time Taken: " << elapsed.count() << " ms\n";

    return 0;
}