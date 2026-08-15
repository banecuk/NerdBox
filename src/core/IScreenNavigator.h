#pragma once

#include "core/ScreenTypes.h"

// Narrow interface exposing the single operation WebServerService needs to
// drive screen navigation from a POST /screen/* handler, so services/ does
// not depend on the concrete UiController (which lives in ui/). Sibling of
// IScreenUpdater — same motivation.
class IScreenNavigator {
 public:
    virtual ~IScreenNavigator() = default;
    virtual void requestScreen(ScreenName screen) = 0;
};
