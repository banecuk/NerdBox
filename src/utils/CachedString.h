#pragma once

#include <cstddef>
#include <cstring>

// Fixed-capacity "last rendered text" cache for widgets that only redraw
// when their displayed string changes. Replaces the hand-written
// strcmp / strncpy / NUL-terminate triple that used to be repeated across a
// dozen widgets (and that two of them got subtly wrong by passing
// sizeof(dest) rather than sizeof(dest) - 1 to strncpy).
//
// The cache always holds at most N-1 chars plus a terminator. Comparisons
// use the same N-1 limit, so a source longer than the cache compares equal
// to its own truncated copy rather than reading as "changed" every tick.
//
// Header-only and Arduino-free — host-tested under [env:native]
// (test/CachedStringTest.cpp).
template <size_t N>
struct CachedString {
    static_assert(N >= 2, "CachedString needs room for at least one char + NUL");

    char value[N] = {};

    const char* c_str() const { return value; }

    bool equals(const char* s) const { return strncmp(value, s, N - 1) == 0; }

    void set(const char* s) {
        strncpy(value, s, N - 1);
        value[N - 1] = '\0';
    }

    // Stores `s` and returns true if it differs from the cached value;
    // returns false (and leaves the cache untouched) otherwise.
    bool setIfChanged(const char* s) {
        if (equals(s))
            return false;
        set(s);
        return true;
    }

    // Empties the cache, so the next setIfChanged() with any non-empty
    // string reports a change — i.e. forces the next draw.
    void clear() { value[0] = '\0'; }
};
