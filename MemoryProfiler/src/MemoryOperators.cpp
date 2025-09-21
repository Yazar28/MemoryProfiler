#include <cstddef>
#include <cstdlib>
#include <new>
#include "MemoryTracker.h"

static thread_local bool g_in_op_new = false;

// Declaración para forzar link de este TU desde el test
void force_link_memory_operators() {}

static inline void maybe_init_tracker() {
    // Si está construyéndose, NO inicializar ni tocar el tracker
    if (MemoryTracker::isInitializing()) return;

    // Si aún no está vivo y no está construyéndose, inicializarlo
    if (!MemoryTracker::isAlive()) {
        (void)MemoryTracker::getInstance();
    }
}

//-----------------------------
// new con file/line (opcional)
//-----------------------------
void* operator new(std::size_t size, const char* file, int line) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();

    if (!g_in_op_new) {
        g_in_op_new = true;

        if (!MemoryTracker::isInitializing()) {
            maybe_init_tracker();
            if (MemoryTracker::isAlive()) {
                MemoryTracker::getInstance().registerAllocation(ptr, size, file, line, "unknown");
            }
        }

        g_in_op_new = false;
    }
    return ptr;
}

//-----------------------------
// new/delete estándar
//-----------------------------
void* operator new(std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();

    if (!g_in_op_new) {
        g_in_op_new = true;

        if (!MemoryTracker::isInitializing()) {
            maybe_init_tracker();
            if (MemoryTracker::isAlive()) {
                MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0, "unknown");
            }
        }

        g_in_op_new = false;
    }
    return ptr;
}

void* operator new(std::size_t size, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(size);

    if (ptr && !g_in_op_new) {
        g_in_op_new = true;

        if (!MemoryTracker::isInitializing()) {
            maybe_init_tracker();
            if (MemoryTracker::isAlive()) {
                MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0, "unknown");
            }
        }

        g_in_op_new = false;
    }
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        if (!g_in_op_new && MemoryTracker::isAlive() && !MemoryTracker::isInitializing()) {
            g_in_op_new = true;
            MemoryTracker::getInstance().unregisterAllocation(ptr);
            g_in_op_new = false;
        }
        std::free(ptr);
    }
}

void operator delete(void* ptr, const std::nothrow_t&) noexcept {
    if (ptr) {
        if (!g_in_op_new && MemoryTracker::isAlive() && !MemoryTracker::isInitializing()) {
            g_in_op_new = true;
            MemoryTracker::getInstance().unregisterAllocation(ptr);
            g_in_op_new = false;
        }
        std::free(ptr);
    }
}

void* operator new[](std::size_t size) {
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();

    if (!g_in_op_new) {
        g_in_op_new = true;

        if (!MemoryTracker::isInitializing()) {
            maybe_init_tracker();
            if (MemoryTracker::isAlive()) {
                MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0, "unknown[]");
            }
        }

        g_in_op_new = false;
    }
    return ptr;
}

void* operator new[](std::size_t size, const std::nothrow_t&) noexcept {
    void* ptr = std::malloc(size);

    if (ptr && !g_in_op_new) {
        g_in_op_new = true;

        if (!MemoryTracker::isInitializing()) {
            maybe_init_tracker();
            if (MemoryTracker::isAlive()) {
                MemoryTracker::getInstance().registerAllocation(ptr, size, "unknown", 0, "unknown[]");
            }
        }

        g_in_op_new = false;
    }
    return ptr;
}

void operator delete[](void* ptr) noexcept {
    if (ptr) {
        if (!g_in_op_new && MemoryTracker::isAlive() && !MemoryTracker::isInitializing()) {
            g_in_op_new = true;
            MemoryTracker::getInstance().unregisterAllocation(ptr);
            g_in_op_new = false;
        }
        std::free(ptr);
    }
}
