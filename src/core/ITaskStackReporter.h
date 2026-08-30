#pragma once

#include <cstdint>

// Narrow interface exposing the task stack high-water marks WebApiHandlers
// needs for /api/status, so services/web/ does not depend on the concrete
// TaskManager (which lives in app/) — see docs-local/11-code-quality.md Q3.
class ITaskStackReporter {
 public:
    virtual ~ITaskStackReporter() = default;
    virtual uint32_t screenTaskStackFree() const = 0;
    virtual uint32_t backgroundTaskStackFree() const = 0;
};
