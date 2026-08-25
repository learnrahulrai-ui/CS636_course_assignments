#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <pthread.h>
#include <chrono>
#include <cassert>
#include <boost/lockfree/queue.hpp>

struct Operation {
    int type;  
    int value; 
};

struct ThreadArgs {
    boost::lockfree::queue<int>* queue;
    const std::vector<Operation>* global_ops;
    int start_idx;
    int end_idx;
};

void* worker(void* arg) {
    ThreadArgs* args = static_cast<ThreadArgs*>(arg);
    boost::lockfree::queue<int>* queue = args->queue;
    const std::vector<Operation>& ops = *(args->global_ops);

    for (int i = args->start_idx; i < args->end_idx; ++i) {
        if (ops[i].type == 1) {
            queue->push(ops[i].value);
        } else {
            int dummy;
            queue->pop(dummy);
        }
    }
    return nullptr;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <num_threads> <total_operations>\n";
        return 1;
    }

    int num_threads = std::atoi(argv[1]);
    int total_operations = std::atoi(argv[2]);

    // Same deterministic sequence generation
    std::vector<Operation> sequence(total_operations);
    std::mt19937 gen(42); 
    std::uniform_int_distribution<> action_dist(0, 1);
    std::uniform_int_distribution<> value_dist(1, 1000);

    for (int i = 0; i < total_operations; ++i) {
        sequence[i].type = action_dist(gen);
        sequence[i].value = value_dist(gen);
    }

    // Boost Queue initialization
    // We pass total_operations to pre-allocate memory to avoid mid-test allocation latency
    boost::lockfree::queue<int> queue(total_operations); 
    
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

    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; ++i) {
        pthread_create(&threads[i], nullptr, worker, &thread_args[i]);
    }
    for (int i = 0; i < num_threads; ++i) {
        pthread_join(threads[i], nullptr);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end_time - start_time;

    std::cout << "[BOOST] Time Taken: " << elapsed.count() << " ms\n";

    return 0;
}