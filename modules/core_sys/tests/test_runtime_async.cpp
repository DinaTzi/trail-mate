#include "sys/runtime_async.h"

#include <cassert>

namespace
{

void test_priority_pop_order()
{
    sys::runtime::FixedCommandQueue<4> queue;

    sys::runtime::RuntimeCommand background{};
    background.command_id = 1;
    background.kind = sys::runtime::RuntimeCommandKind::PersistenceSave;
    background.priority = sys::runtime::RuntimePriority::Background;
    background.created_at_ms = 1;

    sys::runtime::RuntimeCommand interactive{};
    interactive.command_id = 2;
    interactive.kind = sys::runtime::RuntimeCommandKind::MapTileLoad;
    interactive.priority = sys::runtime::RuntimePriority::Interactive;
    interactive.created_at_ms = 2;

    assert(queue.enqueue(background));
    assert(queue.enqueue(interactive));

    sys::runtime::RuntimeCommand out{};
    assert(queue.popNext(10, out));
    assert(out.command_id == 2);
    assert(queue.popNext(10, out));
    assert(out.command_id == 1);
}

void test_dedupe_replace()
{
    sys::runtime::FixedCommandQueue<4> queue;

    sys::runtime::RuntimeCommand first{};
    first.command_id = 1;
    first.dedupe_key = 42;
    first.generation = 1;

    sys::runtime::RuntimeCommand replacement = first;
    replacement.command_id = 2;
    replacement.generation = 2;

    assert(queue.enqueueOrReplaceDedupe(first));
    assert(queue.enqueueOrReplaceDedupe(replacement));
    assert(queue.size() == 1);

    sys::runtime::RuntimeCommand out{};
    assert(queue.popNext(0, out));
    assert(out.command_id == 2);
    assert(out.generation == 2);
}

void test_cancel_generation()
{
    sys::runtime::FixedCommandQueue<4> queue;

    sys::runtime::RuntimeCommand a{};
    a.command_id = 1;
    a.generation = 7;
    sys::runtime::RuntimeCommand b{};
    b.command_id = 2;
    b.generation = 8;

    assert(queue.enqueue(a));
    assert(queue.enqueue(b));
    assert(queue.cancelGeneration(7) == 1);
    assert(queue.size() == 1);

    sys::runtime::RuntimeCommand out{};
    assert(queue.popNext(0, out));
    assert(out.command_id == 2);
}

void test_event_queue_bounds()
{
    sys::runtime::FixedEventQueue<1> events;
    sys::runtime::RuntimeEvent event{};
    event.event_id = 1;
    event.kind = sys::runtime::RuntimeEventKind::MapTileReady;

    assert(events.enqueue(event));
    assert(!events.enqueue(event));
    assert(events.size() == 1);
    sys::runtime::RuntimeEvent out{};
    assert(events.pop(out));
    assert(out.event_id == 1);
}

} // namespace

int main()
{
    test_priority_pop_order();
    test_dedupe_replace();
    test_cancel_generation();
    test_event_queue_bounds();
    return 0;
}
