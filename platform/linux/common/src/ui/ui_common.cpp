#include "ui/ui_common.h"

#include <cstdio>
#include <ctime>

#include "platform/ui/time_runtime.h"
#include "ui/widgets/top_bar.h"

namespace
{

bool resolve_display_time(std::tm& out)
{
    if (::platform::ui::time::localtime_now(&out))
    {
        return true;
    }

    const std::time_t now = std::time(nullptr);
    if (now <= 0)
    {
        return false;
    }

    const std::time_t local = ::platform::ui::time::apply_timezone_offset(now);
    const std::tm* tmp = std::gmtime(&local);
    if (!tmp)
    {
        return false;
    }

    out = *tmp;
    return true;
}

void format_top_bar_time(char* out, std::size_t out_len)
{
    if (!out || out_len == 0)
    {
        return;
    }

    std::tm info{};
    if (!resolve_display_time(info) ||
        std::strftime(out, out_len, "%H:%M", &info) == 0)
    {
        std::snprintf(out, out_len, "--:--");
    }
}

} // namespace

void ui_update_top_bar_battery(ui::widgets::TopBar& bar)
{
    char time_buf[16] = "--:--";

    format_top_bar_time(time_buf, sizeof(time_buf));
    ui::widgets::top_bar_set_right_text_ascii(bar, time_buf);
}

int ui_get_timezone_offset_min()
{
    return ::platform::ui::time::timezone_offset_min();
}

void ui_set_timezone_offset_min(int offset_min)
{
    ::platform::ui::time::set_timezone_offset_min(offset_min);
}

time_t ui_apply_timezone_offset(time_t utc_seconds)
{
    return ::platform::ui::time::apply_timezone_offset(utc_seconds);
}

bool ui_take_screenshot_to_sd()
{
    return false;
}
