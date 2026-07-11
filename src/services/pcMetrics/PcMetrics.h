#pragma once

#include <atomic>

#include <Arduino.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

struct DiskDrive {
    char driveName[4];  // "C", "D", etc. + null terminator
    float freeSpacePercent;
    float readKBPerSec;
    float writeKBPerSec;
};

// RAII guard: locks disk_drivesMutex on construction, releases on destruction.
// Usage:
//   { PcMetricsDiskLock lock(metrics); use metrics.disk_drives; }
class PcMetrics;
class PcMetricsDiskLock {
 public:
    explicit PcMetricsDiskLock(PcMetrics& m);
    ~PcMetricsDiskLock();
    PcMetricsDiskLock(const PcMetricsDiskLock&) = delete;
    PcMetricsDiskLock& operator=(const PcMetricsDiskLock&) = delete;

 private:
    PcMetrics& m_;
};

class PcMetrics {
 public:
    PcMetrics() = default;
    ~PcMetrics() { vSemaphoreDelete(disk_drivesMutex); }

    // There is exactly one PcMetrics instance for the app's lifetime. Copying
    // or moving it would duplicate/steal the SemaphoreHandle_t below — the
    // copy shares the same handle as the original, and both destructors would
    // delete it, double-freeing the semaphore.
    PcMetrics(const PcMetrics&) = delete;
    PcMetrics& operator=(const PcMetrics&) = delete;
    PcMetrics(PcMetrics&&) = delete;
    PcMetrics& operator=(PcMetrics&&) = delete;

    // Mutex protecting disk_drives only. All scalar fields (cpu_load, etc.) are
    // word-sized and accessed on Xtensa as naturally atomic — no lock needed.
    SemaphoreHandle_t disk_drivesMutex = xSemaphoreCreateMutex();

    // is_available is the cross-core publish flag: parseData() writes every
    // other field (including last_update_timestamp) before setting this,
    // and readers must check this first — see DataFreshnessGuard for the
    // happens-before argument that makes that ordering safe without a lock.
    std::atomic<bool> is_available{false};
    unsigned long last_update_timestamp = 0;

    uint8_t cpu_temperature = 0;
    uint8_t gpu_temperature = 0;

    uint8_t cpu_load = 0;
    uint8_t gpu_load = 0;
    uint8_t mem_load = 0;
    uint8_t cpu_thread_load[24] = {};

    uint16_t cpu_power = 0;
    uint16_t gpu_power = 0;

    uint16_t cpu_fan = 0;
    uint16_t gpu_fan = 0;

    // Compacted system fan RPMs — only non-zero (connected) headers are stored.
    // system_fan_count tells callers how many entries are valid.
    // Libre Hardware Monitor reports disconnected headers as 0; those are
    // filtered out in parseMotherboardData so indices here always map to real fans.
    static constexpr uint8_t kMaxSystemFans = 10;
    uint16_t system_fans[kMaxSystemFans] = {};
    uint8_t  system_fan_count = 0;

    uint8_t gpu_3d = 0;
    uint16_t gpu_compute = 0;
    uint16_t gpu_decode = 0;
    uint16_t gpu_mem = 0;
    int16_t gpu_fps = -1;  // Fullscreen FPS; -1 means not available

    float eth_up = 0;
    float eth_dn = 0;

    std::vector<DiskDrive> disk_drives;
};

// Inline RAII implementation — defined here so every TU that includes PcMetrics.h
// can use PcMetricsDiskLock without a separate .cpp.
inline PcMetricsDiskLock::PcMetricsDiskLock(PcMetrics& m) : m_(m) {
    xSemaphoreTake(m_.disk_drivesMutex, portMAX_DELAY);
}

inline PcMetricsDiskLock::~PcMetricsDiskLock() {
    xSemaphoreGive(m_.disk_drivesMutex);
}