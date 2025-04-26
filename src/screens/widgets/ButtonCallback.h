#pragma once

class ButtonCallback {
   public:
    virtual ~ButtonCallback() = default;
    virtual void onButtonPressed() = 0;
};