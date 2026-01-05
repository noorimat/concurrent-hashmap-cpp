#include "lockfree_hashmap.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <mutex>
#include <iomanip>
#include <random>

// Baseline: std::unordered_map with mutex protection
template<typename K, typename V>
class LockedHashMap {
private:
    std::unordered_map<K, V> map;
    mutable std::mutex mtx;

public:
    bool insert(const K& key, const V& value) {
        std::lock_guard<std::mutex> lock(mtx);
        map[key] = value;
        return true;
    }

    bool get(const K& key, V& value) const {
        std::lock_guard<std::mutex> lock(mtx);
        auto it = map.find(key);
        if (it != map.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    bool remove(const K& key) {
        std::lock_guard<std::mutex> lock(mtx);
        return map.erase(key) > 0;
    }
};

// Benchmark workload types
enum class WorkloadType {
    INSERT_ONLY,
    READ_ONLY,
    MIXED_50_50,
    READ_HEAVY_80_20
};

template<typename MapType>
void run_workload(MapType* map, int thread_id, int ops_per_thread, WorkloadType workload) {
    std::mt19937 rng(thread_id);
    std::uniform_int_distribution<int> dist(0, ops_per_thread * 8);
    std::uniform_int_distribution<int> percent(0, 99);

    for (int i = 0; i < ops_per_thread; i++) {
        int key = dist(rng);

        switch (workload) {
            case WorkloadType::INSERT_ONLY:
                map->insert(key, key * 10);
                break;

            case WorkloadType::READ_ONLY: {
                int value;
                map->get(key, value);
                break;
            }

            case WorkloadType::MIXED_50_50: {
                if (percent(rng) < 50) {
                    map->insert(key, key * 10);
                } else {
                    int value;
                    map->get(key, value);
                }
                break;
            }

            case WorkloadType::READ_HEAVY_80_20: {
                if (percent(rng) < 80) {
                    int value;
                    map->get(key, value);
                } else {
                    map->insert(key, key * 10);
                }
                break;
            }
        }
    }
}

template<typename MapType>
double benchmark(MapType* map, int num_threads, int ops_per_thread, WorkloadType workload) {
    std::vector<std::thread> threads;

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(run_workload<MapType>, map, i, ops_per_thread, workload);
    }

    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);

    return duration.count() / 1000.0; // Return milliseconds
}

std::string workload_name(WorkloadType type) {
    switch (type) {
        case WorkloadType::INSERT_ONLY: return "Insert-Only";
        case WorkloadType::READ_ONLY: return "Read-Only";
        case WorkloadType::MIXED_50_50: return "Mixed 50/50";
        case WorkloadType::READ_HEAVY_80_20: return "Read-Heavy 80/20";
    }
    return "Unknown";
}

void print_header() {
    std::cout << "\n┌─────────────────────────────────────────────────────────────────────────┐\n";
    std::cout << "│         Lock-Free HashMap vs Mutex-Based HashMap Benchmark             │\n";
    std::cout << "└─────────────────────────────────────────────────────────────────────────┘\n\n";
}

void run_benchmark_suite(int num_threads, int ops_per_thread, WorkloadType workload) {
    std::cout << "Workload: " << workload_name(workload) << "\n";
    std::cout << "Threads: " << num_threads << " | Operations/thread: " << ops_per_thread << "\n";
    std::cout << std::string(75, '-') << "\n";

    // Benchmark Lock-Free HashMap
    LockFreeHashMap<int, int> lockfree_map(1024);
    double lockfree_time = benchmark(&lockfree_map, num_threads, ops_per_thread, workload);

    // Benchmark Mutex-Based HashMap
    LockedHashMap<int, int> locked_map;
    double locked_time = benchmark(&locked_map, num_threads, ops_per_thread, workload);

    // Calculate speedup
    double speedup = locked_time / lockfree_time;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Lock-Free HashMap:  " << std::setw(8) << lockfree_time << " ms\n";
    std::cout << "Mutex-Based HashMap: " << std::setw(8) << locked_time << " ms\n";
    std::cout << "Speedup:            " << std::setw(8) << speedup << "x ";

    if (speedup > 1.0) {
        std::cout << "✓ Lock-free is FASTER\n";
    } else {
        std::cout << "✗ Mutex-based is faster\n";
    }
    std::cout << "\n";
}

int main() {
    print_header();

    const int OPS_PER_THREAD = 50000;

    // Test different thread counts
    std::vector<int> thread_counts = {1, 2, 4, 8};
    std::vector<WorkloadType> workloads = {
        WorkloadType::INSERT_ONLY,
        WorkloadType::READ_ONLY,
        WorkloadType::MIXED_50_50,
        WorkloadType::READ_HEAVY_80_20
    };

    for (auto workload : workloads) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        for (int threads : thread_counts) {
            run_benchmark_suite(threads, OPS_PER_THREAD, workload);
        }
    }

    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "\n✓ Benchmark complete!\n\n";

    return 0;
}