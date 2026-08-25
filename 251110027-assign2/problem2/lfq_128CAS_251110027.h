#pragma once
#include <atomic>
#include <cstdint>

struct LFNode;

struct alignas(16) TaggedPointer {
    LFNode* ptr;
    uint64_t count;

    // Operator overloading
    bool operator==(const TaggedPointer& other) const {
        return ptr == other.ptr && count == other.count;
    }
    
    TaggedPointer() : ptr(nullptr), count(0) {}
    TaggedPointer(LFNode* p, uint64_t c) : ptr(p), count(c) {}
};

struct LFNode {
    int value;
    std::atomic<TaggedPointer> next;
};

class LFQueue {
    std::atomic<TaggedPointer> head;
    std::atomic<TaggedPointer> tail;

public:
    LFQueue() {
        LFNode* dummy = new LFNode();
        dummy->next.store(TaggedPointer(nullptr, 0));
        head.store(TaggedPointer(dummy, 0));
        tail.store(TaggedPointer(dummy, 0));
    }

    ~LFQueue() {
        while (deq() != -1);
        delete head.load().ptr;
    }

    void print() {
        TaggedPointer curr = head.load().ptr->next.load();
        std::cout << "Queue: ";
        while (curr.ptr != nullptr) {
            std::cout << curr.ptr->value << " -> ";
            curr = curr.ptr->next.load();
        }
        std::cout << "NULL\n";
    }

    int size() {
        int count = 0;
        TaggedPointer curr = head.load().ptr->next.load();
        while (curr.ptr != nullptr) {
            count++;
            curr = curr.ptr->next.load();
        }
        return count;
    }

    bool enq(int value) {
        LFNode* new_node = new LFNode();
        new_node->value = value;
        new_node->next.store(TaggedPointer(nullptr, 0));

        while (true) {
            TaggedPointer last = tail.load();
            TaggedPointer next = last.ptr->next.load();

            if (last == tail.load()) {
                if (next.ptr == nullptr) {
                    // Try to attach the new node. Increment the counter from next's count.
                    TaggedPointer new_next(new_node, next.count + 1);
                    
                    if (last.ptr->next.compare_exchange_weak(next, new_next)) {
                        // Node attached successfully. Try to update the tail pointer.
                        TaggedPointer new_tail(new_node, last.count + 1);
                        tail.compare_exchange_weak(last, new_tail);
                        return true;
                    }
                } else {
                    // Tail points to wrong node help it
                    TaggedPointer new_tail(next.ptr, last.count + 1);
                    tail.compare_exchange_weak(last, new_tail);
                }
            }
        }
    }

    int deq() {
        while (true) {
            TaggedPointer first = head.load();
            TaggedPointer last = tail.load();
            TaggedPointer next = first.ptr->next.load();

            if (first == head.load()) {
                if (first.ptr == last.ptr) {
                    // Queue is empty or tail is falling behind
                    if (next.ptr == nullptr) {
                        return -1;
                    }
                    // Tail points to wrong node, help it advance
                    TaggedPointer new_tail(next.ptr, last.count + 1);
                    tail.compare_exchange_weak(last, new_tail);
                } else {
                    int last_val = next.ptr->value;
                    
                    // Create new head pointer, incrementing the counter
                    TaggedPointer new_head(next.ptr, first.count + 1);
                    
                    if (head.compare_exchange_weak(first, new_head)) {
                        return last_val;
                    }
                }
            }
        }
    }
};