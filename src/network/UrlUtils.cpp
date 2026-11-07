#include "network/UrlUtils.h"

#include <cstdlib>
#include <cstring>

namespace UrlUtils {

void parseHostPort(const char* url, char* outHost, size_t outHostSize, uint16_t& outPort) {
    outPort = 80;
    const char* schemeEnd = strstr(url, "://");
    const char* hostStart = schemeEnd ? schemeEnd + 3 : url;

    const char* slash = strchr(hostStart, '/');
    const char* colon = strchr(hostStart, ':');
    // Only a colon before the first '/' is a port separator — one in the
    // path or query string (e.g. "?t=a:b") is not (B61).
    if (colon && slash && colon > slash) {
        colon = nullptr;
    }
    const char* hostEnd = colon ? colon : (slash ? slash : hostStart + strlen(hostStart));

    size_t hostLen = static_cast<size_t>(hostEnd - hostStart);
    if (hostLen >= outHostSize) {
        hostLen = outHostSize - 1;
    }
    memcpy(outHost, hostStart, hostLen);
    outHost[hostLen] = '\0';

    if (colon) {
        outPort = static_cast<uint16_t>(atoi(colon + 1));
    }
}

}  // namespace UrlUtils
