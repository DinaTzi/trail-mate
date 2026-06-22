#include "ui/screens/walkie_talkie/walkie_talkie_page_runtime.h"

#if defined(ARDUINO) || defined(ESP_PLATFORM) || defined(TRAIL_MATE_CARDPUTER_ZERO_LINUX)

#include "platform/ui/device_runtime.h"
#include "platform/ui/screen_runtime.h"
#include "platform/ui/walkie_runtime.h"
#include "ui/app_runtime.h"
#include "ui/localization.h"
#include "ui/ui_common.h"
#include "ui/ui_status.h"
#include "ui/runtime/ui_feedback.h"
#include "ui/widgets/top_bar.h"

#include <cstdio>

#if !defined(LV_FONT_MONTSERRAT_18) || !LV_FONT_MONTSERRAT_18
#define lv_font_montserrat_18 lv_font_montserrat_14
#endif
#if !defined(LV_FONT_MONTSERRAT_28) || !LV_FONT_MONTSERRAT_28
#define lv_font_montserrat_28 lv_font_montserrat_18
#endif

using Host = walkie_page::ui::shell::Host;

namespace
{

const Host* s_host = nullptr;
lv_obj_t* s_root = nullptr;
lv_obj_t* s_freq_label = nullptr;
lv_obj_t* s_mod_label = nullptr;
lv_obj_t* s_mode_label = nullptr;
lv_obj_t* s_left_fill = nullptr;
lv_obj_t* s_right_fill = nullptr;
lv_obj_t* s_monitor_row = nullptr;
lv_obj_t* s_monitor_switch = nullptr;
lv_obj_t* s_monitor_label = nullptr;
lv_obj_t* s_control_panel = nullptr;
lv_obj_t* s_volume_bar = nullptr;
lv_obj_t* s_volume_label = nullptr;
lv_timer_t* s_timer = nullptr;
ui::widgets::TopBar s_top_bar;
bool s_started = false;

constexpr lv_coord_t kVuWidth = 12;
constexpr lv_coord_t kVuHeight = 104;
constexpr lv_coord_t kControlPanelWidth = 210;
constexpr lv_coord_t kVolumeBarHeight = 8;
constexpr lv_coord_t kMonitorRowHeight = 32;
constexpr lv_coord_t kMonitorSwitchWidth = 46;
constexpr lv_coord_t kMonitorSwitchHeight = 24;

void set_monitor_visual_state(bool enabled)
{
    if (s_monitor_row)
    {
        if (enabled)
        {
            lv_obj_add_state(s_monitor_row, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(s_monitor_row, LV_STATE_CHECKED);
        }
    }
    if (s_monitor_switch)
    {
        if (enabled)
        {
            lv_obj_add_state(s_monitor_switch, LV_STATE_CHECKED);
        }
        else
        {
            lv_obj_clear_state(s_monitor_switch, LV_STATE_CHECKED);
        }
    }
    if (s_monitor_label)
    {
        ::ui::i18n::set_label_text(s_monitor_label, enabled ? "Monitor On" : "Monitor Off");
    }
}

void apply_monitor_enabled_from_ui(bool enabled)
{
    if (!platform::ui::walkie::set_monitor_enabled(enabled))
    {
        const bool actual = platform::ui::walkie::get_status().monitor_enabled;
        set_monitor_visual_state(actual);
        const char* detail = platform::ui::walkie::last_error();
        ::ui::feedback::show_notice((detail && detail[0] != '\0') ? detail : "Monitor unavailable",
                                    2500);
        return;
    }

    set_monitor_visual_state(platform::ui::walkie::get_status().monitor_enabled);
    ::ui::status::force_update();
}

void request_exit()
{
    if (s_host)
    {
        ::ui::page::request_exit(s_host);
        return;
    }
    ui_request_exit_to_menu();
}

void on_back(void*)
{
    request_exit();
}

void root_key_event_cb(lv_event_t* e)
{
    uint32_t key = lv_event_get_key(e);
    if (key != LV_KEY_BACKSPACE)
    {
        return;
    }
    on_back(nullptr);
}

void monitor_row_event_cb(lv_event_t* e)
{
    if (!s_monitor_row)
    {
        return;
    }

    const lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED)
    {
        apply_monitor_enabled_from_ui(!lv_obj_has_state(s_monitor_row, LV_STATE_CHECKED));
        return;
    }
    if (code != LV_EVENT_KEY)
    {
        return;
    }

    const uint32_t key = lv_event_get_key(e);
    if (key == LV_KEY_ENTER || key == ' ')
    {
        apply_monitor_enabled_from_ui(!lv_obj_has_state(s_monitor_row, LV_STATE_CHECKED));
        lv_event_stop_processing(e);
        return;
    }
    if (key == LV_KEY_BACKSPACE)
    {
        on_back(nullptr);
        lv_event_stop_processing(e);
    }
}

void style_monitor_row(lv_obj_t* row)
{
    if (!row)
    {
        return;
    }
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFF7E9), LV_PART_MAIN);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFE5B5), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(row, lv_color_hex(0xD9B06A), LV_PART_MAIN);
    lv_obj_set_style_radius(row, 8, LV_PART_MAIN);
    lv_obj_set_style_outline_width(row, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(row, lv_color_hex(0xC98118), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(row, 2, LV_STATE_FOCUSED);
}

void style_control_panel(lv_obj_t* panel)
{
    if (!panel)
    {
        return;
    }
    lv_obj_set_style_bg_color(panel, lv_color_hex(0xFFF8EA), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(panel, LV_OPA_70, LV_PART_MAIN);
    lv_obj_set_style_border_width(panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xE3C27F), LV_PART_MAIN);
    lv_obj_set_style_radius(panel, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_left(panel, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_right(panel, 6, LV_PART_MAIN);
    lv_obj_set_style_pad_top(panel, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(panel, 5, LV_PART_MAIN);
    lv_obj_set_style_pad_row(panel, 4, LV_PART_MAIN);
    lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(panel,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_CLICKABLE);
}

void style_readonly_volume_bar(lv_obj_t* bar)
{
    if (!bar)
    {
        return;
    }
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0xFFF0D3), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x5BAF4A), LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    lv_obj_set_style_outline_width(bar, 0, LV_STATE_FOCUSED);
}

void update_vu(lv_obj_t* fill, uint8_t level)
{
    if (!fill)
    {
        return;
    }
    lv_obj_t* parent = lv_obj_get_parent(fill);
    if (!parent)
    {
        return;
    }
    lv_coord_t height = lv_obj_get_height(parent);
    lv_coord_t fill_h = static_cast<lv_coord_t>((level * height) / 100);
    if (fill_h < 0)
    {
        fill_h = 0;
    }
    if (fill_h > height)
    {
        fill_h = height;
    }
    lv_obj_set_height(fill, fill_h);
}

void refresh_cb(lv_timer_t*)
{
    platform::ui::walkie::Status st = platform::ui::walkie::get_status();
    ui_update_top_bar_battery(s_top_bar);
    if (s_mode_label)
    {
        ::ui::i18n::set_label_text(s_mode_label, st.tx ? "TALK" : "LISTEN");
    }
    if (s_monitor_switch)
    {
        set_monitor_visual_state(st.monitor_enabled);
    }
    update_vu(s_left_fill, st.tx ? st.tx_level : st.rx_level);
    update_vu(s_right_fill, st.tx ? st.tx_level : st.rx_level);

    if (s_volume_bar)
    {
        int vol = platform::ui::walkie::volume();
        lv_bar_set_value(s_volume_bar, vol, LV_ANIM_OFF);
    }
    if (s_volume_label)
    {
        char buf[24];
        int vol = platform::ui::walkie::volume();
        snprintf(buf, sizeof(buf), "%s", ::ui::i18n::format("VOL %d", vol).c_str());
        ::ui::i18n::set_label_text_raw(s_volume_label, buf);
    }
}

void set_freq_text(float freq_mhz)
{
    if (!s_freq_label)
    {
        return;
    }
    char buf[24];
    if (freq_mhz <= 0.0f)
    {
        snprintf(buf, sizeof(buf), "--.- MHz");
    }
    else
    {
        snprintf(buf, sizeof(buf), "%.3f MHz", static_cast<double>(freq_mhz));
    }
    ::ui::i18n::set_label_text_raw(s_freq_label, buf);
}

void set_error_text(const char* message)
{
    if (s_freq_label)
    {
        ::ui::i18n::set_label_text(s_freq_label, "Walkie Talkie");
    }
    if (s_mod_label)
    {
        ::ui::i18n::set_label_text(s_mod_label, message ? message : "Walkie not available");
    }
    if (s_mode_label)
    {
        ::ui::i18n::set_label_text(s_mode_label, "Press Back");
    }
    update_vu(s_left_fill, 0);
    update_vu(s_right_fill, 0);
}

} // namespace

namespace walkie_page::ui::runtime
{

bool is_available()
{
    return platform::ui::walkie::is_supported();
}

void enter(const shell::Host* host, lv_obj_t* parent)
{
    s_host = host;

    if (platform::ui::device::power_tier() >= 1)
    {
        ::ui::feedback::show_notice("Low battery - audio disabled", 3000);
    }

    s_started = false;

    lv_group_t* prev_group = lv_group_get_default();
    set_default_group(nullptr);

    if (s_root)
    {
        lv_obj_del(s_root);
        s_root = nullptr;
    }

    s_root = lv_obj_create(parent);
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_flex_flow(s_root, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0xFFF3DF), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_root, 0, 0);
    lv_obj_set_style_pad_all(s_root, 0, 0);
    lv_obj_set_style_pad_row(s_root, 0, 0);
    lv_obj_clear_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_root, root_key_event_cb, LV_EVENT_KEY, nullptr);

    ::ui::widgets::top_bar_init(s_top_bar, s_root);
    ::ui::widgets::top_bar_set_title(s_top_bar, ::ui::i18n::tr("Walkie Talkie"));
    ::ui::widgets::top_bar_set_back_callback(s_top_bar, on_back, nullptr);
    if (s_top_bar.back_btn)
    {
        lv_obj_add_event_cb(s_top_bar.back_btn, root_key_event_cb, LV_EVENT_KEY, nullptr);
    }
    ui_update_top_bar_battery(s_top_bar);
    if (app_g && s_top_bar.back_btn)
    {
        lv_group_remove_all_objs(app_g);
        lv_group_add_obj(app_g, s_top_bar.back_btn);
        lv_group_focus_obj(s_top_bar.back_btn);
        set_default_group(app_g);
        lv_group_set_editing(app_g, false);
    }
    else
    {
        set_default_group(prev_group);
    }

    lv_obj_t* content = lv_obj_create(s_root);
    lv_obj_set_size(content, LV_PCT(100), 0);
    lv_obj_set_flex_grow(content, 1);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 0, 0);
    lv_obj_clear_flag(content, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* stack = lv_obj_create(content);
    lv_obj_set_size(stack, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_center(stack);
    lv_obj_set_style_bg_opa(stack, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(stack, 0, 0);
    lv_obj_set_style_pad_all(stack, 0, 0);
    lv_obj_set_style_pad_row(stack, 4, 0);
    lv_obj_set_flex_flow(stack, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(stack, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(stack, LV_OBJ_FLAG_SCROLLABLE);

    s_freq_label = lv_label_create(stack);
    ::ui::i18n::set_label_text_raw(s_freq_label, "--.- MHz");
    lv_obj_set_style_text_font(s_freq_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_align(s_freq_label, LV_TEXT_ALIGN_CENTER, 0);

    s_mod_label = lv_label_create(stack);
    ::ui::i18n::set_label_text(s_mod_label, "FSK Voice");
    lv_obj_set_style_text_font(s_mod_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(s_mod_label, LV_TEXT_ALIGN_CENTER, 0);

    s_mode_label = lv_label_create(stack);
    ::ui::i18n::set_label_text(s_mode_label, "LISTEN");
    lv_obj_set_style_text_font(s_mode_label, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_align(s_mode_label, LV_TEXT_ALIGN_CENTER, 0);

    s_control_panel = lv_obj_create(stack);
    lv_obj_set_size(s_control_panel, kControlPanelWidth, LV_SIZE_CONTENT);
    style_control_panel(s_control_panel);

    s_monitor_row = lv_btn_create(s_control_panel);
    lv_obj_set_size(s_monitor_row, LV_PCT(100), kMonitorRowHeight);
    lv_obj_set_style_pad_left(s_monitor_row, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_right(s_monitor_row, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_top(s_monitor_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(s_monitor_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_column(s_monitor_row, 8, LV_PART_MAIN);
    lv_obj_set_flex_flow(s_monitor_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(s_monitor_row,
                          LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(s_monitor_row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_monitor_row, monitor_row_event_cb, LV_EVENT_CLICKED, nullptr);
    lv_obj_add_event_cb(s_monitor_row, monitor_row_event_cb, LV_EVENT_KEY, nullptr);
    style_monitor_row(s_monitor_row);

    s_monitor_label = lv_label_create(s_monitor_row);
    ::ui::i18n::set_label_text(s_monitor_label, "Monitor Off");
    lv_obj_set_style_text_font(s_monitor_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(s_monitor_label, lv_color_hex(0x3A2A1A), 0);
    lv_obj_set_flex_grow(s_monitor_label, 1);
    lv_label_set_long_mode(s_monitor_label, LV_LABEL_LONG_CLIP);

    s_monitor_switch = lv_switch_create(s_monitor_row);
    lv_obj_set_size(s_monitor_switch, kMonitorSwitchWidth, kMonitorSwitchHeight);
    lv_obj_clear_flag(s_monitor_switch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(s_monitor_switch, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_set_style_outline_width(s_monitor_switch, 2, LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(s_monitor_switch, lv_color_hex(0xC98118), LV_STATE_FOCUSED);
    lv_obj_set_style_outline_pad(s_monitor_switch, 2, LV_STATE_FOCUSED);
    lv_obj_add_event_cb(s_monitor_switch, root_key_event_cb, LV_EVENT_KEY, nullptr);

    lv_obj_t* volume_row = lv_obj_create(s_control_panel);
    lv_obj_set_size(volume_row, LV_PCT(100), 16);
    lv_obj_set_style_bg_opa(volume_row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(volume_row, 0, 0);
    lv_obj_set_style_pad_all(volume_row, 0, 0);
    lv_obj_set_style_pad_column(volume_row, 6, 0);
    lv_obj_set_flex_flow(volume_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(volume_row,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(volume_row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(volume_row, LV_OBJ_FLAG_CLICK_FOCUSABLE);
    lv_obj_clear_flag(volume_row, LV_OBJ_FLAG_SCROLLABLE);

    s_volume_label = lv_label_create(volume_row);
    ::ui::i18n::set_label_text(s_volume_label, "VOL 80");
    lv_obj_set_width(s_volume_label, 56);
    lv_obj_set_style_text_font(s_volume_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(s_volume_label, LV_TEXT_ALIGN_LEFT, 0);

    s_volume_bar = lv_bar_create(volume_row);
    lv_obj_set_size(s_volume_bar, 0, kVolumeBarHeight);
    lv_obj_set_flex_grow(s_volume_bar, 1);
    lv_bar_set_range(s_volume_bar, 0, 100);
    lv_bar_set_value(s_volume_bar, platform::ui::walkie::volume(), LV_ANIM_OFF);
    style_readonly_volume_bar(s_volume_bar);

    if (app_g && s_monitor_row)
    {
        lv_group_add_obj(app_g, s_monitor_row);
        lv_group_set_editing(app_g, false);
    }

    lv_obj_t* vu_left = lv_obj_create(content);
    lv_obj_set_size(vu_left, kVuWidth, kVuHeight);
    lv_obj_align(vu_left, LV_ALIGN_LEFT_MID, 16, 0);
    lv_obj_set_style_bg_opa(vu_left, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vu_left, 1, 0);
    lv_obj_set_style_border_color(vu_left, lv_color_hex(0xD9B06A), 0);
    lv_obj_set_style_pad_all(vu_left, 0, 0);
    lv_obj_clear_flag(vu_left, LV_OBJ_FLAG_SCROLLABLE);

    s_left_fill = lv_obj_create(vu_left);
    lv_obj_set_width(s_left_fill, LV_PCT(100));
    lv_obj_set_height(s_left_fill, 0);
    lv_obj_align(s_left_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_left_fill, lv_color_hex(0x5BAF4A), 0);
    lv_obj_set_style_border_width(s_left_fill, 0, 0);
    lv_obj_clear_flag(s_left_fill, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* vu_right = lv_obj_create(content);
    lv_obj_set_size(vu_right, kVuWidth, kVuHeight);
    lv_obj_align(vu_right, LV_ALIGN_RIGHT_MID, -16, 0);
    lv_obj_set_style_bg_opa(vu_right, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(vu_right, 1, 0);
    lv_obj_set_style_border_color(vu_right, lv_color_hex(0xD9B06A), 0);
    lv_obj_set_style_pad_all(vu_right, 0, 0);
    lv_obj_clear_flag(vu_right, LV_OBJ_FLAG_SCROLLABLE);

    s_right_fill = lv_obj_create(vu_right);
    lv_obj_set_width(s_right_fill, LV_PCT(100));
    lv_obj_set_height(s_right_fill, 0);
    lv_obj_align(s_right_fill, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(s_right_fill, lv_color_hex(0x5BAF4A), 0);
    lv_obj_set_style_border_width(s_right_fill, 0, 0);
    lv_obj_clear_flag(s_right_fill, LV_OBJ_FLAG_SCROLLABLE);

    platform::ui::walkie::Status st = platform::ui::walkie::get_status();
    if (st.freq_mhz > 0.0f)
    {
        set_freq_text(st.freq_mhz);
    }

    const char* error = nullptr;

    if (!error)
    {
        s_started = platform::ui::walkie::start();
        if (!s_started)
        {
            const char* detail = platform::ui::walkie::last_error();
            if (detail && detail[0] != '\0')
            {
                error = detail;
            }
            else
            {
                error = "Walkie start failed";
            }
        }
    }

    if (!s_started)
    {
        set_error_text(error);
        return;
    }

    platform::ui::screen::disable_sleep();

    st = platform::ui::walkie::get_status();
    set_freq_text(st.freq_mhz);
    if (s_monitor_switch)
    {
        set_monitor_visual_state(st.monitor_enabled);
    }

    if (!s_timer)
    {
        s_timer = lv_timer_create(refresh_cb, 120, nullptr);
    }
    refresh_cb(nullptr);
}

void exit(lv_obj_t* parent)
{
    (void)parent;
    const bool keep_monitoring = platform::ui::walkie::monitor_enabled();
    platform::ui::walkie::set_ptt(false);
    if (s_timer)
    {
        lv_timer_del(s_timer);
        s_timer = nullptr;
    }
    if (s_root)
    {
        lv_obj_del(s_root);
        s_root = nullptr;
    }
    s_freq_label = nullptr;
    s_mod_label = nullptr;
    s_mode_label = nullptr;
    s_left_fill = nullptr;
    s_right_fill = nullptr;
    s_monitor_row = nullptr;
    s_monitor_switch = nullptr;
    s_monitor_label = nullptr;
    s_control_panel = nullptr;
    s_volume_bar = nullptr;
    s_volume_label = nullptr;
    s_top_bar = {};
    s_started = false;

    if (!keep_monitoring)
    {
        platform::ui::walkie::stop();
    }
    ::ui::status::force_update();
    platform::ui::screen::enable_sleep();
    s_host = nullptr;
}

} // namespace walkie_page::ui::runtime

void ui_walkie_talkie_enter(lv_obj_t* parent)
{
    walkie_page::ui::shell::enter(nullptr, parent);
}

void ui_walkie_talkie_exit(lv_obj_t* parent)
{
    walkie_page::ui::shell::exit(nullptr, parent);
}

#else

namespace walkie_page::ui::runtime
{

bool is_available()
{
    return false;
}

void enter(const shell::Host* host, lv_obj_t* parent)
{
    (void)host;
    (void)parent;
}

void exit(lv_obj_t* parent)
{
    (void)parent;
}

} // namespace walkie_page::ui::runtime

#endif
