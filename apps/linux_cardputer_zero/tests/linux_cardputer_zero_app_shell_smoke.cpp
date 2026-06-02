#include "linux_cardputer_zero_app_shell.h"

#include "ui_lvgl_ux_packs/ux/ux_menu_provider.h"
#include "ui_presentation/menu/menu_model.h"

#include <cassert>
#include <cstring>

int main()
{
    trailmate::apps::linux_cardputer_zero::LinuxCardputerZeroAppShell shell;
    assert(shell.validate());

    const auto& config = shell.config();
    assert(std::strcmp(config.target_id, "cardputerzero") == 0);
    assert(std::strcmp(config.ux_pack_id, "cardputer_compact") == 0);
    assert(std::strcmp(shell.targetId(), "cardputerzero") == 0);
    assert(std::strcmp(shell.activeUxPackId(), "cardputer_compact") == 0);

    const auto& facts = shell.boardFacts();
    assert(std::strcmp(facts.board_id, "cardputerzero") == 0);
    assert(facts.logical_display_width == 320);
    assert(facts.logical_display_height == 170);
    assert(facts.keyboard_present);
    assert(!facts.touch_present);
    assert(!facts.pointer_present);
    assert(!facts.trackball_present);
    assert(facts.lora_present);
    assert(std::strcmp(facts.lora_state, "hardware_documented_runtime_pending") == 0);
    assert(std::strcmp(facts.lora_chip, "sx1262") == 0);
    assert(std::strcmp(facts.lora_spidev, "/dev/spidev0.1") == 0);
    assert(std::strcmp(facts.lora_gpiochip, "/dev/gpiochip0") == 0);
    assert(facts.lora_spi_speed_hz == 500000);
    assert(facts.lora_reset_gpio == 26);
    assert(facts.lora_irq_gpio == 23);
    assert(facts.lora_busy_gpio == 22);
    assert(facts.lora_power_gpio == -1);
    assert(facts.lora_dio2_as_rf_switch);
    assert(facts.lora_dio3_tcxo_voltage);

    const auto* profile = shell.targetProfile();
    assert(profile != nullptr);
    assert(profile->platform == product_composition::TargetPlatform::Linux);
    assert(profile->renderer == product_composition::TargetRenderer::Ascii);
    assert(std::strcmp(profile->app_shell, "apps/linux_cardputer_zero") == 0);
    assert(profile->has_lora);

    ui::menu::MenuModel menu;
    assert(ui_lvgl_ux::buildMenuForUxPack(shell.activeUxPackId(), menu));
    assert(menu.size() > 0);
    return 0;
}
