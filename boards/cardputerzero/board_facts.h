#pragma once

namespace boards::cardputerzero
{

struct CardputerZeroBoardFacts
{
    const char* board_id = "cardputerzero";
    const char* platform_family = "linux";
    const char* current_route_evidence = "docs/targets/linux_targets.md";

    bool display_present = true;
    int logical_display_width = 320;
    int logical_display_height = 170;
    const char* logical_display_source = "platform/linux/common/src/core/display_profile.h";
    bool keyboard_present = true;
    const char* keyboard_mapping_state = "needs_real_device_sampling";
    bool pointer_present = false;
    bool touch_present = false;
    bool trackball_present = false;
    bool lora_present = true;
    const char* lora_state = "hardware_documented_runtime_pending";
    const char* lora_chip = "sx1262";
    const char* lora_spidev = "/dev/spidev0.1";
    const char* lora_gpiochip = "/dev/gpiochip0";
    int lora_spi_speed_hz = 500000;
    int lora_reset_gpio = 26;
    int lora_irq_gpio = 23;
    int lora_busy_gpio = 22;
    int lora_power_gpio = -1;
    bool lora_dio2_as_rf_switch = true;
    bool lora_dio3_tcxo_voltage = true;
    const char* gps_state = "not_established_in_final_owner";
    bool posix_filesystem_present = true;
};

inline constexpr CardputerZeroBoardFacts kBoardFacts{};

} // namespace boards::cardputerzero
