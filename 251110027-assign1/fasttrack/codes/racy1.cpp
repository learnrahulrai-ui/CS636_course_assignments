#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

volatile uint64_t x = 0;

// Reader thread
void* reader(void* arg) {
    (void)arg; // unused

    while (x == 0) {
        // busy wait
    }
    printf("World\n");
    return NULL;
}

// Writer thread
void* writer(void* arg) {
    (void)arg; // unused

    printf("Hello, ");
    x = 1;
    return NULL;
}

int main() {
    pthread_t t1, t2;

    pthread_create(&t2, NULL, writer, NULL);
    pthread_create(&t1, NULL, reader, NULL);

    pthread_join(t2, NULL);
    pthread_join(t1, NULL);

    return EXIT_SUCCESS;
}
