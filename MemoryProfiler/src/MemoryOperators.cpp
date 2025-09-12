#include <cstddef>
#include <cstdlib>
#include <new>
#include "MemoryTracker.h"

static thread_local bool g_in_op_new = false;

void force_link_memory_operators() {}

static inline void maybe_init_tracker() {
    if (!MemoryTracker::isAlive()) {
        (void)MemoryTracker::getInstance();
    }
}

void* operator new(std::size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    if (!g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, file, line);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void* operator new[](std::size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    if (!g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, file, line);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    if (!g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    if (!g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(size);
    if (ptr && !g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(size);
    if (ptr && !g_in_op_new) {
        g_in_op_new = true;
        maybe_init_tracker();
        if (MemoryTracker::isAlive()) {
            MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0);
        }
        g_in_op_new = false;
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        if (!g_in_op_new && MemoryTracker::isAlive()) {
            g_in_op_new = true;
            MemoryTracker::getInstance().unregisterAllocation(ptr);
            g_in_op_new = false;
        }
        std::free(ptr);
    }
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        if (!g_in_op_new && MemoryTracker::isAlive()) {
            g_in_op_new = true;
            MemoryTracker::getInstance().unregisterAllocation(ptr);
            g_in_op_new = false;
        }
        std::free(ptr);
    }
}