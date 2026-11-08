#pragma once

#include <cstddef>
#include <cstring>

// UTF-8-aware fixed-buffer string copy (B31).
//
// snprintf("%s") truncates at a byte boundary, so a long non-ASCII string can
// end in half a multi-byte sequence, which LovyanGFX renders as a replacement
// box. copyTruncated() cuts at the last complete code point instead. Pure
// logic, no Arduino dependency; host-tested (test/Utf8Test.cpp).
namespace Utf8 {

inline bool isContinuation(unsigned char c) {
    return (c & 0xC0) == 0x80;
}

// Copies src into dst (capacity dstSize bytes, always NUL-terminated when
// dstSize > 0). If src doesn't fit, the copy is cut back to the start of the
// code point that would straddle the end. Returns the number of bytes copied
// (excluding the terminator). A null src copies as "".
inline size_t copyTruncated(char* dst, size_t dstSize, const char* src) {
    if (dst == nullptr || dstSize == 0) {
        return 0;
    }
    if (src == nullptr) {
        dst[0] = '\0';
        return 0;
    }
    const size_t srcLen = strlen(src);
    size_t end = srcLen < dstSize - 1 ? srcLen : dstSize - 1;
    if (end < srcLen) {
        // src[end] is the first excluded byte. If it continues a sequence,
        // walk back to that sequence's lead byte and exclude it as well.
        while (end > 0 && isContinuation(static_cast<unsigned char>(src[end]))) {
            --end;
        }
    }
    memcpy(dst, src, end);
    dst[end] = '\0';
    return end;
}

}  // namespace Utf8
