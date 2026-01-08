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
        std::atomic<bool> deleted; // Logical deletion flag

        Node(const K& k, const V& v) : key(k), value(v), next(nullptr), deleted(false) {}
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

    // Insert - allows duplicate keys
    bool insert(const K& key, const V& value) {
        size_t index = get_bucket_index(key);
        Node* new_node = new Node(key, value);

        while (true) {
            Node* head = buckets[index].load(std::memory_order_acquire);
            new_node->next.store(head, std::memory_order_relaxed);

            if (buckets[index].compare_exchange_weak(
                    head, new_node,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return true;
            }
        }
    }

    // Get - skips logically deleted nodes
    bool get(const K& key, V& value) const {
        size_t index = get_bucket_index(key);
        Node* current = buckets[index].load(std::memory_order_acquire);

        while (current != nullptr) {
            if (!current->deleted.load(std::memory_order_acquire) && current->key == key) {
                value = current->value;
                return true;
            }
            current = current->next.load(std::memory_order_acquire);
        }
        return false;
    }

    // Remove - uses logical deletion (marks node as deleted without freeing memory)
    // Physical deletion happens in destructor
    // NOTE: This causes memory to accumulate until the map is destroyed
    bool remove(const K& key) {
        size_t index = get_bucket_index(key);
        Node* current = buckets[index].load(std::memory_order_acquire);

        while (current != nullptr) {
            if (!current->deleted.load(std::memory_order_acquire) && current->key == key) {
                // Mark as logically deleted
                bool expected = false;
                if (current->deleted.compare_exchange_strong(
                        expected, true,
                        std::memory_order_release,
                        std::memory_order_relaxed)) {
                    return true;
                }
            }
            current = current->next.load(std::memory_order_acquire);
        }
        return false;
    }

    size_t size() const {
        return capacity;
    }
};