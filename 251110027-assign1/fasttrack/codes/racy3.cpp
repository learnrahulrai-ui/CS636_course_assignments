#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// ==========================================
// SHARED DATA
// ==========================================
typedef struct {
    int active_users;
    int total_requests;
    pthread_mutex_t lock;
} Config;

Config server_conf;

// ==========================================
// WORKER THREADS
// ==========================================

// THREAD 1: The "Updater" (Behaves Correctly)
// Always uses the lock to modify data.
void* run_updater(void* arg) {
    long id = (long)arg;
    
    // Simulate some work
    usleep(100); 

    // CORRECT USAGE: Lock -> Write -> Unlock
    pthread_mutex_lock(&server_conf.lock);
    
    server_conf.active_users++;     // WRITE A
    server_conf.total_requests += 5; // WRITE B
    
    printf("[Updater %ld] Updated config. Users: %d\n", id, server_conf.active_users);
    
    pthread_mutex_unlock(&server_conf.lock);
    
    return NULL;
}

// THREAD 2: The "Monitor" (Buggy Reader)
// BUG: Reads shared data WITHOUT holding the lock.
// "I'm just reading stats, I don't need a lock!" (False)
void* run_monitor(void* arg) {
    long id = (long)arg;

    // RACE CONDITION here: Reading while Updater is writing
    int current_users = server_conf.active_users; // READ A (Race with WRITE A)
    int current_reqs  = server_conf.total_requests; // READ B (Race with WRITE B)

    printf("[Monitor %ld] Stats: Users=%d, Reqs=%d\n", id, current_users, current_reqs);
    
    return NULL;
}

// THREAD 3: The "Resetter" (Buggy Writer)
// BUG: Resets active_users WITHOUT lock.
// "I'm just setting it to 0, it's atomic!" (False)
void* run_resetter(void* arg) {
    long id = (long)arg;

    usleep(50); // Timing adjustment to hit the race window

    // RACE CONDITION here: Writing while Updater is writing
    server_conf.active_users = 0; // WRITE C (Race with WRITE A and READ A)

    printf("[Resetter %ld] Reset users to 0.\n", id);

    return NULL;
}

// ==========================================
// MAIN
// ==========================================
int main() {
    server_conf.active_users = 0;
    server_conf.total_requests = 0;
    pthread_mutex_init(&server_conf.lock, NULL);

    pthread_t t1, t2, t3;

    // Spawn threads
    // T1 (Updater) vs T2 (Monitor)  -> Read-Write Race
    // T1 (Updater) vs T3 (Resetter) -> Write-Write Race
    pthread_create(&t1, NULL, run_updater, (void*)1);
    pthread_create(&t2, NULL, run_monitor, (void*)2);
    pthread_create(&t3, NULL, run_resetter, (void*)3);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    pthread_join(t3, NULL);

    pthread_mutex_destroy(&server_conf.lock);
    return 0;
}