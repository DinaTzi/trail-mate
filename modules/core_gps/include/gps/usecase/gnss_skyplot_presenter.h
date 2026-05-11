#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "gps/domain/gnss_satellite.h"
#include "gps/domain/gps_state.h"

namespace gps
{

enum class GnssSignalState : std::uint8_t
{
    Good,
    Fair,
    Weak,
    NotUsed,
    InView,
};

struct GnssSkyplotSatellite
{
    std::uint16_t id = 0;
    GnssSystem system = GnssSystem::UNKNOWN;
    std::uint16_t azimuth = 0;
    std::uint8_t elevation = 0;
    std::int8_t snr = -1;
    bool used = false;
    GnssSignalState signal = GnssSignalState::InView;
    int rank = 0;
};

struct GnssSkyplotStatus
{
    bool has_snapshot = false;
    bool has_fix = false;
    GnssFix fix = GnssFix::NOFIX;
    std::uint8_t sats_in_use = 0;
    std::uint8_t sats_in_view = 0;
    float hdop = 0.0F;
};

struct GnssSkyplotView
{
    GnssSkyplotStatus status{};
    std::vector<GnssSkyplotSatellite> satellites{};
};

const char* gnss_system_label(GnssSystem system) noexcept;
const char* gnss_fix_label(GnssFix fix) noexcept;
const char* gnss_signal_label(GnssSignalState state) noexcept;
GnssSignalState gnss_signal_state(int snr, bool used) noexcept;
int gnss_satellite_rank(const GnssSatInfo& sat) noexcept;

GnssSkyplotView build_gnss_skyplot_view(const GnssSatInfo* sats,
                                        std::size_t count,
                                        const GnssStatus& status,
                                        const GpsState& gps_state,
                                        bool has_snapshot,
                                        std::size_t max_satellites);

} // namespace gps
