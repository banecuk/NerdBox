#include "SseConnection.h"

#include <Arduino.h>

#include <cstring>

namespace {

bool containsCaseInsensitive(const char* haystack, size_t haystackLen, const char* needle) {
    const size_t needleLen = strlen(needle);
    if (needleLen == 0 || haystackLen < needleLen) {
        return false;
    }
    for (size_t i = 0; i + needleLen <= haystackLen; i++) {
        bool match = true;
        for (size_t j = 0; j < needleLen; j++) {
            if (tolower(static_cast<unsigned char>(haystack[i + j])) !=
                tolower(static_cast<unsigned char>(needle[j]))) {
                match = false;
                break;
            }
        }
        if (match) {
            return true;
        }
    }
    return false;
}

}  // namespace

SseConnection::SseConnection(LoggerInterface& logger, size_t eventBufferBytes,
                             uint16_t maxBytesPerPoll)
    : parser_(eventBufferBytes),
      logger_(logger),
      maxBytesPerPoll_(maxBytesPerPoll),
      readBuf_(std::make_unique<char[]>(maxBytesPerPoll)) {}

bool SseConnection::connect(const char* host, uint16_t port, const char* path,
                            uint16_t connectTimeoutMs, uint16_t headerTimeoutMs) {
    disconnect();

    client_.setTimeout(connectTimeoutMs);
    if (!client_.connect(host, port)) {
        logger_.debugf("SSE connect() failed: %s:%u", host, port);
        state_ = State::Error;
        return false;
    }

    client_.print("GET ");
    client_.print(path);
    client_.print(" HTTP/1.1\r\nHost: ");
    client_.print(host);
    client_.print("\r\nConnection: keep-alive\r\nAccept: text/event-stream\r\n\r\n");

    chunkedEncoding_ = false;
    chunkState_ = ChunkState::AwaitingSize;
    chunkSizeLineLen_ = 0;
    chunkRemaining_ = 0;
    parser_.reset();

    if (!readHeaders(headerTimeoutMs)) {
        client_.stop();
        state_ = State::Error;
        return false;
    }

    state_ = State::Connected;
    return true;
}

bool SseConnection::readHeaders(uint16_t headerTimeoutMs) {
    char headerBuf[kHeaderBufferBytes];
    size_t headerLen = 0;
    const unsigned long deadline = millis() + headerTimeoutMs;

    while (millis() < deadline) {
        const int c = client_.read();
        if (c < 0) {
            if (!client_.connected() && client_.available() == 0) {
                logger_.debug("SSE connection closed before headers arrived", true);
                return false;
            }
            continue;
        }

        if (headerLen >= kHeaderBufferBytes) {
            logger_.warning("SSE response headers exceeded buffer — treating as protocol error",
                            true);
            return false;
        }
        headerBuf[headerLen++] = static_cast<char>(c);

        if (headerLen >= 4 && memcmp(headerBuf + headerLen - 4, "\r\n\r\n", 4) == 0) {
            return parseHeaderBuf(headerBuf, headerLen);
        }
    }

    logger_.debug("SSE header read timed out", true);
    return false;
}

bool SseConnection::parseHeaderBuf(const char* buf, size_t len) {
    // Status line is the first "\r\n"-terminated line, e.g. "HTTP/1.1 200 OK".
    const char* crPos = static_cast<const char*>(memchr(buf, '\r', len));
    if (!crPos) {
        return false;
    }
    const size_t statusLineLen = crPos - buf;

    bool ok200 = false;
    for (size_t i = 0; i + 4 <= statusLineLen; i++) {
        if (buf[i] == ' ' && buf[i + 1] == '2' && buf[i + 2] == '0' && buf[i + 3] == '0') {
            ok200 = true;
            break;
        }
    }
    if (!ok200) {
        logger_.warningf("SSE stream rejected: %.*s", static_cast<int>(statusLineLen), buf);
        return false;
    }

    chunkedEncoding_ = containsCaseInsensitive(buf, len, "transfer-encoding") &&
                      containsCaseInsensitive(buf, len, "chunked");
    return true;
}

