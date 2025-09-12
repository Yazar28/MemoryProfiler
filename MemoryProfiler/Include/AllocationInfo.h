#pragma once
#include <cstddef>
#include <string>
#include <chrono>

struct AllocationInfo
{
	void* address = nullptr;
	size_t size = 0;
	std::string file = "unknown";
	int line = 0;
	std::string typeName = "unknown";
	std::chrono::time_point<std::chrono::high_resolution_clock> timestamp =
		std::chrono::high_resolution_clock::now();
};
