#pragma once

#include <cstdint>

// DNS lookup with a caller-chosen upper bound on how long it can block.
//
// HTTPClient resolves through WiFiGenericClass::hostByName(), which can block
// for up to 16 s waiting for another task's lookup to finish (the
// WIFI_DNS_IDLE_BIT handshake) plus up to 15 s for its own — with a router
// DNS forwarder that stops answering, a single internet fetch on the
// background task could outlast the 20 s task watchdog (B38).
//
// resolve() queues the query onto lwIP's tcpip thread (its raw DNS API is
// not thread-safe) and waits at most timeoutMs.
// On success the answer sits in lwIP's DNS cache, so HTTPClient's own lookup
// that follows returns immediately. On timeout/failure the caller should skip
// the request entirely and let its job's failure backoff retry later.
//
// Literal IPv4 addresses return true without touching DNS. Not reentrant:
// call from a single task (in practice the background task).
namespace BoundedDns {
bool resolve(const char* host, uint32_t timeoutMs);
}
