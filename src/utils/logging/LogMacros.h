#pragma once

// DEBUG_MODE is defined by the build environment (-DDEBUG_MODE=1 or =0).
// Default to 0 (off) so IntelliSense and any translation unit that omits the
// flag both compile cleanly.
#ifndef DEBUG_MODE
    #define DEBUG_MODE 0
#endif

// Logger::debug()/debugf() gate on DEBUG_MODE at *runtime* (Logger.cpp), so a
// release build still evaluates every call site's arguments, still performs
// the virtual call, and only then returns early. On a per-frame path that's
// real cost for zero output. LOG_DEBUG/LOG_DEBUGF gate at *compile time*
// instead: in a release build the call disappears entirely, arguments
// included, so use these (not logger.debug()/debugf() directly) at any new
// debug-log call site.
#if DEBUG_MODE
    // Variadic so call sites can still pass the optional forScreen bool, e.g.
    // LOG_DEBUG(logger_, "msg", true), exactly as they did through debug().
    #define LOG_DEBUG(logger, ...) (logger).debug(__VA_ARGS__)
    #define LOG_DEBUGF(logger, ...) (logger).debugf(__VA_ARGS__)
#else
    #define LOG_DEBUG(logger, ...) \
        do {                       \
        } while (0)
    #define LOG_DEBUGF(logger, ...) \
        do {                        \
        } while (0)
#endif
