#include "esp32_lvgl_runtime_config.h"
#include "esp32_lvgl_startup_runtime.h"

extern "C" void app_main(void)
{
    const auto& config = trailmate::apps::esp32_lvgl::esp32LvglRuntimeConfig();
    trailmate::apps::esp32_lvgl::runEsp32LvglStartupRuntime(config);
}
