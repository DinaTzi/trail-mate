#include "ui/presentation_sources/legacy_settings_source.h"

#include "ui/presentation_sources/runtime_settings_source.h"

namespace ui::presentation_sources
{

bool LegacySettingsSource::buildSettingsSnapshot(ui::settings::SettingsSnapshot& out) const
{
    return runtime_settings_source().buildSettingsSnapshot(out);
}

LegacySettingsSource& legacy_settings_source()
{
    static LegacySettingsSource source;
    return source;
}

} // namespace ui::presentation_sources
