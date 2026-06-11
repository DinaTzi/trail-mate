#include "platform/ui/orientation_runtime.h"

#if defined(TRAIL_MATE_ESP_BOARD_TAB5)
#include "boards/tab5/heading_runtime.h"
#endif

namespace platform::ui::orientation
{

HeadingState get_heading()
{
#if defined(TRAIL_MATE_ESP_BOARD_TAB5)
    return ::boards::tab5::heading_runtime::get_data();
#else
    return {};
#endif
}

void ensure_heading_runtime()
{
#if defined(TRAIL_MATE_ESP_BOARD_TAB5)
    ::boards::tab5::heading_runtime::ensure_started();
#endif
}

ScreenOrientationState get_screen_orientation()
{
    ScreenOrientationState state{};
#if defined(TRAIL_MATE_ESP_BOARD_TAB5)
    const HeadingState heading = get_heading();
    state.policy = ScreenOrientationPolicy::SensorLandscapeOnly;
    state.sensor_available = true;
    state.sensor_ready = heading.sensor_ready;
    state.sensor_requested_orientation = ScreenOrientation::Landscape;
    state.sensor_request_ignored = false;
#elif defined(TRAIL_MATE_ESP_BOARD_T_DISPLAY_P4)
    state.policy = ScreenOrientationPolicy::SensorLandscapeOnly;
    state.sensor_available = true;
    state.sensor_ready = false;
    state.sensor_requested_orientation = ScreenOrientation::Landscape;
    state.sensor_request_ignored = false;
#endif
    state.active_orientation = ScreenOrientation::Landscape;
    state.portrait_supported = false;
    return state;
}

void ensure_screen_orientation_runtime()
{
#if defined(TRAIL_MATE_ESP_BOARD_TAB5)
    ::boards::tab5::heading_runtime::ensure_started();
#endif
}

} // namespace platform::ui::orientation
