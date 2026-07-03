#pragma once

// Narrow interface exposing the single UI operation TaskManager needs to
// drive from the screen-update task, so core/ does not depend on the
// concrete UiController (which lives in ui/).
class IScreenUpdater {
 public:
    virtual ~IScreenUpdater() = default;
    virtual void updateDisplay() = 0;
};
