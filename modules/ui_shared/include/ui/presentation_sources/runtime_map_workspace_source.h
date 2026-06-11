#pragma once

#include "platform/ui/team_ui_snapshot_store.h"
#include "ui_presentation/gps/gps_status_source.h"
#include "ui_presentation/map/map_action_sink.h"
#include "ui_presentation/map/map_presentation_source.h"

namespace ui::presentation_sources
{

struct RuntimeMapWorkspaceState
{
    ui::map::MapViewport last_viewport;
    bool has_viewport = false;
    ui::map::MapLayerState layers;
    ui::map::MeasurementState measurement;
    ui::map::MapToolKind active_tool = ui::map::MapToolKind::Pan;
    bool can_zoom_in = true;
    bool can_zoom_out = true;
    bool can_toggle_layers = true;
};

class RuntimeMapWorkspaceSource final : public ui::map::IMapPresentationSource
{
  public:
    RuntimeMapWorkspaceSource(ui::gps::IGpsStatusSource& gps_source,
                              const RuntimeMapWorkspaceState& state,
                              team::ui::ITeamUiSnapshotStore* team_store = nullptr);

    bool buildMapWorkspaceSnapshot(
        const ui::map::MapWorkspaceRequest& request,
        ui::map::MapWorkspaceSnapshot& out) const override;

  private:
    ui::gps::IGpsStatusSource& gps_source_;
    const RuntimeMapWorkspaceState& state_;
    team::ui::ITeamUiSnapshotStore* team_store_ = nullptr;
};

class RuntimeMapActionSink final : public ui::map::IMapActionSink
{
  public:
    RuntimeMapActionSink(ui::gps::IGpsStatusSource& gps_source,
                         RuntimeMapWorkspaceState& state);

    ui::UiActionResult centerOnSelf() override;
    ui::UiActionResult setViewport(const ui::map::MapViewport& viewport) override;
    ui::UiActionResult setLayer(ui::map::MapLayerKind layer, bool enabled) override;
    ui::UiActionResult setActiveTool(ui::map::MapToolKind tool) override;
    ui::UiActionResult clearMeasurement() override;

  private:
    ui::gps::IGpsStatusSource& gps_source_;
    RuntimeMapWorkspaceState& state_;
};

RuntimeMapWorkspaceState& runtime_map_workspace_state();

} // namespace ui::presentation_sources
