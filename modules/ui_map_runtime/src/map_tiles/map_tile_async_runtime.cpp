#include "ui_map_runtime/map_tiles/map_tile_async_runtime.h"

namespace ui
{
namespace map_tiles
{

MapTileAsyncRuntime::MapTileAsyncRuntime(IMapTileCommandSink& commands)
    : commands_(commands)
{
}

uint32_t MapTileAsyncRuntime::activeGeneration() const
{
    return active_generation_;
}

std::size_t MapTileAsyncRuntime::requestVisibleTiles(const MapViewportPlan& plan, uint32_t now_ms)
{
    if (plan.generation != active_generation_)
    {
        if (active_generation_ != 0)
        {
            (void)commands_.cancelGeneration(active_generation_);
        }
        active_generation_ = plan.generation;
    }

    std::size_t queued = 0;
    const std::size_t count = plan.tile_count < MapViewportPlan::kMaxTiles ? plan.tile_count
                                                                           : MapViewportPlan::kMaxTiles;
    for (std::size_t i = 0; i < count; ++i)
    {
        LoadTileCommand command{};
        command.tile = plan.tiles[i];
        command.runtime.command_id = next_command_id_++;
        command.runtime.kind = sys::runtime::RuntimeCommandKind::MapTileLoad;
        command.runtime.priority = priorityFor(plan.interaction_mode);
        command.runtime.cancel_policy = sys::runtime::RuntimeCancelPolicy::CancelByGeneration;
        command.runtime.created_at_ms = now_ms;
        command.runtime.generation = plan.generation;
        command.runtime.dedupe_key =
            (static_cast<uint32_t>(command.tile.z & 0xFF) << 24) ^
            ((command.tile.x & 0xFFFu) << 12) ^
            (command.tile.y & 0xFFFu) ^
            (static_cast<uint32_t>(command.tile.layer) << 28);

        if (commands_.enqueue(command))
        {
            ++queued;
        }
    }
    return queued;
}

bool MapTileAsyncRuntime::handleEvent(const MapTileAsyncEvent& event, MapTileRenderQueue& render_queue)
{
    if (event.generation != active_generation_)
    {
        return false;
    }

    MapTileRenderRef ref{};
    ref.tile = event.tile;
    switch (event.kind)
    {
    case MapTileAsyncEventKind::Ready:
        ref.state = MapTileRenderState::Ready;
        break;
    case MapTileAsyncEventKind::Cancelled:
        ref.state = MapTileRenderState::Cancelled;
        break;
    case MapTileAsyncEventKind::ResourceBusy:
        ref.state = MapTileRenderState::Loading;
        break;
    case MapTileAsyncEventKind::Failed:
    default:
        ref.state = MapTileRenderState::Error;
        break;
    }
    return render_queue.push(ref);
}

sys::runtime::RuntimePriority MapTileAsyncRuntime::priorityFor(MapTileInteractionMode mode)
{
    return mode == MapTileInteractionMode::InteractiveDrag ? sys::runtime::RuntimePriority::Interactive
                                                           : sys::runtime::RuntimePriority::Normal;
}

MapTileWorker::MapTileWorker(IMapTileWorkerBackend& backend,
                             sys::runtime::IBusArbiter& bus,
                             IMapTileEventSink& events,
                             uint8_t* scratch,
                             std::size_t scratch_size)
    : backend_(backend), bus_(bus), events_(events), scratch_(scratch), scratch_size_(scratch_size)
{
}

bool MapTileWorker::execute(const LoadTileCommand& command, uint32_t now_ms)
{
    sys::runtime::BusAcquireRequest request{};
    request.policy = busPolicyFor(command.runtime.priority);
    request.command_id = command.runtime.command_id;
    request.deadline_ms = command.runtime.deadline_ms;

    const auto acquired = bus_.acquire(request);
    if (acquired.status != sys::runtime::BusAcquireStatus::Acquired)
    {
        MapTileAsyncEvent event{};
        event.kind = MapTileAsyncEventKind::ResourceBusy;
        event.command_id = command.runtime.command_id;
        event.generation = command.runtime.generation;
        event.tile = command.tile;
        event.error = static_cast<int32_t>(acquired.status);
        (void)events_.publish(event);
        return false;
    }

    MapTileAsyncEvent event{};
    event.command_id = command.runtime.command_id;
    event.generation = command.runtime.generation;
    event.tile = command.tile;

    const auto lookup = backend_.lookup(command.tile);
    if (lookup.status != MapTileStatus::Available)
    {
        event.kind = MapTileAsyncEventKind::Failed;
        event.format = lookup.format;
        event.error = static_cast<int32_t>(lookup.status);
        bus_.release(acquired.token);
        (void)events_.publish(event);
        return false;
    }

    std::size_t out_size = 0;
    MapTileFormat out_format = MapTileFormat::Unknown;
    const bool ok = backend_.read(command.tile, scratch_, scratch_size_, out_size, out_format);
    bus_.release(acquired.token);

    event.kind = ok ? MapTileAsyncEventKind::Ready : MapTileAsyncEventKind::Failed;
    event.format = out_format;
    event.payload_size = out_size;
    event.error = ok ? 0 : -1;
    (void)events_.publish(event);
    (void)now_ms;
    return ok;
}

sys::runtime::BusAccessPolicy MapTileWorker::busPolicyFor(sys::runtime::RuntimePriority priority)
{
    return priority == sys::runtime::RuntimePriority::Interactive
               ? sys::runtime::BusAccessPolicy::InteractiveWorkerBounded
               : sys::runtime::BusAccessPolicy::BackgroundWorkerBounded;
}

} // namespace map_tiles
} // namespace ui
