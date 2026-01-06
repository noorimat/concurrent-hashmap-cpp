#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>
#include "hazard_pointer.hpp"

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

    // Hazard pointer manager for safe memory reclamation
    HazardPointerManager<Node> hp_manager;

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

        while (true) {
            Node* head = buckets[index].load(std::memory_order_acquire);

            // Protect head with hazard pointer
            auto guard = hp_manager.make_guard(0, head);

            // Verify head hasn't changed
            if (head != buckets[index].load(std::memory_order_acquire)) {
                continue; // Head changed, retry
            }

            // Check if key already exists
            Node* current = head;
            while (current != nullptr) {
                if (current->key == key) {
                    // Key exists, update value
                    current->value = value;
                    delete new_node;
                    return false;
                }

                Node* next = current->next.load(std::memory_order_acquire);
                current = next;
            }

            // Point new node to current head
            new_node->next.store(head, std::memory_order_relaxed);

            // Try to atomically swap
            if (buckets[index].compare_exchange_weak(
                    head, new_node,
                    std::memory_order_release,
                    std::memory_order_acquire)) {
                return true;
            }
        }
    }

    bool get(const K& key, V& value) const {
        size_t index = get_bucket_index(key);

        while (true) {
            Node* current = buckets[index].load(std::memory_order_acquire);

            // Protect current node with hazard pointer
            const_cast<HazardPointerManager<Node>&>(hp_manager).acquire(0, current);

            // Verify current is still valid
            if (current != buckets[index].load(std::memory_order_acquire)) {
                continue; // List changed, retry
            }

            while (current != nullptr) {
                if (current->key == key) {
                    value = current->value;
                    const_cast<HazardPointerManager<Node>&>(hp_manager).release(0);
                    return true;
                }

                Node* next = current->next.load(std::memory_order_acquire);

                // Update hazard pointer to next node
                const_cast<HazardPointerManager<Node>&>(hp_manager).acquire(0, next);

                current = next;
            }

            const_cast<HazardPointerManager<Node>&>(hp_manager).release(0);
            return false;
        }
    }

    bool remove(const K& key) {
        size_t index = get_bucket_index(key);

        while (true) {
            Node* head = buckets[index].load(std::memory_order_acquire);

            // Protect head with hazard pointer
            auto guard = hp_manager.make_guard(0, head);

            // Verify head hasn't changed
            if (head != buckets[index].load(std::memory_order_acquire)) {
                continue;
            }

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
                            // Successfully removed head - retire node safely
                            hp_manager.retire(current);
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
                            // Successfully removed - retire node safely
                            hp_manager.retire(current);
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
        }
    }

    size_t size() const {
        return capacity;
    }
};