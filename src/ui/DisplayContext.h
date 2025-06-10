/**
 * @file DisplayContext.h
 * @brief Provides a context for display-related resources used by widgets and screens.
 *
 * The DisplayContext class serves as a lightweight container that bundles references to
 * essential display-related resources: the display driver (LGFX), color management
 * (Colors), and logging interface (LoggerInterface). It simplifies passing these
 * resources to widgets and screens, reducing parameter bloat and ensuring consistent
 * access.
 *
 * @note This class is designed to be passed by reference to avoid unnecessary copying.
 *       It does not own the resources it references; the caller is responsible for their
 * lifetime.
 */

#pragma once

#include "Colors.h"
#include "config/LgfxConfig.h"
#include "utils/LoggerInterface.h"

class DisplayContext {
   public:
    DisplayContext(LGFX& display, Colors& colors, LoggerInterface& logger)
        : display_(display), colors_(colors), logger_(logger) {}

    DisplayContext(const DisplayContext&) = delete;
    DisplayContext& operator=(const DisplayContext&) = delete;

    /**
     * @brief Gets the display driver.
     * @return Reference to the LGFX display driver.
     */
    LGFX& getDisplay() { return display_; }

    /**
     * @brief Gets the color management object.
     * @return Reference to the Colors object.
     */
    Colors& getColors() { return colors_; }

    /**
     * @brief Gets the logger interface.
     * @return Reference to the LoggerInterface.
     */
    LoggerInterface& getLogger() { return logger_; }

   private:
    LGFX& display_;
    Colors& colors_;
    LoggerInterface& logger_;
};