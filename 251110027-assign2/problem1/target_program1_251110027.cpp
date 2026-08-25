/*
 * Program 1: Shared Sum
 * ----------------------
 * Each thread adds numbers 1..ops_per_thread to a single shared sum.
 * A mutex protects the shared sum.
 *
 * Correctness: final sum must equal the expected arithmetic total.
 *
 * Compile: g++ -O2 -std=c++17 -pthread -o shared_sum shared_sum.cpp
 * Run    : ./shared_sum <num_threads> <num_ops>
 */

#include <pthread.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

/* ── Shared state ──────────────────────────────────────────────────────── */

static long             g_sum  = 0;
static pthread_mutex_t  g_lock = PTHREAD_MUTEX_INITIALIZER;

/* ── Thread argument ───────────────────────────────────────────────────── */

struct Arg { int ops; };

/* ── Worker ────────────────────────────────────────────────────────────── */

static void* worker(void* arg)
{
    int ops = static_cast<Arg*>(arg)->ops;

    for (int i = 1; i <= ops; ++i) {
        pthread_mutex_lock(&g_lock);
        g_sum += i;                     /* critical section */
        pthread_mutex_unlock(&g_lock);
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

    int num_threads = atoi(argv[1]);
    int num_ops     = atoi(argv[2]);
    int ops_per_thread = num_ops / num_threads;

    printf("Shared Sum | threads=%d  ops/thread=%d\n",
           num_threads, ops_per_thread);

    std::vector<pthread_t> threads(num_threads);
    std::vector<Arg>       args(num_threads);

    /* Expected answer: each thread adds 1+2+...+ops_per_thread */
    long expected = (long)ops_per_thread * (ops_per_thread + 1) / 2 * num_threads;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < num_threads; ++i) {
        args[i].ops = ops_per_thread;
        pthread_create(&threads[i], nullptr, worker, &args[i]);
    }
    for (int i = 0; i < num_threads; ++i)
        pthread_join(threads[i], nullptr);

    clock_gettime(CLOCK_MONOTONIC, &t1);

    double ms = (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) * 1e-6;
    printf("  Time     = %.3f ms\n", ms);
    printf("  Total Sum    = %ld\n", g_sum);
    printf("  Expected Sum = %ld\n", expected);

    // Assert to verify the final answer is correct.
    assert(g_sum == expected && "FAIL: sum is wrong!");
    
    printf("PASS\n");

    pthread_mutex_destroy(&g_lock);
    return 0;
}