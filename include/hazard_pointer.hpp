#pragma once

#include <atomic>
#include <vector>
#include <thread>
#include <algorithm>

// Hazard Pointer implementation for safe memory reclamation in lock-free data structures
template<typename T>
class HazardPointerManager {
private:
    static constexpr size_t MAX_THREADS = 128;
    static constexpr size_t MAX_HAZARDS_PER_THREAD = 2;
    static constexpr size_t RETIRED_THRESHOLD = 100;

    struct HazardPointer {
        std::atomic<T*> pointer;

        HazardPointer() : pointer(nullptr) {}

        // Prevent copying
        HazardPointer(const HazardPointer&) = delete;
        HazardPointer& operator=(const HazardPointer&) = delete;

        // Allow moving
        HazardPointer(HazardPointer&& other) noexcept
            : pointer(other.pointer.load(std::memory_order_relaxed)) {}
    };

    struct RetiredNode {
        T* ptr;
        std::thread::id thread_id;
    };

    // Global hazard pointer array (one per thread)
    std::vector<std::vector<HazardPointer>> hazard_pointers;

    // Retired list for each thread
    std::vector<std::vector<RetiredNode>> retired_lists;

    // Thread-local index for accessing hazard pointers
    thread_local static size_t thread_index;
    static std::atomic<size_t> thread_counter;

    size_t get_thread_index() {
        if (thread_index == SIZE_MAX) {
            thread_index = thread_counter.fetch_add(1, std::memory_order_relaxed);
        }
        return thread_index;
    }

    // Scan all hazard pointers to build protected set
    std::vector<T*> get_protected_pointers() {
        std::vector<T*> protected_ptrs;

        for (auto& thread_hazards : hazard_pointers) {
            for (auto& hp : thread_hazards) {
                T* ptr = hp.pointer.load(std::memory_order_acquire);
                if (ptr != nullptr) {
                    protected_ptrs.push_back(ptr);
                }
            }
        }

        std::sort(protected_ptrs.begin(), protected_ptrs.end());
        protected_ptrs.erase(std::unique(protected_ptrs.begin(), protected_ptrs.end()),
                            protected_ptrs.end());

        return protected_ptrs;
    }

    bool is_protected(T* ptr, const std::vector<T*>& protected_ptrs) {
        return std::binary_search(protected_ptrs.begin(), protected_ptrs.end(), ptr);
    }

public:
    HazardPointerManager() : retired_lists(MAX_THREADS) {
        // Manually construct hazard pointers to avoid copy constructor issues
        hazard_pointers.reserve(MAX_THREADS);
        for (size_t i = 0; i < MAX_THREADS; i++) {
            hazard_pointers.emplace_back();
            hazard_pointers[i].reserve(MAX_HAZARDS_PER_THREAD);
            for (size_t j = 0; j < MAX_HAZARDS_PER_THREAD; j++) {
                hazard_pointers[i].emplace_back();
            }
        }
    }

    ~HazardPointerManager() {
        // Clean up any remaining retired nodes
        for (auto& retired_list : retired_lists) {
            for (auto& node : retired_list) {
                delete node.ptr;
            }
        }
    }

    // Prevent copying the manager itself
    HazardPointerManager(const HazardPointerManager&) = delete;
    HazardPointerManager& operator=(const HazardPointerManager&) = delete;

    // Acquire a hazard pointer slot
    void acquire(size_t slot, T* ptr) {
        size_t idx = get_thread_index();
        hazard_pointers[idx][slot].pointer.store(ptr, std::memory_order_release);
    }

    // Release a hazard pointer slot
    void release(size_t slot) {
        size_t idx = get_thread_index();
        hazard_pointers[idx][slot].pointer.store(nullptr, std::memory_order_release);
    }

    // Retire a pointer for later deletion
    void retire(T* ptr) {
        size_t idx = get_thread_index();
        retired_lists[idx].push_back({ptr, std::this_thread::get_id()});

        // Try to reclaim memory if retired list is getting large
        if (retired_lists[idx].size() >= RETIRED_THRESHOLD) {
            reclaim();
        }
    }

    // Attempt to reclaim retired memory
    void reclaim() {
        size_t idx = get_thread_index();
        auto& retired_list = retired_lists[idx];

        if (retired_list.empty()) {
            return;
        }

        // Get all currently protected pointers
        std::vector<T*> protected_ptrs = get_protected_pointers();

        // Separate safe-to-delete from still-protected
        std::vector<RetiredNode> still_retired;

        for (auto& node : retired_list) {
            if (!is_protected(node.ptr, protected_ptrs)) {
                // Safe to delete
                delete node.ptr;
            } else {
                // Still protected, keep in retired list
                still_retired.push_back(node);
            }
        }

        retired_list = std::move(still_retired);
    }

    // RAII helper for automatic acquire/release
    class Guard {
    private:
        HazardPointerManager* manager;
        size_t slot;

    public:
        Guard(HazardPointerManager* mgr, size_t s, T* ptr)
            : manager(mgr), slot(s) {
            manager->acquire(slot, ptr);
        }

        ~Guard() {
            manager->release(slot);
        }

        // Prevent copying
        Guard(const Guard&) = delete;
        Guard& operator=(const Guard&) = delete;

        void update(T* ptr) {
            manager->acquire(slot, ptr);
        }
    };

    Guard make_guard(size_t slot, T* ptr) {
        return Guard(this, slot, ptr);
    }
};

// Static member initialization
template<typename T>
thread_local size_t HazardPointerManager<T>::thread_index = SIZE_MAX;

template<typename T>
std::atomic<size_t> HazardPointerManager<T>::thread_counter{0};