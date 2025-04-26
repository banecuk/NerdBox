#include "core/utils/Logger.h"
#include "screens/IScreen.h"

class BootScreen : public IScreen {
   public:
    explicit BootScreen(ILogger& logger, LGFX* lcd);

    void initialize();
    void onEnter() override;
    void onExit() override;
    void draw() override;

   private:
    ILogger& logger_;
    LGFX* lcd_;

    int lineNumber_ = 0;
};