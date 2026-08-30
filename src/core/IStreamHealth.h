#pragma once

#include <cstdint>

// Narrow interface exposing the SSE-stream health fields WebApiHandlers needs
// for /api/status and /metrics, so services/web/ does not depend on the
// concrete SseStreamJob (which lives in app/) — see docs-local/11-code-quality.md Q3.
class IStreamHealth {
 public:
    virtual ~IStreamHealth() = default;
    virtual const char* stateName() const = 0;
    virtual uint32_t reconnectCount() const = 0;
    virtual uint32_t lastEventAgeMs() const = 0;
    virtual uint32_t overflowCount() const = 0;
};
