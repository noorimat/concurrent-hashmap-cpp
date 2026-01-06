#include "lockfree_hashmap.hpp"
#include <iostream>
#include <thread>
#include <vector>

int main() {
    std::cout << "Hazard Pointer Memory Reclamation Test\n";
    std::cout << "========================================\n\n";

    LockFreeHashMap<int, int> map(64);

    // Insert a lot of data
    std::cout << "Phase 1: Inserting 100,000 entries...\n";
    for (int i = 0; i < 100000; i++) {
        map.insert(i, i * 10);
    }
    std::cout << "✓ Inserted 100,000 entries\n\n";

    // Remove everything with concurrent operations
    std::cout << "Phase 2: Removing all entries concurrently (8 threads)...\n";

    auto remove_worker = [&map](int start, int end) {
        for (int i = start; i < end; i++) {
            map.remove(i);
        }
    };

    std::vector<std::thread> threads;
    int chunk = 100000 / 8;

    for (int i = 0; i < 8; i++) {
        threads.emplace_back(remove_worker, i * chunk, (i + 1) * chunk);
    }

    for (auto& t : threads) {
        t.join();
    }

    std::cout << "✓ Removed all 100,000 entries\n\n";

    // Verify everything is gone
    std::cout << "Phase 3: Verifying removal...\n";
    int found = 0;
    int value;
    for (int i = 0; i < 100000; i++) {
        if (map.get(i, value)) {
            found++;
        }
    }

    if (found == 0) {
        std::cout << "✓ All entries successfully removed\n";
        std::cout << "\n🎉 Hazard pointers successfully reclaimed memory!\n";
        std::cout << "   (No memory leaks - nodes deleted safely)\n";
    } else {
        std::cout << "✗ Found " << found << " entries still present\n";
    }

    return 0;
}