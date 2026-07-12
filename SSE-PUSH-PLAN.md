# SSE metrics push — implementation plan

Status: **all 7 milestones done.** `pcMetricsStreamEnabled` defaults to
**true** — a deliberate call to accept residual risk (see milestone 6)
rather than a claim that the long soak / `MaxStreamClients` gate was
fully met; `PcMetricsJob` stays in the tree as an instant no-code-change
fallback (`pcMetricsStreamEnabled = false`) if streaming misbehaves in
the field. `CLAUDE.md` updated to document the new data flow and fix its
pre-existing stale references. Remaining open items (not blockers, just
not yet done): a genuinely long soak, `MaxStreamClients` rejection
behavior, and eventually removing `PcMetricsJob` once streaming has
proven itself over more than one session's testing.

**Real-hardware validation performed this session** (device at
192.168.1.23, `pcMetricsStreamEnabled` flipped to `true` and flashed):
- `curl -N` against the live stream endpoint confirmed: `Transfer-Encoding:
  chunked` **is** used (justifying `SseConnection`'s hand-rolled dechunk
  over `HTTPClient::getStream()`); each `data:` payload **is**
  `Metrics`-wrapped matching the polling shape; query params
  `intervalMs`/`delta` are correct (echoed back in the server's
  `event: connected` handshake); delta mode omits unchanged sections as
  expected; max observed event size ~1.6 KB, comfortably under the 4096 B
  buffer.
- `GET /api/status`'s `pc_stream` object showed `state: CONNECTED` stable
  for 3+ minutes with `reconnect_count: 0`, `overflow_count: 0`, and
  `heap_free` oscillating 216–218 KB with no downward trend (no leak).
  `GET /api/pc` confirmed all fields (CPU/GPU/RAM/disk/network/fans)
  populating correctly, `pc.fetch_ok/fetch_fail` staying `0/0` (confirms
  `PcMetricsJob` polling correctly stayed mutually-excluded).
- Restarted NerdWinSense mid-stream: metrics froze as expected, then
  `PcMetricsStreamJob` detected the drop and reconnected
  (`reconnect_count` went `0` → `1`, `state` back to `CONNECTED`,
  `pc.fresh` back to `true`) with no watchdog reset (uptime climbed
  continuously through the outage). This is the milestone 5 soak/reconnect
  scenario, confirmed working.
- **Not tested**: `MaxStreamClients` rejection (503) behavior, and a
  genuinely long (hours+) soak. Multi-line `data:` events and the
  buffer-overflow path were never exercised on real hardware either (no
  event came close to the 4096 B cap) — covered only by
  `SseEventParserTest`'s synthetic cases.
- One hardware hiccup, unrelated to the firmware logic: rapid repeated
  auto-reset attempts during upload/monitor tooling left the board's
  native-USB-CDC port in a bad state (`COM5` vanished, Windows reported
  device-descriptor error 43) until a manual unplug/replug — a known
  quirk of this board's 1200bps-touch auto-reset, not a firmware crash.

## Goal

Replace (or, for a transition period, run alongside) the current polling of
`GET /api/v1/system` with a long-lived SSE connection to
`GET /api/v1/stream?intervalMs=&delta=`, per the NerdWinSense server-push
protocol doc. Delta mode is the target end state — it cuts bandwidth/CPU by
only re-sending sections that changed since the last tick.

## Current architecture (baseline, verified against source)

- `PcMetricsService::fetchData()` (`src/services/pcMetrics/PcMetricsService.cpp`)
  does one bounded HTTP GET per call via `HttpClient::downloadAndParse()`,
  which streams the response body straight into a reused
  `std::unique_ptr<JsonDocument> doc_` with a filtered `deserializeJson`
  (only requested keys are parsed). There is no `String` body buffer on this
  path.
