#pragma once
#include <atomic>
#include <cstdint>

struct LFNode{
    int value;
    std::atomic<uintptr_t> next;
};

class LFQueue {
    std::atomic<uintptr_t> head;
    std::atomic<uintptr_t> tail;

    // Bottom 48 bits for pointer
    // Top 16 bits for counter
    static const uintptr_t PTR_MASK = 0x0000FFFFFFFFFFFFull;

    // Extracts the real pointer from the 64-bit tagged integer
    static LFNode* get_ptr(uintptr_t tagged) {
        return reinterpret_cast<LFNode*>(tagged & PTR_MASK);
    }

    // Extracts the 16-bit counter from the top of the 64-bit tagged integer
    static uint16_t get_count(uintptr_t tagged) {
        return static_cast<uint16_t>(tagged >> 48);
    }

    // Packs a pointer and a counter together into a single 64-bit integer
    static uintptr_t pack(LFNode* ptr, uint16_t count) {
        uintptr_t raw_ptr = reinterpret_cast<uintptr_t>(ptr) & PTR_MASK;
        uintptr_t shifted_count = static_cast<uintptr_t>(count) << 48;
        return raw_ptr | shifted_count;
    }

public:
    LFQueue() {
        LFNode* dummy = new LFNode();
        dummy->next.store(pack(nullptr, 0));
        head.store(pack(dummy, 0));
        tail.store(pack(dummy, 0));
    }

    ~LFQueue() {
        while (deq() != -1);
        delete get_ptr(head.load());
    }

    void print() {
        uintptr_t curr_tagged = get_ptr(head.load())->next.load();
        LFNode* curr = get_ptr(curr_tagged);
        std::cout << "Queue: ";
        while (curr != nullptr) {
            std::cout << curr->value << " -> ";
            curr = get_ptr(curr->next.load());
        }
        std::cout << "NULL\n";
    }

    int size() {
        int count = 0;
        uintptr_t curr_tagged = get_ptr(head.load())->next.load();
        LFNode* curr = get_ptr(curr_tagged);
        while (curr != nullptr) {
            count++;
            curr = get_ptr(curr->next.load());
        }
        return count;
    }

    bool enq(int value) {
        LFNode* new_node = new LFNode();
        new_node->value = value;
        new_node->next.store(pack(nullptr, 0));

        while(true){
            uintptr_t last_tagged = tail.load();
            LFNode* last = get_ptr(last_tagged);

            uintptr_t next_tagged = last->next.load();
            LFNode* next = get_ptr(next_tagged);
    
            if(last_tagged == tail.load()){
                if(next == nullptr){
                    // The state till now is consistant
                    // try to replace
                    uintptr_t new_next_tagged = pack(new_node, get_count(next_tagged)+1);
                    if(last->next.compare_exchange_weak(next_tagged,new_next_tagged)){
                        // new node inserted in the queue now try to update the tail
                        tail.compare_exchange_weak(last_tagged,new_next_tagged);
                        return true;
                    }
                }else{
                    // failed but help the other thread by updating the tail
                    uintptr_t new_tail_tagged = pack(next, get_count(last_tagged)+1);
                    tail.compare_exchange_weak(last_tagged,new_tail_tagged);
                }
            }
        }
    }

    int deq() {
        while(true){

            uintptr_t first_tagged = head.load();
            LFNode* first = get_ptr(first_tagged);
            uintptr_t last_tagged = tail.load();
            LFNode* last = get_ptr(last_tagged);
            uintptr_t next_tagged = first->next.load();
            LFNode* next = get_ptr(next_tagged);
            
            if(first_tagged == head.load()){
                // The queue is empty
                if(first == last){
                    // The queue is endeed empty
                    if(next == nullptr)
                    return -1;
                    // The last pointer is pointing to wrong node help
                    uintptr_t new_tail_tagged = pack(next,get_count(last_tagged)+1);
                    tail.compare_exchange_weak(last_tagged, new_tail_tagged);
                }else{
                    // Return the last value
                    int last_val = next->value;
                    uintptr_t new_head_tagged = pack(next,get_count(first_tagged)+1);
                    if(head.compare_exchange_weak(first_tagged, new_head_tagged)){
                        return last_val;
                    }
                }
            }
        }
    }
};
