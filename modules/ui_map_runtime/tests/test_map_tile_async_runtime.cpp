#include "ui_map_runtime/map_tiles/map_tile_async_runtime.h"

#include <cassert>
#include <cstring>

namespace
{

ui::map_tiles::MapTileRef make_tile(uint32_t x)
{
    ui::map_tiles::MapTileRef ref{};
    ref.layer = ui::map_tiles::MapTileLayer::Osm;
    ref.z = 12;
    ref.x = x;
    ref.y = x + 1;
    return ref;
}

class FakeCommandSink final : public ui::map_tiles::IMapTileCommandSink
{
  public:
    ui::map_tiles::LoadTileCommand commands[8]{};
    std::size_t count = 0;
    std::size_t cancelled = 0;

    bool enqueue(const ui::map_tiles::LoadTileCommand& command) override
    {
        if (count >= 8)
        {
            return false;
        }
        commands[count++] = command;
        return true;
    }

    std::size_t cancelGeneration(uint32_t generation) override
    {
        std::size_t removed = 0;
        ui::map_tiles::LoadTileCommand kept[8]{};
        std::size_t kept_count = 0;
        for (std::size_t i = 0; i < count; ++i)
        {
            if (commands[i].runtime.generation == generation)
            {
                ++removed;
                continue;
            }
            kept[kept_count++] = commands[i];
        }
        for (std::size_t i = 0; i < kept_count; ++i)
        {
            commands[i] = kept[i];
        }
        count = kept_count;
        cancelled += removed;
        return removed;
    }
};

class FakeBusArbiter final : public sys::runtime::IBusArbiter
{
  public:
    sys::runtime::BusAcquireStatus next_status = sys::runtime::BusAcquireStatus::Acquired;
    sys::runtime::BusAccessPolicy last_policy = sys::runtime::BusAccessPolicy::BackgroundWorkerBounded;
    int acquire_count = 0;
    int release_count = 0;

    sys::runtime::BusAcquireResult acquire(const sys::runtime::BusAcquireRequest& request) override
    {
        ++acquire_count;
        last_policy = request.policy;
        sys::runtime::BusAcquireResult result{};
        result.status = next_status;
        result.token.valid = next_status == sys::runtime::BusAcquireStatus::Acquired;
        result.token.resource = request.resource;
        result.token.owner = request.command_id;
        return result;
    }

    void release(const sys::runtime::BusAccessToken& token) override
    {
        assert(token.valid);
        ++release_count;
    }

    sys::runtime::StorageHealthState health() const override
    {
        return {};
    }
};

class FakeBackend final : public ui::map_tiles::IMapTileWorkerBackend
{
  public:
    bool available = true;
    bool read_ok = true;

    ui::map_tiles::MapTileLookupResult lookup(const ui::map_tiles::MapTileRef& ref) override
    {
        (void)ref;
        ui::map_tiles::MapTileLookupResult result{};
        result.status = available ? ui::map_tiles::MapTileStatus::Available
                                  : ui::map_tiles::MapTileStatus::Missing;
        result.format = ui::map_tiles::MapTileFormat::Png;
        return result;
    }

    bool read(const ui::map_tiles::MapTileRef& ref,
              uint8_t* buffer,
              std::size_t capacity,
              std::size_t& out_size,
              ui::map_tiles::MapTileFormat& out_format) override
    {
        (void)ref;
        out_size = 0;
        out_format = ui::map_tiles::MapTileFormat::Png;
        if (!read_ok || !buffer || capacity < 3)
        {
            return false;
        }
        buffer[0] = 1;
        buffer[1] = 2;
        buffer[2] = 3;
        out_size = 3;
        return true;
    }
};

class FakeEventSink final : public ui::map_tiles::IMapTileEventSink
{
  public:
    ui::map_tiles::MapTileAsyncEvent events[4]{};
    std::size_t count = 0;

