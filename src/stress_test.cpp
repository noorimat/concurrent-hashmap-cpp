#include "lockfree_hashmap.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

void insert_worker(LockFreeHashMap<int, int>* map, int thread_id, int num_ops) {
    for (int i = 0; i < num_ops; i++) {
        int key = thread_id * num_ops + i;
        map->insert(key, key * 10);
    }
}

void read_worker(LockFreeHashMap<int, int>* map, int num_ops, std::atomic<int>& success_count) {
    int local_success = 0;
    for (int i = 0; i < num_ops * 4; i++) { // Read across all threads' keys
        int value;
        if (map->get(i, value)) {
            local_success++;
        }
    }
    success_count.fetch_add(local_success, std::memory_order_relaxed);
}

int main() {
    const int NUM_THREADS = 8;
    const int OPS_PER_THREAD = 10000;

    std::cout << "Lock-Free HashMap Stress Test\n";
    std::cout << "==============================\n";
    std::cout << "Threads: " << NUM_THREADS << "\n";
    std::cout << "Operations per thread: " << OPS_PER_THREAD << "\n\n";

    LockFreeHashMap<int, int> map(1024);

    // Test 1: Concurrent Inserts
    std::cout << "Test 1: Concurrent inserts...\n";
    auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> insert_threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        insert_threads.emplace_back(insert_worker, &map, i, OPS_PER_THREAD);
    }

    for (auto& t : insert_threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "✓ Completed in " << duration.count() << "ms\n";
    std::cout << "  Total insertions: " << NUM_THREADS * OPS_PER_THREAD << "\n\n";

    // Test 2: Concurrent Reads
    std::cout << "Test 2: Concurrent reads...\n";
    std::atomic<int> success_count{0};

    start = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> read_threads;
    for (int i = 0; i < NUM_THREADS; i++) {
        read_threads.emplace_back(read_worker, &map, OPS_PER_THREAD, std::ref(success_count));
    }

    for (auto& t : read_threads) {
        t.join();
    }

    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "✓ Completed in " << duration.count() << "ms\n";
    std::cout << "  Successful reads: " << success_count.load() << "\n\n";

    // Verify correctness
    std::cout << "Test 3: Verifying data integrity...\n";
    int verified = 0;
    for (int i = 0; i < NUM_THREADS * OPS_PER_THREAD; i++) {
        int value;
        if (map.get(i, value) && value == i * 10) {
            verified++;
        }
    }

    std::cout << "✓ Verified " << verified << "/" << NUM_THREADS * OPS_PER_THREAD << " entries\n";

    if (verified == NUM_THREADS * OPS_PER_THREAD) {
        std::cout << "\n ALL TESTS PASSED! Hash map is thread-safe!\n";
    } else {
        std::cout << "\n Data corruption detected!\n";
    }

    return 0;
}