#pragma once

#include "Application.h"
#include "ApplicationComponents.h"

class ApplicationFactory {
   public:
    static Application* createApplication() {
        // Create and inject all components
        auto components = std::make_unique<ApplicationComponents>();

        // Create application with injected components
        return new Application(std::move(components));
    }

    static void destroyApplication(Application* app) { delete app; }
};