    bool publish(const ui::map_tiles::MapTileAsyncEvent& event) override
    {
        if (count >= 4)
        {
            return false;
        }
        events[count++] = event;
        return true;
    }
};

void test_generation_cancels_old_commands()
{
    FakeCommandSink sink;
    ui::map_tiles::MapTileAsyncRuntime runtime(sink);

    ui::map_tiles::MapViewportPlan first{};
    first.generation = 1;
    first.tile_count = 1;
    first.tiles[0] = make_tile(10);
    assert(runtime.requestVisibleTiles(first, 100) == 1);
    assert(sink.count == 1);

    ui::map_tiles::MapViewportPlan second{};
    second.generation = 2;
    second.tile_count = 1;
    second.tiles[0] = make_tile(20);
    assert(runtime.requestVisibleTiles(second, 120) == 1);
    assert(sink.cancelled == 1);
    assert(sink.count == 1);
    assert(sink.commands[0].runtime.generation == 2);
}

void test_stale_event_is_ignored()
{
    FakeCommandSink sink;
    ui::map_tiles::MapTileAsyncRuntime runtime(sink);
    ui::map_tiles::MapViewportPlan plan{};
    plan.generation = 2;
    plan.tile_count = 1;
    plan.tiles[0] = make_tile(20);
    assert(runtime.requestVisibleTiles(plan, 100) == 1);

    ui::map_tiles::MapTileRenderQueue render_queue;
    ui::map_tiles::MapTileAsyncEvent stale{};
    stale.kind = ui::map_tiles::MapTileAsyncEventKind::Ready;
    stale.generation = 1;
    stale.tile = make_tile(10);
    assert(!runtime.handleEvent(stale, render_queue));
    assert(render_queue.size() == 0);

    ui::map_tiles::MapTileAsyncEvent current = stale;
    current.generation = 2;
    current.tile = make_tile(20);
    assert(runtime.handleEvent(current, render_queue));
    assert(render_queue.size() == 1);
    assert(render_queue.items()[0].state == ui::map_tiles::MapTileRenderState::Ready);
}

void test_worker_busy_does_not_read_storage()
{
    FakeBackend backend;
    FakeBusArbiter bus;
    FakeEventSink events;
    uint8_t scratch[8]{};
    ui::map_tiles::MapTileWorker worker(backend, bus, events, scratch, sizeof(scratch));

    ui::map_tiles::LoadTileCommand command{};
    command.runtime.command_id = 7;
    command.runtime.generation = 3;
    command.runtime.priority = sys::runtime::RuntimePriority::Interactive;
    command.tile = make_tile(30);

    bus.next_status = sys::runtime::BusAcquireStatus::Busy;
    assert(!worker.execute(command, 200));
    assert(bus.acquire_count == 1);
    assert(bus.release_count == 0);
    assert(events.count == 1);
    assert(events.events[0].kind == ui::map_tiles::MapTileAsyncEventKind::ResourceBusy);
    assert(bus.last_policy == sys::runtime::BusAccessPolicy::InteractiveWorkerBounded);
}

void test_worker_success_publishes_ready()
{
    FakeBackend backend;
    FakeBusArbiter bus;
    FakeEventSink events;
    uint8_t scratch[8]{};
    ui::map_tiles::MapTileWorker worker(backend, bus, events, scratch, sizeof(scratch));

    ui::map_tiles::LoadTileCommand command{};
    command.runtime.command_id = 9;
    command.runtime.generation = 4;
    command.runtime.priority = sys::runtime::RuntimePriority::Normal;
    command.tile = make_tile(40);

    assert(worker.execute(command, 300));
    assert(bus.acquire_count == 1);
    assert(bus.release_count == 1);
    assert(events.count == 1);
    assert(events.events[0].kind == ui::map_tiles::MapTileAsyncEventKind::Ready);
    assert(events.events[0].payload_size == 3);
}

} // namespace

int main()
{
    test_generation_cancels_old_commands();
    test_stale_event_is_ignored();
    test_worker_busy_does_not_read_storage();
    test_worker_success_publishes_ready();
    return 0;
}
