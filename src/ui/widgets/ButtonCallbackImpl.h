#pragma once
#include "ButtonCallback.h"

template <typename T>
class ButtonCallbackImpl : public ButtonCallback {
   public:
    typedef void (T::*CallbackMethod)();

    ButtonCallbackImpl(T* instance, CallbackMethod method)
        : instance_(instance), method_(method) {}

    void onButtonPressed() override {
        if (instance_ && method_) {
            (instance_->*method_)();
        }
    }

   private:
    T* instance_;
    CallbackMethod method_;
};