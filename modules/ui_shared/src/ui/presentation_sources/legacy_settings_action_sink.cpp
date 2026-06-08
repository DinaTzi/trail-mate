#include "ui/presentation_sources/legacy_settings_action_sink.h"

#include "ui/presentation_sources/runtime_settings_source.h"

namespace ui::presentation_sources
{

ui::UiActionResult LegacySettingsActionSink::applySetting(
    const ui::settings::SettingsPatchView& patch)
{
    return runtime_settings_action_sink().applySetting(patch);
}

LegacySettingsActionSink& legacy_settings_action_sink()
{
    static LegacySettingsActionSink sink;
    return sink;
}

} // namespace ui::presentation_sources
