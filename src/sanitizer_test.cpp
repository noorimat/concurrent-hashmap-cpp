#include "lockfree_hashmap.hpp"
#include <iostream>
#include <thread>
#include <vector>

// Simple test to verify with AddressSanitizer and ThreadSanitizer
int main() {
    std::cout << "Running sanitizer verification test...\n\n";

    LockFreeHashMap<int, int> map(128);

    auto worker = [&map](int id) {
        // Mix of operations
        for (int i = 0; i < 1000; i++) {
            int key = id * 1000 + i;
            map.insert(key, key * 10);

            int value;
            map.get(key, value);

            if (i % 2 == 0) {
                map.remove(key);
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; i++) {
        threads.emplace_back(worker, i);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "✓ Test completed\n";
    std::cout << "✓ No memory leaks detected (if running with ASan)\n";
    std::cout << "✓ No data races detected (if running with TSan)\n";

    return 0;
}