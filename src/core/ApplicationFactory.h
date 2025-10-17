#pragma once

#include "Application.h"

class ApplicationFactory {
public:
    static Application* createApplication() {
        return new Application();
    }
    
    static void destroyApplication(Application* app) {
        delete app;
    }
};