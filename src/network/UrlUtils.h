#pragma once

#include <cstddef>
#include <cstdint>

namespace UrlUtils {

// Splits a "http://host[:port][/path]" URL into host/port. Pure string
// parsing — no Arduino/network dependency — so it's host-tested under
// [env:native]. Truncates a host longer than outHostSize - 1; defaults to
// port 80 when no ":port" segment is present.
void parseHostPort(const char* url, char* outHost, size_t outHostSize, uint16_t& outPort);

}  // namespace UrlUtils
