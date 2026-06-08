#include "ui/presentation_sources/legacy_mesh_status_source.h"

#include "ui/presentation_sources/runtime_mesh_status_source.h"

namespace ui::presentation_sources
{

bool LegacyMeshStatusSource::buildMeshStatusSnapshot(ui::mesh::MeshStatusSnapshot& out) const
{
    return runtime_mesh_status_source().buildMeshStatusSnapshot(out);
}

LegacyMeshStatusSource& legacy_mesh_status_source()
{
    static LegacyMeshStatusSource source;
    return source;
}

} // namespace ui::presentation_sources
