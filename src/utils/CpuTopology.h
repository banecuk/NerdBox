#pragma once
#include <cstdint>

// Maps a Cpu.CoreLoads index onto its core class for a hybrid CPU, given how
// many leading entries are P-core threads. Deliberately degrades to
// "everything is a P-core" (i.e. today's look) whenever the configuration and
// the reported thread count disagree, so a wrong kThreadsPerformanceCount or a
// CPU swap can never paint a misleading picture.
namespace CpuTopology {

enum class CoreClass : uint8_t { Performance, Efficiency };

inline bool hasSplit(uint8_t performanceThreads, uint8_t totalThreads) {
    return performanceThreads > 0 && performanceThreads < totalThreads;
}

inline CoreClass classOf(uint8_t index, uint8_t performanceThreads, uint8_t totalThreads) {
    if (!hasSplit(performanceThreads, totalThreads)) return CoreClass::Performance;
    return index < performanceThreads ? CoreClass::Performance : CoreClass::Efficiency;
}

// Index of the first Efficiency thread, or 0 when there is no split.
inline uint8_t splitIndex(uint8_t performanceThreads, uint8_t totalThreads) {
    return hasSplit(performanceThreads, totalThreads) ? performanceThreads : uint8_t(0);
}

}  // namespace CpuTopology
