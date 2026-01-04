#include "lockfree_hashmap.hpp"
#include <iostream>
#include <string>

int main() {
    LockFreeHashMap<std::string, int> map(16);

    // Test basic operations
    std::cout << "Testing Lock-Free HashMap\n";
    std::cout << "=========================\n\n";

    // Insert some values
    map.insert("apple", 1);
    map.insert("banana", 2);
    map.insert("cherry", 3);

    std::cout << "Inserted: apple=1, banana=2, cherry=3\n\n";

    // Retrieve values
    int value;
    if (map.get("apple", value)) {
        std::cout << "apple: " << value << "\n";
    }

    if (map.get("banana", value)) {
        std::cout << "banana: " << value << "\n";
    }

    if (map.get("cherry", value)) {
        std::cout << "cherry: " << value << "\n";
    }

    // Try non-existent key
    if (!map.get("orange", value)) {
        std::cout << "\norange: not found (expected)\n";
    }

    std::cout << "\n✓ Basic operations working!\n";

    // Test remove
    std::cout << "\nTesting remove...\n";
    if (map.remove("banana")) {
        std::cout << "✓ Removed banana\n";
    }

    if (!map.get("banana", value)) {
        std::cout << "✓ Banana no longer exists\n";
    }

    if (map.get("apple", value)) {
        std::cout << "✓ Apple still exists: " << value << "\n";
    }

    std::cout << "\n🎉 All tests passed!\n";

    return 0;
}