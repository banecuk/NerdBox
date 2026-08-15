#pragma once

#include "LogTypes.h"

// The log sink: five levels, each as a `const char*` overload plus a
// formatted (`f`-suffix) variant. The boot-screen message queue
// (ScreenLogQueue) and the non-destructive recent-log ring (RecentLogView)
// are separate interfaces — most consumers only ever need to log, and
// shouldn't have to implement (or depend on) the other two concerns.
class LoggerInterface {
 public:
    virtual ~LoggerInterface() = default;

    virtual void debug(const char* message, bool forScreen = false) = 0;
    virtual void info(const char* message, bool forScreen = false) = 0;
    virtual void warning(const char* message, bool forScreen = false) = 0;
    virtual void error(const char* message, bool forScreen = false) = 0;
    virtual void critical(const char* message, bool forScreen = false) = 0;

    // Formatted log methods
    virtual void debugf(const char* format, ...) = 0;
    virtual void infof(const char* format, ...) = 0;
    virtual void warningf(const char* format, ...) = 0;
    virtual void errorf(const char* format, ...) = 0;
    virtual void criticalf(const char* format, ...) = 0;
};
