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

    // Basic insert - we'll make this lock-free next
    bool insert(const K& key, const V& value) {
        size_t index = get_bucket_index(key);
        Node* new_node = new Node(key, value);

        // For now, simple (non-thread-safe) version
        Node* head = buckets[index].load(std::memory_order_acquire);
        new_node->next.store(head, std::memory_order_relaxed);
        buckets[index].store(new_node, std::memory_order_release);

        return true;
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
        // We'll implement this properly later
        return false;
    }

    size_t size() const {
        return capacity;
    }
};