#include "sys/storage_event_runtime.h"

#include <cassert>
#include <cstring>

using sys::runtime::LatestSnapshotStorageRuntime;
using sys::runtime::StorageWorkItem;
using sys::runtime::StorageWorkState;

namespace
{

void burst_updates_coalesce_to_latest_snapshot()
{
    LatestSnapshotStorageRuntime<16> runtime;
    const uint8_t first[] = {1, 2, 3};
    const uint8_t second[] = {9, 8};

    assert(runtime.requestSave(42, first, sizeof(first)));
    assert(runtime.requestSave(42, second, sizeof(second)));

    StorageWorkItem work;
    assert(runtime.takeNext(work));
    assert(work.key == 42);
    assert(work.generation == 2);
    assert(work.len == sizeof(second));
    assert(std::memcmp(work.data, second, sizeof(second)) == 0);
}

void completion_ignores_stale_generation()
{
    LatestSnapshotStorageRuntime<16> runtime;
    const uint8_t first[] = {1};
    const uint8_t second[] = {2};

    assert(runtime.requestSave(1, first, sizeof(first)));
    StorageWorkItem work;
    assert(runtime.takeNext(work));
    assert(runtime.requestSave(1, second, sizeof(second)));

    runtime.complete(work.generation + 100, true);
    assert(runtime.busy());

    runtime.complete(work.generation, true);
    assert(runtime.pending());
    assert(runtime.state() == StorageWorkState::Pending);
}

void failed_save_is_retried_with_same_generation()
{
    LatestSnapshotStorageRuntime<16> runtime;
    const uint8_t blob[] = {4, 5, 6};

    assert(runtime.requestSave(7, blob, sizeof(blob)));
    StorageWorkItem first;
    assert(runtime.takeNext(first));
    runtime.complete(first.generation, false);

    assert(runtime.pending());
    assert(runtime.state() == StorageWorkState::FailedPendingRetry);

    StorageWorkItem retry;
    assert(runtime.takeNext(retry));
    assert(retry.generation == first.generation);
    assert(retry.len == sizeof(blob));
    assert(std::memcmp(retry.data, blob, sizeof(blob)) == 0);
}

} // namespace

int main()
{
    burst_updates_coalesce_to_latest_snapshot();
    completion_ignores_stale_generation();
    failed_save_is_retried_with_same_generation();
    return 0;
}
