#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

template<typename K, typename V>
class LockFreeHashMap {
private:
    struct Node {
        K key;
        V value;
        std::atomic<Node*> next;

        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    std::vector<std::atomic<Node*>> buckets;
    size_t capacity;
    std::hash<K> hasher;

    size_t get_bucket_index(const K& key) const {
        return hasher(key) % capacity;
    }

public:
    explicit LockFreeHashMap(size_t initial_capacity = 16)
        : buckets(initial_capacity), capacity(initial_capacity) {
        for (auto& bucket : buckets) {
            bucket.store(nullptr, std::memory_order_relaxed);
        }
    }

    ~LockFreeHashMap() {
        // Clean up all nodes
        for (auto& bucket : buckets) {
            Node* current = bucket.load(std::memory_order_relaxed);
            while (current != nullptr) {
                Node* next = current->next.load(std::memory_order_relaxed);
                delete current;
                current = next;
            }
        }
    }

    bool insert(const K& key, const V& value) {
        size_t index = get_bucket_index(key);
        Node* new_node = new Node(key, value);

        // Lock-free insert using Compare-And-Swap (CAS)
        while (true) {
            // Load current head
            Node* head = buckets[index].load(std::memory_order_acquire);

            // Check if key already exists
            Node* current = head;
            while (current != nullptr) {
                if (current->key == key) {
                    // Key exists, update value
                    current->value = value;
                    delete new_node; // Don't leak memory
                    return false; // Indicate key already existed
                }
                current = current->next.load(std::memory_order_acquire);
            }

            // Point new node to current head
            new_node->next.store(head, std::memory_order_relaxed);

            // Try to atomically swap new node as the new head
            // If head hasn't changed, this succeeds
            // If another thread changed head, we retry
            if (buckets[index].compare_exchange_weak(
                    head, new_node,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return true; // Successfully inserted
            }
            // CAS failed, another thread modified the list
            // Loop will retry with updated head
        }
    }

    bool get(const K& key, V& value) const {
        size_t index = get_bucket_index(key);
        Node* current = buckets[index].load(std::memory_order_acquire);

        while (current != nullptr) {
            if (current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next.load(std::memory_order_acquire);
        }
        return false;
    }

    bool remove(const K& key) {
        size_t index = get_bucket_index(key);

        while (true) {
            Node* head = buckets[index].load(std::memory_order_acquire);
            Node* current = head;
            Node* prev = nullptr;

            // Search for the key
            while (current != nullptr) {
                if (current->key == key) {
                    // Found the key to remove
                    if (prev == nullptr) {
                        // Removing head node
                        Node* next = current->next.load(std::memory_order_acquire);
                        if (buckets[index].compare_exchange_weak(
                                head, next,
                                std::memory_order_release,
                                std::memory_order_acquire)) {
                            // Successfully removed head
                            // WARNING: We're leaking memory here for now
                            // We'll fix this with hazard pointers later
                            // delete current; // UNSAFE in lock-free context!
                            return true;
                        }
                        // CAS failed, retry
                        break;
                    } else {
                        // Removing middle/end node
                        Node* next = current->next.load(std::memory_order_acquire);
                        if (prev->next.compare_exchange_weak(
                                current, next,
                                std::memory_order_release,
                                std::memory_order_acquire)) {
                            // Successfully removed
                            // WARNING: Memory leak here too
                            // delete current; // UNSAFE!
                            return true;
                        }
                        // CAS failed, retry
                        break;
                    }
                }
                prev = current;
                current = current->next.load(std::memory_order_acquire);
            }

            // Key not found
            if (current == nullptr) {
                return false;
            }
            // If we got here, CAS failed, retry the whole operation
        }
    }

    size_t size() const {
        return capacity;
    }
};