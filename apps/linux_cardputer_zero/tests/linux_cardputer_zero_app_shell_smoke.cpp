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

    const auto* profile = shell.targetProfile();
    assert(profile != nullptr);
    assert(profile->platform == product_composition::TargetPlatform::Linux);
    assert(profile->renderer == product_composition::TargetRenderer::Ascii);
    assert(std::strcmp(profile->app_shell, "apps/linux_cardputer_zero") == 0);

    ui::menu::MenuModel menu;
    assert(ui_lvgl_ux::buildMenuForUxPack(shell.activeUxPackId(), menu));
    assert(menu.size() > 0);
    return 0;
}
