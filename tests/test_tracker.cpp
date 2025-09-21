#include <cstdio>
#include "MemoryTracker.h"

void force_link_memory_operators();

static void flush(const char* s) {
    std::printf("%s\n", s);
    std::fflush(stdout);
}

int main() {
    force_link_memory_operators();

    flush("[PING0] main start");

    flush("[PING1] before getInstance");
    auto& tracker = MemoryTracker::getInstance();

    flush("[PING2] after getInstance");

    flush("[PING3] before registerAllocation");
    tracker.registerAllocation((void*)0x1, 40, "fake.cpp", 99, "int[]");
    flush("[PING4] after registerAllocation");

    flush("[PING5] before collectReport");
    auto report = tracker.collectReport();
    flush("[PING6] after collectReport");

    std::printf("[CHECK] total=%zu active=%zu current=%zu peak=%zu\n",
        report.stats.totalAllocations,
        report.stats.activeAllocations,
        report.stats.currentMemory, 
        report.stats.peakMemory);     

    std::printf("[CHECK] leaks count=%zu\n", report.leaks.size());

    for (auto& leak : report.leaks) {
        std::printf("LEAK addr=%p size=%zu file=%s:%d type=%s ts(ms)=%lld\n",
            leak.address,
            leak.size,
            leak.file.c_str(),
            leak.line,
            leak.typeName.c_str(),   
            (long long)leak.timestamp_ms); 
    }

    flush("[PING7] before unregisterAllocation");
    tracker.unregisterAllocation((void*)0x1);
    flush("[PING8] after unregisterAllocation");

    flush("[PING9] end");

    return 0;
}
