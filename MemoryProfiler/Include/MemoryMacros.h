#pragma once
#include <cstddef>
#include <new>

void force_link_memory_operators();

void* operator new(std::size_t size, const char* file, int line);
void* operator new[](std::size_t size, const char* file, int line);

#define new new(__FILE__, __LINE__)