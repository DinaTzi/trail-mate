#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "platform/linux/map_tile_cache.h"

namespace platform::linux_runtime
{

struct MapContourGenerationResult
{
    std::size_t requested_tiles = 0;
    std::size_t cached_tiles = 0;
    std::size_t generated_tiles = 0;
    std::size_t failed_tiles = 0;
    std::string message{};
};

class MapContourTileGenerator final
{
  public:
    MapContourTileGenerator();

    [[nodiscard]] MapContourGenerationResult ensure_tiles(
        const std::vector<MapContourTileId>& tiles,
        const std::string& earthdata_token) const;

  private:
    MapContourTileStore store_{};
};

} // namespace platform::linux_runtime