- `PcMetricsJob` (`src/core/jobs/PcMetricsJob.h`) is a `BackgroundJob`:
  `nextDueMs()` gates on core-initialized / screen MAIN or GAME / WiFi
  connected, `run()` calls `fetchData()` once and reschedules itself
  `hardwareMonitorRefreshMs` (500 ms) later on success or
  `hardwareMonitorFailureRefreshMs` (3000 ms) later on failure.
- `TaskManager::executeBackgroundTask()` ticks every 20 ms, iterates
  `jobs_`, calls each job's `run()` if due, and feeds the watchdog
  (20 s timeout) every tick **regardless of what jobs did**. Jobs must
  return quickly — nothing here is designed to block.
- Parsed data lands in the single long-lived `PcMetrics` instance
  (`src/services/pcMetrics/PcMetrics.h`). Cross-core publish relies on
  `is_available` (atomic, written **last**) then `last_update_timestamp`,
  consumed via `DataFreshnessGuard` (`src/utils/DataFreshnessGuard.h`),
  which reads `isAvailable` before the timestamp to make the ordering
  safe without a lock. `disk_drives` (a `std::vector`) is the one field
  guarded by an explicit mutex (`PcMetricsDiskLock`).
- Per-field parse methods (`parseCpuData`, `parseGpuData`,
  `parseMotherboardData`, `parseDiskData`, `parseCpuExtendedData`,
  `parseRamData`, `parseNetworkData`) live as private methods on
  `PcMetricsService` and assume a **full** report is always present.
  `parseCpuData`/`parseMotherboardData` build a local stack array first,
  then commit it to the shared struct in one tight loop, to avoid a torn
  read from the screen task.
- `HttpClient` (`src/network/HttpClient.h/.cpp`) wraps Arduino's
  `HTTPClient`/`WiFiClient`. It forces `useHTTP10(true)` for the polling
  path specifically so the body is a plain `Content-Length` response, not
  chunked — i.e. it is built around bounded request/response, not a
  connection that stays open indefinitely.
- No SSE client, no chunked-transfer decode, no `WiFiClientSecure`
  anywhere in the repo today. The closest precedent is the **outbound**
  streamed JSON in `WebServerService.cpp` (`/api/status`, via
  `server_.sendContent()`), which is the ESP32 acting as HTTP server, not
  client — not directly reusable, but confirms chunked I/O is not new to
  this codebase's mental model.

## Design constraints this plan must respect

1. **No blocking reads inside a `BackgroundJob::run()`.** A long-lived
   socket must be read incrementally, a bounded number of bytes per
   20 ms tick, never with a blocking `connect()`/`readStringUntil()` that
   could stall past the 20 s watchdog timeout.
2. **No `String` for held state** — the SSE line/event buffer must be a
   fixed-size (or heap-allocated once, PSRAM-preferred) `char` buffer,
   not a growing `String`.
3. **Delta-mode absent-key semantics are load-bearing.** "Key not
   present" means "unchanged," not "zero this field out." Every
   `parseXData` helper must only touch `outData` fields when its JSON
   object is actually present in the current event — this is a real
   change from today's always-full-report assumption and needs an
   explicit audit (see Phase 1).
4. **Keep the `is_available`-before-`timestamp` publish ordering** in
   `DataFreshnessGuard` intact for any new writer path.
5. **Reuse, don't duplicate, the parse logic.** `PcMetricsService`'s
   per-section parsers are the single source of truth for
   "NerdWinSense JSON → `PcMetrics` fields." The SSE path must call the
   same functions rather than re-implement CPU/GPU/RAM/disk parsing a
   second time.
6. **`AppSettings` is the only config extension point** — there is no
   `AppConfigInterface`/`AppConfigService` (despite CLAUDE.md's stale
   description of one). New tuning knobs go in
   `AppConfig::internal::HardwareMonitorImpl` (or a new struct) and get a
   matching field in `AppSettings.h`.
7. **Fall back gracefully.** The server can reject a stream at
   `MaxStreamClients` with a plain `503`, and any TCP drop just closes
   the connection with no in-band error event. The client must detect
   both and reconnect with backoff, mirroring `PcMetricsJob`'s existing
   failure/backoff pattern.

