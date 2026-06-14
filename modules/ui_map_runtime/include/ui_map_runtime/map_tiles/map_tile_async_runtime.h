#pragma once

#include "sys/runtime_async.h"
#include "ui_map_runtime/map_tiles/map_tile_render_queue.h"
#include "ui_map_runtime/map_tiles/map_tile_source.h"

#include <cstddef>
#include <cstdint>

namespace ui
{
namespace map_tiles
{

enum class MapTileInteractionMode : uint8_t
{
    Idle,
    InteractiveDrag,
};

enum class MapTileAsyncEventKind : uint8_t
{
    Ready,
    Failed,
    Cancelled,
    ResourceBusy,
};

struct MapViewportPlan
{
    static constexpr std::size_t kMaxTiles = MapTileRenderQueue::kMaxTiles;

    uint32_t generation = 0;
    MapTileInteractionMode interaction_mode = MapTileInteractionMode::Idle;
    MapTileRef tiles[kMaxTiles]{};
    std::size_t tile_count = 0;
};

struct LoadTileCommand
{
    sys::runtime::RuntimeCommand runtime{};
    MapTileRef tile{};
};

struct MapTileAsyncEvent
{
    MapTileAsyncEventKind kind = MapTileAsyncEventKind::Failed;
    uint32_t command_id = 0;
    uint32_t generation = 0;
    MapTileRef tile{};
    MapTileFormat format = MapTileFormat::Unknown;
    std::size_t payload_size = 0;
    int32_t error = 0;
};

class IMapTileCommandSink
{
  public:
    virtual ~IMapTileCommandSink() = default;

    virtual bool enqueue(const LoadTileCommand& command) = 0;
    virtual std::size_t cancelGeneration(uint32_t generation) = 0;
};

class IMapTileEventSink
{
  public:
    virtual ~IMapTileEventSink() = default;

    virtual bool publish(const MapTileAsyncEvent& event) = 0;
};

class IMapTileWorkerBackend
{
  public:
    virtual ~IMapTileWorkerBackend() = default;

    virtual MapTileLookupResult lookup(const MapTileRef& ref) = 0;
    virtual bool read(const MapTileRef& ref,
                      uint8_t* buffer,
                      std::size_t capacity,
                      std::size_t& out_size,
                      MapTileFormat& out_format) = 0;
};

class MapTileAsyncRuntime
{
  public:
    explicit MapTileAsyncRuntime(IMapTileCommandSink& commands,
                                 sys::runtime::RuntimePolicyStrategy* policy = nullptr);

    uint32_t activeGeneration() const;
    std::size_t requestVisibleTiles(const MapViewportPlan& plan, uint32_t now_ms);
    bool handleEvent(const MapTileAsyncEvent& event, MapTileRenderQueue& render_queue);

  private:
    static sys::runtime::RuntimePriority priorityFor(MapTileInteractionMode mode);
    sys::runtime::RuntimeCommand commandFromIntent(const sys::runtime::RuntimeIntent& intent);

    IMapTileCommandSink& commands_;
    sys::runtime::DefaultRuntimePolicyStrategy default_policy_{};
    sys::runtime::RuntimePolicyStrategy* policy_ = nullptr;
    sys::runtime::RuntimeState state_{};
    uint32_t active_generation_ = 0;
    uint32_t next_command_id_ = 1;
};

class MapTileWorker
{
  public:
    MapTileWorker(IMapTileWorkerBackend& backend,
                  sys::runtime::IBusArbiter& bus,
                  IMapTileEventSink& events,
                  uint8_t* scratch,
                  std::size_t scratch_size,
                  sys::runtime::RuntimePolicyStrategy* policy = nullptr);

    bool execute(const LoadTileCommand& command, uint32_t now_ms);

  private:
    IMapTileWorkerBackend& backend_;
    sys::runtime::IBusArbiter& bus_;
    IMapTileEventSink& events_;
    sys::runtime::DefaultRuntimePolicyStrategy default_policy_{};
    sys::runtime::RuntimePolicyStrategy* policy_ = nullptr;
    uint8_t* scratch_ = nullptr;
    std::size_t scratch_size_ = 0;
};

} // namespace map_tiles
} // namespace ui
