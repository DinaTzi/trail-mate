#include "linux_cardputer_zero_app_shell.h"

#include "product_composition/target_ux_binding.h"
#include "ui_lvgl_ux_packs/ux/ux_pack_registry.h"

#include <cstring>

namespace trailmate
{
namespace apps
{
namespace linux_cardputer_zero
{

LinuxCardputerZeroAppShell::LinuxCardputerZeroAppShell(
    LinuxCardputerZeroAppShellConfig config)
    : config_(config)
{
}

const LinuxCardputerZeroAppShellConfig&
LinuxCardputerZeroAppShell::config() const
{
    return config_;
}

const char* LinuxCardputerZeroAppShell::targetId() const
{
    return config_.target_id;
}

const product_composition::TargetProfile*
LinuxCardputerZeroAppShell::targetProfile() const
{
    return product_composition::findTargetProfile(targetId());
}

const char* LinuxCardputerZeroAppShell::activeUxPackId() const
{
    const auto* binding = product_composition::findTargetUxBinding(targetId());
    return binding != nullptr ? binding->active_ux_pack_id : nullptr;
}

const boards::cardputerzero::CardputerZeroBoardFacts&
LinuxCardputerZeroAppShell::boardFacts() const
{
    return boards::cardputerzero::kBoardFacts;
}

bool LinuxCardputerZeroAppShell::validate() const
{
    const auto& facts = boardFacts();
    const auto* profile = targetProfile();
    return config_.target_id != nullptr &&
           profile != nullptr &&
           std::strcmp(profile->target_id, "cardputerzero") == 0 &&
           std::strcmp(profile->board_id, facts.board_id) == 0 &&
           std::strcmp(profile->app_shell, "apps/linux_cardputer_zero") == 0 &&
           profile->platform == product_composition::TargetPlatform::Linux &&
           profile->renderer == product_composition::TargetRenderer::Ascii &&
           profile->has_display == facts.display_present &&
           profile->has_keyboard == facts.keyboard_present &&
           profile->has_touch == facts.touch_present &&
           profile->has_trackball == facts.trackball_present &&
           profile->has_lora == facts.lora_present &&
           facts.logical_display_width == 320 &&
           facts.logical_display_height == 170 &&
           std::strcmp(facts.lora_chip, "sx1262") == 0 &&
           std::strcmp(facts.lora_spidev, "/dev/spidev0.1") == 0 &&
           facts.lora_spi_speed_hz == 500000 &&
           facts.lora_reset_gpio == 26 &&
           facts.lora_irq_gpio == 23 &&
           facts.lora_busy_gpio == 22 &&
           facts.lora_dio2_as_rf_switch &&
           facts.lora_dio3_tcxo_voltage &&
           product_composition::findTargetUxBinding(targetId()) != nullptr &&
           config_.ux_pack_id != nullptr &&
           std::strcmp(config_.ux_pack_id, activeUxPackId()) == 0 &&
           ui_lvgl_ux::findUxPackById(activeUxPackId()) != nullptr;
}

} // namespace linux_cardputer_zero
} // namespace apps
} // namespace trailmate