## Proposed shape

### New files

- `src/services/pcMetrics/PcMetricsParser.h/.cpp`
  Extracted from `PcMetricsService`: free functions (or a small stateless
  class) taking a `JsonObjectConst` (the `Metrics` object) and a
  `PcMetrics&` to fill in. One function per section, each guarded by
  `containsKey()`/`isNull()` so an absent section is a true no-op. Both
  the polling path and the new streaming path call into this. This is
  pure logic with no Arduino/network dependency beyond ArduinoJson, so it
  can plausibly get host-side (`native`) test coverage the same way
  `ValueSmoother` does — worth checking whether ArduinoJson builds under
  the `native` env; if not, keep the extraction anyway for the DRY
  benefit and skip native tests for this piece.
- `src/services/pcMetrics/SseEventParser.h/.cpp`
  Pure byte-buffer framing logic, hardware-independent, unit-testable
  under `native`: feed it arbitrary chunks of bytes (`feed(const char*
  data, size_t len)`), it accumulates into an internal fixed-capacity
  buffer, and calls back (or returns a small result struct) whenever a
  full SSE event (`event:` line optional + one/more `data:` lines +
  terminating blank line) is complete. Must handle: multi-line `data:`
  reassembly, a line split across two `feed()` calls, and buffer-full
  degradation (log + reset rather than silently corrupt).
- `src/network/SseConnection.h/.cpp` (or under
  `src/services/pcMetrics/`)
  Owns a `WiFiClient` (plain, matching the non-TLS `http://` endpoint),
  drives connect → send SSE GET request line/headers → non-blocking
  incremental read loop feeding `SseEventParser` → detects
  disconnect/error. Exposes something like:
  ```cpp
  enum class State { Disconnected, Connecting, Connected, Error };
  State state() const;
  void connect(const char* host, uint16_t port, const char* path);
  void poll(); // call every background tick; reads whatever is available, non-blocking
  void disconnect();
  ```
  `poll()` must cap bytes read per call (e.g. 512–1024 B) so one job tick
  can't spend unbounded time even if the OS socket buffer is full.
- `src/core/jobs/PcMetricsStreamJob.h`
  A `BackgroundJob` shaped like `PcMetricsJob`: `nextDueMs()` gates on
  the same conditions (core ready / MAIN or GAME screen / WiFi
  connected) plus connection state; `run()` calls `connection_.poll()`
  every tick when connected, or attempts a (non-blocking) connect when
  disconnected and due for a reconnect attempt. On a complete event from
  `SseEventParser`, deserializes via the existing filtered `JsonDocument`
  and calls into `PcMetricsParser` — same commit-ordering rules as today
  (`is_available` last).

### Config additions (`AppConfig.h` + `AppSettings.h`)

New struct, e.g. `AppConfig::internal::PcMetricsStreamImpl`:
- `kEnabled` (bool) — feature flag; default `false` until validated on
  hardware, then flip to `true` once `PcMetricsJob` (polling) is retired.
- `kIntervalMs` (default 500, matching current `kRefreshMs` cadence)
- `kDelta` (default `true`)
- `kConnectTimeoutMs` (e.g. 1000, matching existing HTTP connect timeout)
- `kReconnectBackoffMs` (e.g. 2000, mirrors `kRefreshAfterFailureMs`)
- `kMaxEventBufferBytes` (size of the fixed accumulation buffer —
  needs a real number: measure a full filtered report's JSON size against
  the current filter and pad, e.g. 2048–4096 B, allocate via
  `std::make_unique<char[]>` so it can land in PSRAM rather than as a
  stack/static member)

Corresponding fields threaded into `AppSettings` following the existing
pattern (`hardwareMonitor*` naming precedent).

### Wiring

