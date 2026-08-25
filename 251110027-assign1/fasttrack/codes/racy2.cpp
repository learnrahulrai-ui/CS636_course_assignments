#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> // for sleep/usleep

// Shared resource (Data Race Vulnerable)
int shared_balance = 1000;

// Thread 1: The "Depositor"
// RACE 1: Reads shared_balance, Writes shared_balance
void* depositor(void* arg) {
    (void)arg; 
    
    int amount = 500;
    int temp = shared_balance; // READ
    // Simulate processing time to widen the race window
    usleep(100); 
    shared_balance = temp + amount; // WRITE
    
    printf("Depositor: Added %d. New Balance: %d\n", amount, shared_balance);
    return NULL;
}

// Thread 2: The "Withdrawer"
// RACE 2: Reads shared_balance, Writes shared_balance
void* withdrawer(void* arg) {
    (void)arg;
    
    int amount = 200;
    int temp = shared_balance; // READ
    // Simulate processing time
    usleep(100);
    shared_balance = temp - amount; // WRITE
    
    printf("Withdrawer: Removed %d. New Balance: %d\n", amount, shared_balance);
    return NULL;
}

// Thread 3: The "Monitor"
// RACE 3: Reads shared_balance while others are writing
void* monitor(void* arg) {
    (void)arg;
    
    // Busy wait reading the balance
    // This creates Read-Write races with both Depositor and Withdrawer
    for (int i = 0; i < 5; i++) {
        int current = shared_balance; // READ
        printf("Monitor: Check %d -> Balance is %d\n", i, current);
        usleep(50);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2, t3;

    printf("Initial Balance: %d\n", shared_balance);

    // Parent (Main Thread 0) spawns all children
    pthread_create(&t1, NULL, depositor, NULL);
    pthread_create(&t2, NULL, withdrawer, NULL);
    pthread_create(&t3, NULL, monitor, NULL);

    // Wait for all threads to finish
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    printf("Final Balance: %d (Expected 1300 if synchronized, likely incorrect here)\n", shared_balance);

    return EXIT_SUCCESS;
}