#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

// ==========================================
// SHARED DATA
// ==========================================
typedef struct {
    int balance;
    int total_transactions;
    bool bank_is_open;
    
    // Only using Mutex (Compatible with your tool)
    pthread_mutex_t lock;
} Bank;

Bank my_bank;

// ==========================================
// WORKER THREADS
// ==========================================

// DEPOSITOR: Simply locks, adds money, unlocks.
void* run_depositor(void* arg) {
    long id = (long)arg;
    
    for (int i = 0; i < 5; i++) { // Do 5 deposits
        pthread_mutex_lock(&my_bank.lock);

        if (!my_bank.bank_is_open) {
            pthread_mutex_unlock(&my_bank.lock);
            break;
        }

        int amount = 100;
        my_bank.balance += amount;
        my_bank.total_transactions++;
        printf("[Depositor %ld] Deposited $%d. Balance: $%d\n", id, amount, my_bank.balance);

        pthread_mutex_unlock(&my_bank.lock);
        
        // Sleep to allow other threads to run
        usleep(100000); 
    }
    return NULL;
}

// WITHDRAWER: The "Complicated" Logic
// Instead of waiting on a condition variable, we explicitly:
// LOCK -> CHECK -> UNLOCK -> SLEEP(if failed) -> RETRY
void* run_withdrawer(void* arg) {
    long id = (long)arg;
    int amount_needed = 150;
    
    while (true) {
        // 1. Explicit Lock (Instrumented)
        pthread_mutex_lock(&my_bank.lock);

        // Check if we should stop
        if (!my_bank.bank_is_open) {
            pthread_mutex_unlock(&my_bank.lock);
            break;
        }

        // 2. Check Balance
        if (my_bank.balance >= amount_needed) {
            // SUCCESS PATH
            my_bank.balance -= amount_needed;
            my_bank.total_transactions++;
            printf("--- [Withdrawer %ld] SUCCESS! Withdrew $%d. Balance: $%d ---\n", 
                   id, amount_needed, my_bank.balance);
            
            pthread_mutex_unlock(&my_bank.lock);
            break; // Transaction done, exit thread
        } 
        else {
            // FAILURE PATH (Not enough money)
            // We must unlock to let a depositor in!
            printf("[Withdrawer %ld] Not enough funds ($%d). Unlocking and retrying...\n", 
                   id, my_bank.balance);
            
            // 3. Explicit Unlock (Instrumented)
            pthread_mutex_unlock(&my_bank.lock);
            
            // 4. Sleep outside the lock (simulate waiting)
            usleep(200000); 
        }
    }
    return NULL;
}

// AUDITOR: Reads state to verify consistency
void* run_auditor(void* arg) {
    long id = (long)arg;

    for (int i = 0; i < 3; i++) {
        pthread_mutex_lock(&my_bank.lock);
        
        printf("=== [Auditor %ld] Audit: Balance=$%d, Trans=%d ===\n", 
               id, my_bank.balance, my_bank.total_transactions);
        
        pthread_mutex_unlock(&my_bank.lock);
        usleep(300000);
    }
    return NULL;
}

// ==========================================
// MAIN (PARENT)
// ==========================================
int main() {
    // 1. Init
    my_bank.balance = 0;
    my_bank.total_transactions = 0;
    my_bank.bank_is_open = true;
    
    // Only using the mutex you instrument
    pthread_mutex_init(&my_bank.lock, NULL);

    pthread_t t1, t2, t3, t4, t5;

    printf("Bank Open. Initial Balance: 0\n");

    // 2. Create Threads (Captured by your __ft_thread_create)
    // Note: All spawned by TID 0 (Main)
    pthread_create(&t1, NULL, run_depositor, (void*)1);
    pthread_create(&t2, NULL, run_withdrawer, (void*)2); // Will fail initially
    pthread_create(&t3, NULL, run_depositor, (void*)3);
    pthread_create(&t4, NULL, run_auditor, (void*)4);
    pthread_create(&t5, NULL, run_withdrawer, (void*)5); // Will fail initially
    
    // 3. Join Threads (Captured by your __ft_thread_join)
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);
    pthread_join(t4, NULL);
    pthread_join(t5, NULL);

    // Cleanup
    pthread_mutex_lock(&my_bank.lock);
    my_bank.bank_is_open = false;
    pthread_mutex_unlock(&my_bank.lock);
    
    pthread_mutex_destroy(&my_bank.lock);

    printf("Bank Closed. Final Balance: %d\n", my_bank.balance);
    return 0;
}