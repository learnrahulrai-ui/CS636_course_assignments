/*
 * Array-Based Queue Lock (ABQL)
 * ----------------------------------------
 */

#pragma once
#include <atomic>
#include <vector>
#include <cstddef>
#include <thread>

class ABQLock {
public:
    /* ── Constructor ─────────────────────────────────────────────────────── */

    explicit ABQLock(int max_threads)
        : capacity_(max_threads),
          tail_(0),
          slots_(max_threads)
    {
        /* Only slot 0 is open at start; all others are closed. */
        slots_[0].val.store(true,  std::memory_order_relaxed);
        for (int i = 1; i < max_threads; ++i)
            slots_[i].val.store(false, std::memory_order_relaxed);
    }

    /* ── lock() ──────────────────────────────────────────────────────────── */
    size_t lock()
    {
        size_t slot = tail_.fetch_add(1, std::memory_order_relaxed)
                      % (size_t)capacity_;

        while (!slots_[slot].val.load(std::memory_order_acquire)){
            std::this_thread::yield();
        }
        
        return slot;
    }

    /* ── unlock() ────────────────────────────────────────────────────────── */
    void unlock(size_t slot)
    {
        size_t next = (slot + 1) % (size_t)capacity_;

        slots_[slot].val.store(false, std::memory_order_relaxed);
        slots_[next].val.store(true,  std::memory_order_release);
    }

private:
    /* In order to avoid False Sharing */
    struct alignas(64) PaddedSlot {
        std::atomic<bool> val;
    };

    int                      capacity_;
    std::atomic<size_t>      tail_;
    std::vector<PaddedSlot>  slots_;
};