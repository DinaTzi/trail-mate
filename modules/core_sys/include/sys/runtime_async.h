#pragma once

#include <cstddef>
#include <cstdint>

namespace sys
{
namespace runtime
{

enum class RuntimeCommandKind : uint8_t
{
    Unknown,
    MapTileLoad,
    MapTileCancelGeneration,
    TrackStart,
    TrackStop,
    TrackAppendPoint,
    TrackFlush,
    TrackList,
    PersistenceSave,
    FeedbackNotice,
    ProtocolAction,
};

enum class RuntimeEventKind : uint8_t
{
    Unknown,
    CommandQueued,
    CommandStarted,
    CommandCompleted,
    CommandFailed,
    CommandCancelled,
    ResourceBusy,
    StorageSlow,
    StorageDegraded,
    MapTileReady,
    MapTileFailed,
    TrackStarted,
    TrackStopped,
    TrackFlushSucceeded,
    TrackFlushFailed,
    PersistenceSaved,
    PersistenceFailed,
    FeedbackPosted,
    FeedbackDropped,
};

enum class RuntimePriority : uint8_t
{
    Realtime = 0,
    Interactive = 16,
    Normal = 64,
    Background = 128,
    Idle = 192,
};

enum class RuntimeCancelPolicy : uint8_t
{
    None,
    CancelByGeneration,
    CancelByDedupeKey,
    DropIfStale,
};

enum class BusAccessPolicy : uint8_t
{
    UiNeverBlock,
    InteractiveWorkerBounded,
    BackgroundWorkerBounded,
    DurableCommit,
    RecoveryExclusive,
};

enum class BusAcquireStatus : uint8_t
{
    Acquired,
    Busy,
    TimedOut,
    Unavailable,
};

enum class StorageHealthStatus : uint8_t
{
    Healthy,
    Slow,
    Degraded,
    Unavailable,
    Recovering,
};

struct RuntimeCommand
{
    uint32_t command_id = 0;
    RuntimeCommandKind kind = RuntimeCommandKind::Unknown;
    RuntimePriority priority = RuntimePriority::Normal;
    RuntimeCancelPolicy cancel_policy = RuntimeCancelPolicy::None;
    uint32_t created_at_ms = 0;
    uint32_t deadline_ms = 0;
    uint32_t generation = 0;
    uint32_t dedupe_key = 0;
    uint32_t origin = 0;
};

struct RuntimeEvent
{
    uint32_t event_id = 0;
    RuntimeEventKind kind = RuntimeEventKind::Unknown;
    uint32_t command_id = 0;
    uint32_t timestamp_ms = 0;
    uint32_t generation = 0;
    int32_t error = 0;
};

struct BusAcquireRequest
{
    uint32_t resource = 0;
    BusAccessPolicy policy = BusAccessPolicy::BackgroundWorkerBounded;
    uint32_t command_id = 0;
    uint32_t deadline_ms = 0;
    uint32_t origin = 0;
};

struct BusAccessToken
{
    uint32_t resource = 0;
    uint32_t owner = 0;
    uint32_t acquired_ms = 0;
    bool valid = false;
};

struct BusDiagnostics
{
    uint32_t resource = 0;
    uint32_t owner = 0;
    uint32_t command_id = 0;
    uint32_t wait_ms = 0;
    uint32_t hold_ms = 0;
    BusAccessPolicy policy = BusAccessPolicy::BackgroundWorkerBounded;
};

struct BusAcquireResult
{
    BusAcquireStatus status = BusAcquireStatus::Unavailable;
    BusAccessToken token{};
    BusDiagnostics diagnostics{};
};

struct StorageHealthState
{
    StorageHealthStatus status = StorageHealthStatus::Healthy;
    int32_t last_error = 0;
    uint32_t last_transition_ms = 0;
};

class IBusArbiter
{
  public:
    virtual ~IBusArbiter() = default;

    virtual BusAcquireResult acquire(const BusAcquireRequest& request) = 0;
    virtual void release(const BusAccessToken& token) = 0;
    virtual StorageHealthState health() const = 0;
};

class IUiOwnerGuard
{
  public:
    virtual ~IUiOwnerGuard() = default;

    virtual bool isUiOwner() const = 0;
    virtual void recordForbiddenBlockingCall(RuntimeCommandKind kind) = 0;
};

template <typename T, std::size_t N>
class FixedRuntimeQueue
{
  public:
    FixedRuntimeQueue()
    {
        static_assert(N > 0, "FixedRuntimeQueue capacity must be > 0");
    }

