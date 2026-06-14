#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace sys::runtime
{

enum class StorageWorkKind : uint8_t
{
    SnapshotSave,
    LogAppend,
    SnapshotRead,
    DeleteBlob,
};

enum class StorageWorkState : uint8_t
{
    Idle,
    Pending,
    InFlight,
    FailedPendingRetry,
};

struct StorageWorkItem
{
    StorageWorkKind kind = StorageWorkKind::SnapshotSave;
    uint32_t key = 0;
    uint32_t generation = 0;
    const uint8_t* data = nullptr;
    size_t len = 0;
};

template <size_t MaxBytes>
class LatestSnapshotStorageRuntime
{
  public:
    bool requestSave(uint32_t key, const uint8_t* data, size_t len)
    {
        if (len > MaxBytes || (len > 0 && data == nullptr))
        {
            return false;
        }
        key_ = key;
        len_ = len;
        if (len > 0)
        {
            std::memcpy(buffer_.data(), data, len);
        }
        ++generation_;
        pending_ = true;
        state_ = (state_ == StorageWorkState::InFlight)
                     ? StorageWorkState::InFlight
                     : StorageWorkState::Pending;
        return true;
    }

    bool takeNext(StorageWorkItem& out)
    {
        if (!pending_)
        {
            return false;
        }
        pending_ = false;
        in_flight_generation_ = generation_;
        state_ = StorageWorkState::InFlight;
        out.kind = StorageWorkKind::SnapshotSave;
        out.key = key_;
        out.generation = in_flight_generation_;
        out.data = buffer_.data();
        out.len = len_;
        return true;
    }

    void complete(uint32_t generation, bool ok)
    {
        if (generation != in_flight_generation_)
        {
            return;
        }
        if (ok)
        {
            state_ = pending_ ? StorageWorkState::Pending : StorageWorkState::Idle;
            last_completed_generation_ = generation;
            return;
        }
        pending_ = true;
        state_ = StorageWorkState::FailedPendingRetry;
    }

    bool pending() const { return pending_; }
    bool busy() const { return state_ == StorageWorkState::InFlight; }
    StorageWorkState state() const { return state_; }
    uint32_t generation() const { return generation_; }
    uint32_t lastCompletedGeneration() const { return last_completed_generation_; }
    size_t len() const { return len_; }

  private:
    std::array<uint8_t, MaxBytes> buffer_{};
    size_t len_ = 0;
    uint32_t key_ = 0;
    uint32_t generation_ = 0;
    uint32_t in_flight_generation_ = 0;
    uint32_t last_completed_generation_ = 0;
    bool pending_ = false;
    StorageWorkState state_ = StorageWorkState::Idle;
};

} // namespace sys::runtime
