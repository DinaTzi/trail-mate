#pragma once

#include "ui_presentation/mesh/mesh_status_source.h"

namespace ui::presentation_sources
{

class RuntimeMeshStatusSource final : public ui::mesh::IMeshStatusSource
{
  public:
    bool buildMeshStatusSnapshot(ui::mesh::MeshStatusSnapshot& out) const override;
};

RuntimeMeshStatusSource& runtime_mesh_status_source();

} // namespace ui::presentation_sources
