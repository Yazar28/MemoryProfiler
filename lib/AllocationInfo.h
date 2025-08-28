#pragma once
struct AllocationInfo {
    void* address;
    size_t size;
    const char* file;
    int line;
};
