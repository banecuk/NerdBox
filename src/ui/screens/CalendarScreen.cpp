#include "CalendarScreen.h"

#include "ui/core/Colors.h"
#include "ui/core/Layout.h"

namespace {
// Arrow buttons sit in the calendar's title row (0..48), flanking the month
// title rather than spanning the full grid height — this frees the entire
// screen width for the 7-column day grid underneath. 48x48 meets the
// minimum tap-target size (see docs-local/03-visual-ux.md N11/V13).
constexpr uint16_t kArrowW = 48;
constexpr uint16_t kArrowH = 48;

// Darker than ButtonWidget's default TFT_DARKGRAY background, so the
// month-navigation arrows read as subordinate to the back/settings buttons
// elsewhere, which keep the default shade.
constexpr uint16_t kArrowBg = Colors::kMutedBlueGrey;
}  // namespace

CalendarScreen::CalendarScreen(LoggerInterface& logger, UiController* uiController,
                               const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config) {}

void CalendarScreen::createWidgets() {
    // Calendar grid — covers everything above the bottom back-button band.
    auto calendarWidget = std::make_unique<CalendarWidget>(
        WidgetInterface::Dimensions{0, 0, Layout::kScreenW, Layout::kContentH}, 1000);
    CalendarWidget* calendar = calendarWidget.get();
    widgetManager_.addWidget(std::move(calendarWidget), "calendar");

    // Prev/next-month arrows — drawn on top of the calendar widget's title
    // row so they're never overpainted by it (CalendarWidget confines its
    // title repaint to the inset between them). `calendar` outlives these
    // buttons: both are owned by the same widgetManager_.
    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            uiController_->getDisplayContext(), "<",
            WidgetInterface::Dimensions{0, 0, kArrowW, kArrowH}, 0, EventType::NONE,
            [calendar](EventType) { calendar->stepMonth(-1); }, kArrowBg, TFT_WHITE),
        "prev_month_button");

    widgetManager_.addWidget(
        std::make_unique<ButtonWidget>(
            uiController_->getDisplayContext(), ">",
            WidgetInterface::Dimensions{Layout::kScreenW - kArrowW, 0, kArrowW, kArrowH}, 0,
            EventType::NONE, [calendar](EventType) { calendar->stepMonth(1); }, kArrowBg,
            TFT_WHITE),
        "next_month_button");

    addBackButton();
    addBottomClock();
}
