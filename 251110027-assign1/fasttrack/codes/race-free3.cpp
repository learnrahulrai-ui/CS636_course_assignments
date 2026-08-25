#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 4
#define ARRAY_SIZE 1000000

// ==========================================
// SHARED DATA
// ==========================================
int* big_array;
long partial_sums[NUM_THREADS]; // Each thread writes to its own slot

// ==========================================
// WORKER LOGIC
// ==========================================
void* worker_sum(void* arg) {
    long tid = (long)arg;

    // 1. Determine the chunk size and boundaries
    int chunk_size = ARRAY_SIZE / NUM_THREADS;
    int start_index = tid * chunk_size;
    int end_index = start_index + chunk_size;

    // Handle remainder for the last thread
    if (tid == NUM_THREADS - 1) {
        end_index = ARRAY_SIZE;
    }

    long local_sum = 0;

    // 2. Perform computation (Read-Only on shared array)
    // This is SAFE because no one is writing to big_array concurrently.
    for (int i = start_index; i < end_index; i++) {
        local_sum += big_array[i];
    }

    // 3. Write result to shared memory
    // This is SAFE because 'tid' guarantees a unique address for each thread.
    partial_sums[tid] = local_sum;

    printf("[Worker %ld] Summed indices %d to %d. Result: %ld\n", 
           tid, start_index, end_index - 1, local_sum);

    return NULL;
}

// ==========================================
// MAIN THREAD
// ==========================================
int main() {
    // 1. Initialize Data
    big_array = (int*)malloc(ARRAY_SIZE * sizeof(int));
    
    // Fill array with 1s (Expected Total = 1,000,000)
    for (int i = 0; i < ARRAY_SIZE; i++) {
        big_array[i] = 1;
    }

    pthread_t threads[NUM_THREADS];

    printf("Starting parallel sum with %d threads...\n", NUM_THREADS);

    // 2. Spawn Threads
    // PARENT (Main) -> FORK -> CHILD (Worker)
    for (long i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_sum, (void*)i);
    }

    // 3. Join Threads
    // PARENT waits for CHILD. 
    // This creates a Happens-Before edge: All writes by Worker happen BEFORE Main continues.
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // 4. Aggregate Results
    // Main thread can safely read partial_sums because of the Join.
    long total_sum = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        total_sum += partial_sums[i];
    }

    printf("Final Total Sum: %ld (Expected: %d)\n", total_sum, ARRAY_SIZE);

    free(big_array);
    return 0;
}