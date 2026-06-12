#include "ui/mono/screens/screensaver_layout.h"

#include "ui/mono/screens/screen_128x64/screensaver_layout.h"
#include "ui/mono/screens/screen_192x176/screensaver_layout.h"

namespace ui::mono
{

bool usesLargeScreensaverLayout(const MonoDisplay& display)
{
    return screen_192x176::matches(display);
}

void renderScreensaverLayout(MonoDisplay& display, const TextRenderer& text_renderer,
                             const TextRenderer& accent_text_renderer,
                             const ScreensaverLayoutModel& model)
{
    if (screen_192x176::matches(display))
    {
        screen_192x176::renderScreensaverLayout(display, text_renderer, accent_text_renderer, model);
        return;
    }

    screen_128x64::renderScreensaverLayout(display, text_renderer, model);
}

} // namespace ui::mono
