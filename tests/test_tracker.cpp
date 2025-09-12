#include "MemoryMacros.h"

int main() {
    force_link_memory_operators();

    int* x = new int;
    delete x;

    [[maybe_unused]] int* leak = new int[10]; 

    return 0;
}
