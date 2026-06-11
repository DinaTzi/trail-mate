#include "ui/presentation_sources/legacy_gps_status_source.h"

#include "ui/presentation_sources/runtime_gps_status_source.h"

namespace ui::presentation_sources
{

bool LegacyGpsStatusSource::buildGpsStatusSnapshot(ui::gps::GpsStatusSnapshot& out) const
{
    return runtime_gps_status_source().buildGpsStatusSnapshot(out);
}

LegacyGpsStatusSource& legacy_gps_status_source()
{
    static LegacyGpsStatusSource source;
    return source;
}

} // namespace ui::presentation_sources