void SseConnection::poll(const SseEventParser::EventCallback& onEvent) {
    if (state_ != State::Connected) {
        return;
    }

    if (!client_.connected() && client_.available() == 0) {
        state_ = State::Disconnected;
        client_.stop();
        return;
    }

    uint16_t budget = maxBytesPerPoll_;
    while (budget > 0) {
        const int available = client_.available();
        if (available <= 0) {
            break;
        }
        const size_t toRead = static_cast<size_t>(available) < static_cast<size_t>(budget)
                                  ? static_cast<size_t>(available)
                                  : static_cast<size_t>(budget);
        const int n = client_.read(reinterpret_cast<uint8_t*>(readBuf_.get()), toRead);
        if (n <= 0) {
            break;
        }

        if (chunkedEncoding_) {
            feedChunked(readBuf_.get(), static_cast<size_t>(n), onEvent);
        } else {
            parser_.feed(readBuf_.get(), static_cast<size_t>(n), onEvent);
        }

        if (state_ != State::Connected) {
            break;  // feedChunked can disconnect on the terminal 0-length chunk
        }
        budget -= static_cast<uint16_t>(n);
    }
}

void SseConnection::feedChunked(const char* data, size_t len,
                                const SseEventParser::EventCallback& onEvent) {
    size_t i = 0;
    while (i < len) {
        switch (chunkState_) {
            case ChunkState::AwaitingSize: {
                const char c = data[i++];
                if (c == '\r') {
                    continue;
                }
                if (c == '\n') {
                    chunkSizeLine_[chunkSizeLineLen_] = '\0';
                    char* semicolon = strchr(chunkSizeLine_, ';');  // chunk extensions, unused
                    if (semicolon) {
                        *semicolon = '\0';
                    }
                    chunkRemaining_ = strtoul(chunkSizeLine_, nullptr, 16);
                    chunkSizeLineLen_ = 0;
                    chunkState_ = (chunkRemaining_ == 0) ? ChunkState::AwaitingTrailerBlank
                                                          : ChunkState::AwaitingData;
                } else if (chunkSizeLineLen_ < sizeof(chunkSizeLine_) - 1) {
                    chunkSizeLine_[chunkSizeLineLen_++] = c;
                }
                break;
            }
            case ChunkState::AwaitingData: {
                const size_t remainingInBuf = len - i;
                const size_t take =
                    chunkRemaining_ < remainingInBuf ? chunkRemaining_ : remainingInBuf;
                parser_.feed(data + i, take, onEvent);
                i += take;
                chunkRemaining_ -= take;
                if (chunkRemaining_ == 0) {
                    chunkState_ = ChunkState::AwaitingDataCrlf;
                }
                break;
            }
            case ChunkState::AwaitingDataCrlf: {
                const char c = data[i++];
                if (c == '\n') {
                    chunkState_ = ChunkState::AwaitingSize;
                }
                // A stray '\r' (or anything else here) is simply consumed.
                break;
            }
            case ChunkState::AwaitingTrailerBlank: {
                // The terminal "0\r\n" chunk, optionally followed by trailer
                // headers and a blank line, signals a clean end-of-stream.
                // NerdWinSense's SSE stream is meant to stay open
                // indefinitely, so reaching this state at all means the
                // server closed it — treat it the same as a dropped
                // connection and let the caller's reconnect/backoff handle
                // it, rather than trying to keep parsing trailer headers.
                logger_.debug("SSE stream sent a terminal chunk — disconnecting", true);
                state_ = State::Disconnected;
                client_.stop();
                return;
            }
        }
    }
}

void SseConnection::disconnect() {
    client_.stop();
    state_ = State::Disconnected;
    chunkState_ = ChunkState::AwaitingSize;
    chunkSizeLineLen_ = 0;
    chunkRemaining_ = 0;
    chunkedEncoding_ = false;
    parser_.reset();
}
