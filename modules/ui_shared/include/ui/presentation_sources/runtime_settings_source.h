#pragma once

#include "ui_presentation/settings/settings_action_sink.h"
#include "ui_presentation/settings/settings_source.h"

namespace ui::presentation_sources
{

class RuntimeSettingsSource final : public ui::settings::ISettingsSource
{
  public:
    bool buildSettingsSnapshot(ui::settings::SettingsSnapshot& out) const override;
};

class RuntimeSettingsActionSink final : public ui::settings::ISettingsActionSink
{
  public:
    ui::UiActionResult applySetting(
        const ui::settings::SettingsPatchView& patch) override;
};

RuntimeSettingsSource& runtime_settings_source();
RuntimeSettingsActionSink& runtime_settings_action_sink();

} // namespace ui::presentation_sources
