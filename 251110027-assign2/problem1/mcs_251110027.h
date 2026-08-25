/*
 * MCS Queue Lock
 * -----------------------
 */

#pragma once
#include <atomic>
#include <cstddef>
#include <thread>

/* ── MCSNode ─────────────────────────────────────────────────────────────── */
struct alignas(64) MCSNode {
    std::atomic<bool>      locked{false};   /* spin variable              */
    std::atomic<MCSNode*>  next{nullptr};   /* successor in the queue     */
};


/* ── MCSLock ─────────────────────────────────────────────────────────────── */
class MCSLock {
public:
    MCSLock() : tail_(nullptr) {}

    /* ── lock() ─────────────────────────────────────────────────────────── */
    void lock(MCSNode* node)
    {
        /* Reset the node so it is ready for a fresh acquire. */
        node->locked.store(true,    std::memory_order_relaxed);
        node->next  .store(nullptr, std::memory_order_relaxed);

        MCSNode* prev = tail_.exchange(node, std::memory_order_acq_rel);

        if (prev != nullptr) {
            prev->next.store(node, std::memory_order_release);
            while (node->locked.load(std::memory_order_acquire)){
                std::this_thread::yield();
            }
        }
    }

    /* ── unlock() ───────────────────────────────────────────────────────── */
    void unlock(MCSNode* node)
    {
        MCSNode* succ = node->next.load(std::memory_order_acquire);

        if (succ == nullptr) {
            MCSNode* expected = node;
            if (tail_.compare_exchange_strong(expected, nullptr,
                                              std::memory_order_release,
                                              std::memory_order_relaxed)) {
                return;
            }

            while ((succ = node->next.load(std::memory_order_acquire)) == nullptr){
                std::this_thread::yield();
            }
        }

        succ->locked.store(false, std::memory_order_release);
    }

private:
    std::atomic<MCSNode*> tail_;
};