#pragma once
// Empty on purpose — see freertos/semphr.h for the actual stub surface.
// `[env:native]` has no FreeRTOS to link against; this just satisfies the
// #include so host-testable headers that transitively pull it in compile.
