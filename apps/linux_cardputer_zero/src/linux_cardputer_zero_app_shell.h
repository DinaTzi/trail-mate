#pragma once

#include "boards/cardputerzero/board_facts.h"
#include "cardputer_zero_input_method_port.h"
#include "cardputer_zero_notification_port.h"

#include "product_composition/target_profile.h"

namespace trailmate
{
namespace apps
{
namespace linux_cardputer_zero
{

struct LinuxCardputerZeroAppShellConfig
{
    const char* target_id = "cardputerzero";
    const char* ux_pack_id = "cardputer_compact";
};

class LinuxCardputerZeroAppShell
{
  public:
    LinuxCardputerZeroAppShell() = default;
    explicit LinuxCardputerZeroAppShell(LinuxCardputerZeroAppShellConfig config);

    const LinuxCardputerZeroAppShellConfig& config() const;
    const char* targetId() const;
    const product_composition::TargetProfile* targetProfile() const;
    const char* activeUxPackId() const;
    const boards::cardputerzero::CardputerZeroBoardFacts& boardFacts() const;
    const CardputerZeroNotificationPort& notificationPort() const;
    const CardputerZeroInputMethodPort& inputMethodPort() const;
    bool validate() const;

  private:
    LinuxCardputerZeroAppShellConfig config_{};
    CardputerZeroNotificationPort notification_port_{};
    CardputerZeroInputMethodPort input_method_port_{};
};

} // namespace linux_cardputer_zero
} // namespace apps
} // namespace trailmate
