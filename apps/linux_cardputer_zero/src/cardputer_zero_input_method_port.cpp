#include "cardputer_zero_input_method_port.h"

#include <cstring>

namespace trailmate
{
namespace apps
{
namespace linux_cardputer_zero
{
namespace
{

constexpr CardputerZeroInputMethodContract kContract = {
    "fcitx5",
    "Fcitx5 -> Linux toolkit frontend -> application",
    "cardputerzero-ui",
    "libcardputerzero-ui",
    "cardputer-zero-ime-panel",
    "cardputer-zero-ime-session",
    "cardputer-zero-ime.service",
    "cardputer-zero/ime-panel.sock",
    "unix-jsonl-addon-to-panel",
    "panel.update",
    "panel.hide",
    "state.update",
    "@im=fcitx",
    "fcitx",
    "Control+space",
    320,
    170,
    3,
    false,
    false,
    false,
    false,
    false,
    false,
    false,
};

} // namespace

const CardputerZeroInputMethodContract& CardputerZeroInputMethodPort::contract() const
{
    return kContract;
}

bool CardputerZeroInputMethodPort::validate() const
{
    const auto& c = contract();
    return std::strcmp(c.framework_name, "fcitx5") == 0 &&
           std::strcmp(c.text_submission_path,
                       "Fcitx5 -> Linux toolkit frontend -> application") == 0 &&
           std::strcmp(c.ui_addon_name, "cardputerzero-ui") == 0 &&
           std::strcmp(c.ui_addon_library, "libcardputerzero-ui") == 0 &&
           std::strcmp(c.panel_executable, "cardputer-zero-ime-panel") == 0 &&
           std::strcmp(c.session_executable, "cardputer-zero-ime-session") == 0 &&
           std::strcmp(c.user_service, "cardputer-zero-ime.service") == 0 &&
           std::strcmp(c.panel_socket_suffix, "cardputer-zero/ime-panel.sock") == 0 &&
           std::strcmp(c.panel_transport, "unix-jsonl-addon-to-panel") == 0 &&
           std::strcmp(c.panel_update_message, "panel.update") == 0 &&
           std::strcmp(c.panel_hide_message, "panel.hide") == 0 &&
           std::strcmp(c.state_update_message, "state.update") == 0 &&
           std::strcmp(c.xmodifiers, "@im=fcitx") == 0 &&
           std::strcmp(c.sdl_im_module, "fcitx") == 0 &&
           std::strcmp(c.trigger_key, "Control+space") == 0 &&
           c.screen_width == 320 &&
           c.screen_height == 170 &&
           c.max_exported_candidates == 3 &&
           !c.app_owns_input_method_engine &&
           !c.app_owns_candidate_display &&
           !c.app_owns_text_commit &&
           !c.panel_socket_commits_text &&
           !c.panel_requests_keyboard_focus &&
           !c.missing_panel_blocks_text_input &&
           !c.uses_embedded_touch_ime;
}

} // namespace linux_cardputer_zero
} // namespace apps
} // namespace trailmate