- `ApplicationComponents` constructs `PcMetricsStreamJob` alongside (not
  replacing, initially) `PcMetricsJob`, gated by
  `settings.pcMetricsStreamEnabled`. Only one of the two jobs should
  actually be *due* at a time — simplest approach: `PcMetricsJob`'s
  `nextDueMs()` also returns `ULONG_MAX` when stream mode is enabled, so
  they're mutually exclusive at runtime, not just by feature intent.
- No changes needed to widgets — they already read `PcMetrics` +
  `DataFreshnessGuard`, agnostic to poll vs push.

## Milestones

1. **DONE. Extract `PcMetricsParser`** out of `PcMetricsService`
   (`src/services/pcMetrics/PcMetricsParser.h/.cpp`), refactor
   `PcMetricsService::parseData` to call it — both firmware envs build
   clean. Every section parser now guards each field with `isNull()`
   (ArduinoJson v7's `containsKey()` is deprecated) so an absent key
   leaves `outData` untouched instead of falling back to a zero default —
   required for delta mode's "absent means unchanged" contract.
   `parseDiskData` no longer clears `disk_drives` when `Drives` is absent
   (previously did, since the polling path always sent a full report and
   this case never came up). One deliberate exception:
   `Gpu.FullscreenFps` is only reachable in the current always-full
   filtered report, so absence there still means "no fullscreen app", not
   "unchanged" — revisit if delta mode ever needs this field.
2. **DONE. `SseEventParser`** (`src/services/pcMetrics/SseEventParser.h/.cpp`)
   as an isolated, host-testable class — pure byte-buffer SSE framing, no
   Arduino/network dependency. `test/SseEventParserTest.cpp` covers
   multi-line `data:`, CRLF, comments, split-across-`feed()` calls
   (including byte-by-byte), and line/data buffer-overflow recovery; all
   32 native tests (existing `ValueSmootherTest` + new) pass. Wired into
   `platformio.ini`'s `[env:native]` `build_src_filter`/`-I` list.
   `PcMetricsParser` was **not** added to the `native` env — `PcMetrics.h`
   pulls in `Arduino.h`/FreeRTOS headers, so it can't build host-side
   without a larger refactor; per the note above, the DRY extraction was
   kept and native coverage was skipped for that file, per this plan's
   original caveat.
3. **DONE, VERIFIED ON HARDWARE. `SseConnection`**
   (`src/services/pcMetrics/SseConnection.h/.cpp`) — hand-rolls the GET
   over a raw `WiFiClient` rather than building on `HTTPClient::getStream()`:
   `HTTPClient` only applies its chunked-transfer decoding inside
   `getString()`/`writeToStream()`, not on the raw stream `getStream()`
   hands back, so reading that stream directly would have fed raw
   chunk-framing bytes into `SseEventParser`. **Confirmed via `curl -N`
   against the live NerdWinSense endpoint that it does send
   `Transfer-Encoding: chunked`** — the dechunk path is real, not
   defensive-only, and the hand-rolled approach was the right call.
   `connect()` performs one bounded blocking TCP-connect + header-read
   (same pattern `PcMetricsJob`'s polling fetch already uses); `poll()`
   only reads bytes already buffered by the socket, capped by
   `maxBytesPerPoll` (default 512 B/tick). On-device: connection held
   `CONNECTED` for 3+ minutes straight, survived a real NerdWinSense
   restart via reconnect-with-backoff, no heap growth.
4. **DONE, VERIFIED ON HARDWARE. `PcMetricsStreamJob`**
   (`src/core/jobs/PcMetricsStreamJob.h/.cpp`), wired into
   `ApplicationComponents`/`TaskManager`'s job list alongside
   `PcMetricsJob`, gated by the new `AppConfig::internal::PcMetricsStreamImpl`
   struct (`AppSettings.pcMetricsStreamEnabled`). `PcMetricsJob::nextDueMs()`
   returns `ULONG_MAX` whenever streaming is enabled, so the two are
   mutually exclusive at runtime as designed — confirmed on-device via
   `pc.fetch_ok`/`fetch_fail` staying `0/0` the whole time streaming was
   active. `PcMetricsParser::buildFilter()` was factored out of
   `PcMetricsService::initFilter()` so both paths build the identical
   ArduinoJson filter from one definition. `GET /api/status` gained a
   `"pc_stream"` object — `enabled`, `state`, `reconnect_count`,
   `last_event_age_ms`, `overflow_count` — used directly to confirm all of
   the above live. `/config` lists the new `pcMetricsStream*` tuning
   constants.

   **Confirmed on real hardware** (see status header): event payload is
   `"Metrics"`-wrapped matching the polling response;
   `intervalMs`/`delta` are the correct query parameter names (echoed back
   by the server's `event: connected` handshake); `kMaxEventBufferBytes`
   (4096 B) has ample headroom over the ~1.6 KB observed max event size;
   `handleEvent`'s "any section present counts as a fresh update" delta
   semantics work correctly with real delta payloads.
5. **DONE (core scenario), PARTIAL. Soak test**: reconnect-with-backoff
   confirmed working against a real NerdWinSense restart — metrics froze,
   `PcMetricsStreamJob` detected the drop, reconnected
   (`reconnect_count` 0→1), resumed streaming, no watchdog reset. Heap
   stable (216–218 KB, no trend) over a ~3 minute window. **Still not
   done**: a long (hours+) soak, and confirming behavior when
   `MaxStreamClients` is hit (should back off, not spin) — never
   triggered since only one client connected during testing.
6. **DONE. Cut over**: `AppConfig::internal::PcMetricsStreamImpl::kEnabled`
   flipped to `true` — a deliberate decision to accept the residual risk
   on the untested scenarios (long soak, `MaxStreamClients` rejection)
   rather than a claim they've been validated. `PcMetricsJob` (polling)
   stays in the tree, documented in its own header comment as the
   retained fallback — flip the flag back to `false` to revert with no
   code change if streaming misbehaves in the field. Both firmware envs
   build clean. Removing `PcMetricsJob` entirely is explicitly deferred to
   a follow-up once the stream path has actually proven stable over a
   longer run.
7. **DONE. Docs**: `CLAUDE.md`'s directory layout, architecture rules,
   tuning reference, data-flow section, memory-discipline bullet, web
   server endpoint table, and known-issues section all updated for the
   new SSE push path (`PcMetricsParser`/`SseEventParser`/`SseConnection`/
   `PcMetricsStreamJob`, `core/jobs/`, `pc_stream` in `/api/status`). Also
   fixed the two pre-existing stale-doc issues noticed during this
   planning pass:
   - the `String rawData_` / `lastSuccessfulFetchTime_` / `isDataStale()`
     description no longer matched code (now describes the reused
     `JsonDocument` + `DataFreshnessGuard`)
   - `AppConfigInterface.h`/`AppConfigService.h` were referenced but no
     longer exist (now describes the plain `AppSettings` struct)

## Open questions to confirm before/while implementing

- RESOLVED: measured directly against the live endpoint (9 disks, 18
  cores) — max observed event ~1.6 KB. `kMaxEventBufferBytes` (4096 B)
  has 2.5x headroom.
- RESOLVED (see milestone 3): went with a raw `WiFiClient` rather than
  `HTTPClient::getStream()`, specifically to be able to detect and
  correctly dechunk `Transfer-Encoding: chunked` responses ourselves —
  `HTTPClient` only decodes chunking inside its own `getString()`/
  `writeToStream()` helpers, not on the raw stream `getStream()` returns.
  Confirmed via `curl -N` that NerdWinSense's stream endpoint does send
  chunked encoding, and confirmed on-device that the dechunk logic
  handles it correctly.
- Whether `fetchRawJson()` (`GET /api/raw`, used by the web server) stays
  on the old polling `HttpClient` regardless — yes, that's a dedicated
  one-shot fetch, unaffected by this change.
