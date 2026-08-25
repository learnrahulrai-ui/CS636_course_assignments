#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const uint64_t ITERS = 2;
uint64_t counter = 0;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;

// Thread entry function must match pthread signature
void* increment(void* arg) {
    (void)arg; // unused

    for (uint64_t i = 0; i < ITERS; ++i) {
        pthread_mutex_lock(&mtx);
        ++counter;
        pthread_mutex_unlock(&mtx);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t1, NULL, increment, NULL);
    pthread_create(&t2, NULL, increment, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    printf("Counter value: %lu\n", counter);

    return EXIT_SUCCESS;
}
