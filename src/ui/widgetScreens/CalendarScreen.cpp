#include "CalendarScreen.h"

#include "ui/core/Layout.h"

namespace {
// Arrow buttons sit in the calendar's title row (0..34), flanking the month
// title rather than spanning the full grid height — this frees the entire
// screen width for the 7-column day grid underneath.
constexpr uint16_t kArrowW = 44;
constexpr uint16_t kArrowH = 34;

// Darker than ButtonWidget's default TFT_DARKGRAY background, so the
// month-navigation arrows read as subordinate to the back/settings buttons
// elsewhere, which keep the default shade.
constexpr uint16_t kArrowBg = 0x4208;
}  // namespace

CalendarScreen::CalendarScreen(LoggerInterface& logger, UiController* uiController,
                               const AppSettings& config)
    : BaseWidgetScreen(logger, uiController, config) {}

void CalendarScreen::createWidgets() {
    // Calendar grid — covers everything above the bottom back-button band.
    auto calendarWidget = std::unique_ptr<CalendarWidget>(
        new CalendarWidget(WidgetInterface::Dimensions{0, 0, Layout::kScreenW, 272}, 1000));
    CalendarWidget* calendar = calendarWidget.get();
    widgetManager_.addWidget(std::move(calendarWidget));

    // Prev/next-month arrows — drawn on top of the calendar widget's title
    // row so they're never overpainted by it (CalendarWidget confines its
    // title repaint to the inset between them). `calendar` outlives these
    // buttons: both are owned by the same widgetManager_.
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<", WidgetInterface::Dimensions{0, 0, kArrowW, kArrowH},
        0, EventType::NONE, [calendar](EventType) { calendar->stepMonth(-1); }, kArrowBg,
        TFT_WHITE)));

    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), ">",
        WidgetInterface::Dimensions{Layout::kScreenW - kArrowW, 0, kArrowW, kArrowH}, 0,
        EventType::NONE, [calendar](EventType) { calendar->stepMonth(1); }, kArrowBg, TFT_WHITE)));

    // Back button — same position/style as GameScreen/DiskScreen/WeatherScreen.
    widgetManager_.addWidget(std::unique_ptr<ButtonWidget>(new ButtonWidget(
        uiController_->getDisplayContext(), "<",
        WidgetInterface::Dimensions{0, Layout::kBottomBarY, Layout::kButtonSize,
                                    Layout::kButtonSize},
        0, EventType::SHOW_MAIN, [this](EventType action) { this->handleAction(action); },
        TFT_BLACK, TFT_WHITE)));

    // Clock — same position/colors as DiskScreen/WeatherScreen's.
    widgetManager_.addWidget(std::unique_ptr<ClockWidget>(
        new ClockWidget(WidgetInterface::Dimensions{328, 276, Layout::kClockW, 40}, 1000,
                        TFT_LIGHTGREY, TFT_BLACK)));
}