    bool enqueue(const T& item)
    {
        if (size_ >= N)
        {
            return false;
        }
        items_[(head_ + size_) % N] = item;
        ++size_;
        return true;
    }

    bool pop(T& out)
    {
        if (size_ == 0)
        {
            return false;
        }
        out = items_[head_];
        head_ = (head_ + 1) % N;
        --size_;
        return true;
    }

    std::size_t size() const
    {
        return size_;
    }

    constexpr std::size_t capacity() const
    {
        return N;
    }

    bool empty() const
    {
        return size_ == 0;
    }

    bool full() const
    {
        return size_ >= N;
    }

    void clear()
    {
        head_ = 0;
        size_ = 0;
    }

    const T* at(std::size_t index) const
    {
        if (index >= size_)
        {
            return nullptr;
        }
        return &items_[(head_ + index) % N];
    }

    T* at(std::size_t index)
    {
        if (index >= size_)
        {
            return nullptr;
        }
        return &items_[(head_ + index) % N];
    }

  private:
    T items_[N]{};
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

template <std::size_t N>
class FixedCommandQueue
{
  public:
    bool enqueue(const RuntimeCommand& command)
    {
        return queue_.enqueue(command);
    }

    bool enqueueOrReplaceDedupe(const RuntimeCommand& command)
    {
        if (command.dedupe_key != 0)
        {
            for (std::size_t i = 0; i < queue_.size(); ++i)
            {
                RuntimeCommand* existing = queue_.at(i);
                if (existing && existing->dedupe_key == command.dedupe_key)
                {
                    *existing = command;
                    return true;
                }
            }
        }
        return queue_.enqueue(command);
    }

    bool popNext(uint32_t now_ms, RuntimeCommand& out)
    {
        if (queue_.empty())
        {
            return false;
        }

        std::size_t best_index = N;
        for (std::size_t i = 0; i < queue_.size(); ++i)
        {
            const RuntimeCommand* candidate = queue_.at(i);
            if (!candidate)
            {
                continue;
            }
            if (candidate->deadline_ms != 0 &&
                static_cast<int32_t>(candidate->deadline_ms - now_ms) < 0)
            {
                continue;
            }
            if (best_index == N)
            {
                best_index = i;
                continue;
            }
            const RuntimeCommand* best = queue_.at(best_index);
            if (!best ||
                static_cast<uint8_t>(candidate->priority) < static_cast<uint8_t>(best->priority) ||
                (candidate->priority == best->priority &&
                 candidate->created_at_ms < best->created_at_ms))
            {
                best_index = i;
            }
        }

        if (best_index == N)
        {
            return false;
        }

        RuntimeCommand selected{};
        FixedRuntimeQueue<RuntimeCommand, N> rebuilt;
        for (std::size_t i = 0; i < queue_.size(); ++i)
        {
            const RuntimeCommand* item = queue_.at(i);
            if (!item)
            {
                continue;
            }
            if (i == best_index)
            {
                selected = *item;
            }
            else
            {
                (void)rebuilt.enqueue(*item);
            }
        }
        queue_ = rebuilt;
        out = selected;
        return true;
    }

    std::size_t cancelGeneration(uint32_t generation)
    {
        return removeMatching(generation, 0, true);
    }

    std::size_t cancelDedupeKey(uint32_t dedupe_key)
    {
        return removeMatching(0, dedupe_key, false);
    }

    std::size_t size() const
    {
        return queue_.size();
    }

    bool empty() const
    {
        return queue_.empty();
    }

    void clear()
    {
        queue_.clear();
    }

    const RuntimeCommand* at(std::size_t index) const
    {
        return queue_.at(index);
    }

  private:
    std::size_t removeMatching(uint32_t generation, uint32_t dedupe_key, bool by_generation)
    {
        FixedRuntimeQueue<RuntimeCommand, N> rebuilt;
        std::size_t removed = 0;
        for (std::size_t i = 0; i < queue_.size(); ++i)
        {
            const RuntimeCommand* item = queue_.at(i);
            if (!item)
            {
                continue;
            }
            const bool match = by_generation ? item->generation == generation
                                             : item->dedupe_key == dedupe_key;
            if (match)
            {
                ++removed;
                continue;
            }
            (void)rebuilt.enqueue(*item);
        }
        queue_ = rebuilt;
        return removed;
    }

    FixedRuntimeQueue<RuntimeCommand, N> queue_;
};

template <std::size_t N>
using FixedEventQueue = FixedRuntimeQueue<RuntimeEvent, N>;

} // namespace runtime
} // namespace sys
