#pragma once

#include <WiFiClient.h>

#include <cstddef>
#include <cstdint>
#include <memory>

#include "services/pcMetrics/SseEventParser.h"
#include "utils/LoggerInterface.h"

// Owns a single long-lived plain-HTTP (`http://`, matching NerdWinSense's
// non-TLS endpoint) connection to an SSE stream. The request is hand-rolled
// over a raw WiFiClient rather than built on Arduino's HTTPClient: this
// codebase's HttpClient wrapper (src/network/HttpClient.h) is designed
// around a bounded request/response, and HTTPClient::getStream() would hand
// back the raw socket without applying its own chunked-transfer decoding
// (that only happens inside getString()/writeToStream()) — so reading
// getStream() directly for an indefinite-length SSE body risks feeding raw
// chunk-framing bytes into the event parser if NerdWinSense's stream
// endpoint uses "Transfer-Encoding: chunked". Hand-rolling the request lets
// this class detect that header and dechunk before parsing, whichever way
// the server behaves. See SSE-PUSH-PLAN.md's "open questions" section.
//
// connect() performs one bounded, blocking TCP-connect + header-read step —
// the same "occasional blocking call inside a BackgroundJob::run()" pattern
// PcMetricsJob's polling fetchData() already relies on (bounded by
// connectTimeoutMs / headerTimeoutMs, not indefinite). poll() is the
// steady-state path and never blocks: it only reads bytes the socket has
// already buffered, capped at maxBytesPerPoll, so a single BackgroundJob
// tick can't stall regardless of how much data is queued up.
//
// NOT YET VALIDATED AGAINST REAL HARDWARE. Milestone 3 of SSE-PUSH-PLAN.md
// calls for benching this against the actual NerdWinSense stream endpoint
// (with `curl -N` as an independent reference for expected framing) before
// it's trusted — in particular whether the server actually sends
// "Transfer-Encoding: chunked" or a plain unbounded body, and whether any
// intermediate (proxy, corporate AP) reshapes the response.
class SseConnection {
 public:
    enum class State { Disconnected, Connected, Error };

    explicit SseConnection(LoggerInterface& logger, size_t eventBufferBytes,
                            uint16_t maxBytesPerPoll = 512);

    State state() const { return state_; }

    // host/port/path build the request by hand (see class comment for why).
    // Blocking, bounded by connectTimeoutMs (TCP connect) + headerTimeoutMs
    // (status line + headers) — call only when a (re)connect is actually
    // due, not every tick.
    bool connect(const char* host, uint16_t port, const char* path,
                 uint16_t connectTimeoutMs = kDefaultConnectTimeoutMs,
                 uint16_t headerTimeoutMs = kDefaultHeaderTimeoutMs);

    // Reads whatever is already available on the socket (bounded by
    // maxBytesPerPoll), dechunks it if the response used
    // "Transfer-Encoding: chunked", and feeds the result into the SSE
    // parser, dispatching each complete event via onEvent. No-op if not
    // Connected. Detects a server-side close and transitions to
    // Disconnected.
    void poll(const SseEventParser::EventCallback& onEvent);

    void disconnect();

    uint32_t overflowCount() const { return parser_.overflowCount(); }
    bool isChunkedEncoding() const { return chunkedEncoding_; }

 private:
    enum class ChunkState { AwaitingSize, AwaitingData, AwaitingDataCrlf, AwaitingTrailerBlank };

    bool readHeaders(uint16_t headerTimeoutMs);
    bool parseHeaderBuf(const char* buf, size_t len);
    void feedChunked(const char* data, size_t len, const SseEventParser::EventCallback& onEvent);

    static constexpr uint16_t kDefaultConnectTimeoutMs = 1000;
    static constexpr uint16_t kDefaultHeaderTimeoutMs = 2000;
    static constexpr size_t kHeaderBufferBytes = 512;

    WiFiClient client_;
    SseEventParser parser_;
    LoggerInterface& logger_;
    State state_ = State::Disconnected;
    uint16_t maxBytesPerPoll_;
    bool chunkedEncoding_ = false;

    ChunkState chunkState_ = ChunkState::AwaitingSize;
    char chunkSizeLine_[16] = "";
    uint8_t chunkSizeLineLen_ = 0;
    size_t chunkRemaining_ = 0;

    std::unique_ptr<char[]> readBuf_;  // scratch buffer, sized maxBytesPerPoll_
};
