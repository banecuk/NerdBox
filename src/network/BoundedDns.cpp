#include "BoundedDns.h"

#include <Arduino.h>
#include <IPAddress.h>
#include <freertos/semphr.h>
#include <lwip/dns.h>
#include <lwip/tcpip.h>

#include <atomic>
#include <cstring>

namespace {

SemaphoreHandle_t gDoneSem = nullptr;

// Each resolve() call gets a fresh generation, passed through lwIP as the
// callback argument. A completion that arrives after its caller already gave
// up (timeout) carries an old generation and is ignored — so nothing the
// callbacks touch can outlive the call that's waiting on it.
std::atomic<uint32_t> gGeneration{0};
std::atomic<uint32_t> gResultAddr{0};

// Hostname for the in-flight request. Only read on the tcpip thread by
// startLookup(), which runs right after resolve() queues it (lwIP copies the
// name into its own table there).
char gHost[64] = "";

void finish(uint32_t gen, uint32_t addr) {
    if (gen != gGeneration.load()) {
        return;
    }
    gResultAddr.store(addr);
    xSemaphoreGive(gDoneSem);
}

uint32_t ipv4Of(const ip_addr_t* ipaddr) {
    if (ipaddr == nullptr || !IP_IS_V4(ipaddr)) {
        return 0;
    }
    return ip_2_ip4(ipaddr)->addr;
}

// lwIP DNS completion — runs on the tcpip thread.
void onDnsFound(const char* /*name*/, const ip_addr_t* ipaddr, void* arg) {
    finish(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg)), ipv4Of(ipaddr));
}

// Runs on the tcpip thread (queued via tcpip_callback). lwIP's raw DNS API
// isn't thread-safe, so it must not be called from the background task
// directly.
void startLookup(void* arg) {
    const uint32_t gen = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));
    ip_addr_t addr;
    const err_t err = dns_gethostbyname(gHost, &addr, &onDnsFound, arg);
    if (err == ERR_OK) {
        finish(gen, ipv4Of(&addr));  // answered from lwIP's cache
    } else if (err != ERR_INPROGRESS) {
        finish(gen, 0);
    }
    // ERR_INPROGRESS: onDnsFound() fires later.
}

}  // namespace

bool BoundedDns::resolve(const char* host, uint32_t timeoutMs) {
    IPAddress literal;
    if (literal.fromString(host)) {
        return true;
    }

    if (gDoneSem == nullptr) {
        gDoneSem = xSemaphoreCreateBinary();
        if (gDoneSem == nullptr) {
            return true;  // can't bound it — let HTTPClient try as before
        }
    }
    xSemaphoreTake(gDoneSem, 0);  // drain any stray give from an abandoned call

    const uint32_t gen = gGeneration.fetch_add(1) + 1;
    gResultAddr.store(0);
    strncpy(gHost, host, sizeof(gHost) - 1);
    gHost[sizeof(gHost) - 1] = '\0';

    if (tcpip_callback(&startLookup, reinterpret_cast<void*>(uintptr_t{gen})) != ERR_OK) {
        return true;  // couldn't queue it — let HTTPClient try as before
    }

    if (xSemaphoreTake(gDoneSem, pdMS_TO_TICKS(timeoutMs)) != pdTRUE) {
        gGeneration.fetch_add(1);  // orphan the late completion
        return false;
    }
    return gResultAddr.load() != 0;
}
