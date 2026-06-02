#pragma once

namespace trailmate
{
namespace apps
{
namespace linux_cardputer_zero
{

struct CardputerZeroInputMethodContract
{
    const char* framework_name;
    const char* text_submission_path;
    const char* ui_addon_name;
    const char* ui_addon_library;
    const char* panel_executable;
    const char* session_executable;
    const char* user_service;
    const char* panel_socket_suffix;
    const char* panel_transport;
    const char* panel_update_message;
    const char* panel_hide_message;
    const char* state_update_message;
    const char* xmodifiers;
    const char* sdl_im_module;
    const char* trigger_key;
    int screen_width;
    int screen_height;
    int max_exported_candidates;
    bool app_owns_input_method_engine;
    bool app_owns_candidate_display;
    bool app_owns_text_commit;
    bool panel_socket_commits_text;
    bool panel_requests_keyboard_focus;
    bool missing_panel_blocks_text_input;
    bool uses_embedded_touch_ime;
};

class CardputerZeroInputMethodPort
{
  public:
    const CardputerZeroInputMethodContract& contract() const;
    bool validate() const;
};

} // namespace linux_cardputer_zero
} // namespace apps
} // namespace trailmate
