#include "ui/widgets/system_notification.h"

#include <string>

namespace
{

struct GpsTrackCompatState
{
    std::string tracker_file;
    bool tracker_overlay_active = false;
};

GpsTrackCompatState g_track_compat_state{};

} // namespace

bool gps_tracker_load_file(const char* path, bool show_toast)
{
    (void)show_toast;
    g_track_compat_state.tracker_file = path ? path : "";
    g_track_compat_state.tracker_overlay_active = !g_track_compat_state.tracker_file.empty();
    return g_track_compat_state.tracker_overlay_active;
}

void gps_tracker_open_modal()
{
}

void gps_tracker_draw_event(lv_event_t* e)
{
    (void)e;
}

void gps_tracker_cleanup()
{
    g_track_compat_state.tracker_overlay_active = false;
    g_track_compat_state.tracker_file.clear();
}

void show_toast(const char* message, uint32_t duration_ms)
{
    ::ui::SystemNotification::show(message ? message : "", duration_ms);
}
