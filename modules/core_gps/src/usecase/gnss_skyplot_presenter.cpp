#include "gps/usecase/gnss_skyplot_presenter.h"

#include <algorithm>

namespace gps
{
namespace
{

std::uint8_t clamp_sat_count(std::size_t count) noexcept
{
    return static_cast<std::uint8_t>(
        std::min<std::size_t>(count, static_cast<std::size_t>(255U)));
}

} // namespace

const char* gnss_system_label(GnssSystem system) noexcept
{
    switch (system)
    {
    case GnssSystem::GPS:
        return "GPS";
    case GnssSystem::GLN:
        return "GLN";
    case GnssSystem::GAL:
        return "GAL";
    case GnssSystem::BD:
        return "BD";
    case GnssSystem::UNKNOWN:
    default:
        return "UNK";
    }
}

const char* gnss_fix_label(GnssFix fix) noexcept
{
    switch (fix)
    {
    case GnssFix::FIX3D:
        return "3D FIX";
    case GnssFix::FIX2D:
        return "2D FIX";
    case GnssFix::NOFIX:
    default:
        return "NO FIX";
    }
}

const char* gnss_signal_label(GnssSignalState state) noexcept
{
    switch (state)
    {
    case GnssSignalState::Good:
        return "good";
    case GnssSignalState::Fair:
        return "fair";
    case GnssSignalState::Weak:
        return "weak";
    case GnssSignalState::NotUsed:
        return "not-used";
    case GnssSignalState::InView:
    default:
        return "in-view";
    }
}

GnssSignalState gnss_signal_state(int snr, bool used) noexcept
{
    if (snr < 0)
    {
        return used ? GnssSignalState::NotUsed : GnssSignalState::InView;
    }
    if (!used)
    {
        return GnssSignalState::NotUsed;
    }
    if (snr >= 35)
    {
        return GnssSignalState::Good;
    }
    if (snr >= 25)
    {
        return GnssSignalState::Fair;
    }
    return GnssSignalState::Weak;
}

int gnss_satellite_rank(const GnssSatInfo& sat) noexcept
{
    const int used_bonus = sat.used ? 1000 : 0;
    const int snr_score = std::max(0, static_cast<int>(sat.snr)) * 10;
    return used_bonus + snr_score + static_cast<int>(sat.elevation);
}

GnssSkyplotView build_gnss_skyplot_view(const GnssSatInfo* sats,
                                        std::size_t count,
                                        const GnssStatus& status,
                                        const GpsState& gps_state,
                                        bool has_snapshot,
                                        std::size_t max_satellites)
{
    GnssSkyplotView out{};
    out.status.has_snapshot = has_snapshot;
    out.status.fix = has_snapshot ? status.fix : GnssFix::NOFIX;
    out.status.has_fix =
        gps_state.valid || (has_snapshot && status.fix != GnssFix::NOFIX);
    out.status.hdop = has_snapshot ? status.hdop : 0.0F;
    out.status.sats_in_use =
        has_snapshot ? status.sats_in_use : static_cast<std::uint8_t>(0);
    out.status.sats_in_view =
        has_snapshot ? (status.sats_in_view > 0 ? status.sats_in_view
                                                : clamp_sat_count(count))
                     : static_cast<std::uint8_t>(0);

    if (!has_snapshot || sats == nullptr || count == 0 || max_satellites == 0)
    {
        return out;
    }

    const std::size_t safe_count =
        std::min<std::size_t>(count, static_cast<std::size_t>(kMaxGnssSats));
    out.satellites.reserve(std::min(safe_count, max_satellites));
    for (std::size_t index = 0; index < safe_count; ++index)
    {
        const auto& sat = sats[index];
        GnssSkyplotSatellite item{};
        item.id = sat.id;
        item.system = sat.sys;
        item.azimuth = sat.azimuth;
        item.elevation = sat.elevation;
        item.snr = sat.snr;
        item.used = sat.used;
        item.signal = gnss_signal_state(sat.snr, sat.used);
        item.rank = gnss_satellite_rank(sat);
        out.satellites.push_back(item);
    }

    std::sort(out.satellites.begin(),
              out.satellites.end(),
              [](const GnssSkyplotSatellite& lhs,
                 const GnssSkyplotSatellite& rhs)
              {
                  if (lhs.rank != rhs.rank)
                  {
                      return lhs.rank > rhs.rank;
                  }
                  return lhs.id < rhs.id;
              });
    if (out.satellites.size() > max_satellites)
    {
        out.satellites.resize(max_satellites);
    }
    return out;
}

} // namespace gps
