#pragma once

#include <cstddef>
#include <cstring>

#include "services/pcMetrics/PcMetrics.h"
#include "utils/ScopedLock.h"

// The one place PcMetrics::disk_drives is read under its mutex outside the
// parser that writes it. Every consumer (DiskBandWidget, DiskInfoWidget,
// DiskSummaryWidget, /api/pc, /metrics) used to re-establish the "snapshot
// under lock, then work lock-free" rule with its own hand-written struct and
// copy loop; they now all go through here, so the lock is never held across
// drawing or network I/O by construction.
namespace DiskSnapshot {

// Upper bound for callers that don't have their own on-screen cap (the web
// endpoints). A PC with more drives than this just reports the first 16.
constexpr size_t kMaxDrives = 16;

// Copies up to `max` drives out from under disk_drivesMutex into `out`.
// driveName is always NUL-terminated in the copy. Returns the count copied.
inline size_t copy(PcMetrics& metrics, DiskDrive* out, size_t max) {
    ScopedLock lock(metrics.disk_drivesMutex);
    const size_t n = metrics.disk_drives.size() < max ? metrics.disk_drives.size() : max;
    for (size_t i = 0; i < n; ++i) {
        out[i] = metrics.disk_drives[i];
        out[i].driveName[sizeof(out[i].driveName) - 1] = '\0';
    }
    return n;
}

// Sum of read/write rates across every drive — DiskSummaryWidget's view,
// which needs totals rather than a per-drive copy.
inline void totals(PcMetrics& metrics, float& readKBps, float& writeKBps) {
    readKBps = 0.0f;
    writeKBps = 0.0f;
    ScopedLock lock(metrics.disk_drivesMutex);
    for (const DiskDrive& d : metrics.disk_drives) {
        readKBps += d.readKBPerSec;
        writeKBps += d.writeKBPerSec;
    }
}

// True when the drive *set* changed — the count differs, or the count is the
// same but some name at the same index differs (e.g. D: unplugged and E:
// appearing in its slot). A count-only check leaves the old drive's cached
// name/state on screen forever; DiskBandWidget and DiskInfoWidget each had
// to learn that separately. `cachedName(i)` returns the caller's cached
// name for slot i.
template <typename CachedNameFn>
bool driveSetChanged(const DiskDrive* snap, size_t n, size_t cachedCount,
                     CachedNameFn cachedName) {
    if (n != cachedCount)
        return true;
    for (size_t i = 0; i < n; ++i) {
        if (strncmp(cachedName(i), snap[i].driveName, sizeof(snap[i].driveName)) != 0)
            return true;
    }
    return false;
}

}  // namespace DiskSnapshot
