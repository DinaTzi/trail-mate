#pragma once

#include <cstddef>
#include <cstdint>

namespace chat::runtime
{

struct MeshtasticPositionInput
{
    bool valid = false;
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    bool has_altitude = false;
    double altitude_m = 0.0;
    bool has_speed = false;
    double speed_mps = 0.0;
    bool has_course = false;
    double course_deg = 0.0;
    uint32_t satellites = 0;
    uint32_t timestamp_s = 0;
};

class MeshtasticPositionCore final
{
  public:
    static bool buildPositionPayload(const MeshtasticPositionInput& input,
                                     uint8_t* out_buf,
                                     size_t* out_len);
};

} // namespace chat::runtime
