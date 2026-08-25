/*
 * Program 2: Shared Vector Append
 * --------------------------------
 * Each thread appends its thread-id (ops_per_thread) times to a shared
 * std::vector. A mutex protects the vector.
 *
 * Correctness: final vector size must equal num_threads * ops_per_thread,
 * and the count of each thread-id in the vector must equal ops_per_thread.
 *
 * Compile: g++ -O2 -std=c++17 -pthread -o shared_vec shared_vec.cpp
 * Run    : ./shared_vec <num_threads> <num_ops>
 */

#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>
#include <algorithm>
#include "abql_251110027.h"

/* ── Shared state ──────────────────────────────────────────────────────── */

static std::vector<int>  g_vec;
static ABQLock *g_lock = nullptr;

/* ── Thread argument ───────────────────────────────────────────────────── */

struct Arg { int thread_id; int ops; };

/* ── Worker ────────────────────────────────────────────────────────────── */

static void* worker(void* arg)
{
    Arg* a = static_cast<Arg*>(arg);

    for (int i = 0; i < a->ops; ++i) {
        size_t my_slot = g_lock->lock();
        g_vec.push_back(a->thread_id);
        g_lock->unlock(my_slot);
    }

    return nullptr;
}

/* ── Main ──────────────────────────────────────────────────────────────── */

int main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_threads> <num_ops>\n", argv[0]);
        return 1;
    }

    int num_threads    = atoi(argv[1]);
    int num_ops        = atoi(argv[2]);
    int ops_per_thread = num_ops / num_threads;

    printf("Shared Vector | threads=%d  ops/thread=%d\n",
           num_threads, ops_per_thread);

    g_lock = new ABQLock(num_threads);

    g_vec.reserve((size_t)num_threads * ops_per_thread);

    std::vector<pthread_t> threads(num_threads);
    std::vector<Arg>       args(num_threads);

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < num_threads; ++i) {
        args[i] = { i, ops_per_thread };
        pthread_create(&threads[i], nullptr, worker, &args[i]);
    }
    for (int i = 0; i < num_threads; ++i)
        pthread_join(threads[i], nullptr);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double ms = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) * 1e-6;
    printf("  Time       = %.3f ms\n", ms);
    printf("  Vector size = %zu  (expected %d)\n",
           g_vec.size(), num_threads * ops_per_thread);

    /* ── Correctness checks ─────────────────────────────────────────────── */

    /* 1. Total element count must be exact. */
    assert((int)g_vec.size() == num_threads * ops_per_thread &&
           "FAIL: vector size is wrong!");

    /* 2. Each thread-id must appear exactly ops_per_thread times. */
    for (int id = 0; id < num_threads; ++id) {
        int cnt = (int)std::count(g_vec.begin(), g_vec.end(), id);
        assert(cnt == ops_per_thread && "FAIL: thread-id count is wrong!");
    }

    printf("  Each thread-id appears exactly %d times in the final vector.\n", ops_per_thread);

    printf("PASS\n");

    delete g_lock;

    return 0;
}