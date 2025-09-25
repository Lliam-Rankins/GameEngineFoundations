#pragma once
#include <atomic>
#include <mutex>
#include <string>

// Shared state for multithreaded system
struct SharedData {
    std::atomic<long long> counter{0};
    std::atomic<size_t> nextJobIndex{0};

    // Shared network message
    std::string networkMessage;
    std::mutex networkMutex;
};