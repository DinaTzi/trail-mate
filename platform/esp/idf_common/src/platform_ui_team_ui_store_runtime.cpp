#include "platform/ui/team_ui_store_runtime.h"

#include <algorithm>
#include <deque>
#include <map>
#include <utility>

namespace team::ui
{
namespace
{

uint64_t team_id_to_u64(const TeamId& id)
{
    uint64_t value = 0;
    for (size_t i = 0; i < id.size(); ++i)
    {
        value |= (static_cast<uint64_t>(id[i]) << (8U * i));
    }
    return value;
}

struct TeamUiRuntimeMemory
{
    std::map<uint64_t, std::deque<TeamChatLogEntry>> chat_logs{};
    std::map<uint64_t, std::deque<TeamPosSample>> latest_positions{};
};

TeamUiRuntimeMemory& runtime_memory()
{
    static TeamUiRuntimeMemory memory{};
    return memory;
}

constexpr size_t kMaxChatEntriesPerTeam = 64;
constexpr size_t kMaxPosSamplesPerTeam = 64;

TeamUiSnapshotMemoryStore s_snapshot_memory_store{};
ITeamUiSnapshotStore* s_snapshot_store = &s_snapshot_memory_store;

class TeamUiRuntimeChatLogStore final : public ITeamUiChatLogStore
{
  public:
    bool appendText(const TeamId& team_id,
                    uint32_t peer_id,
                    bool incoming,
                    uint32_t ts,
                    const std::string& text) override
    {
        return appendStructured(team_id,
                                peer_id,
                                incoming,
                                ts,
                                team::proto::TeamChatType::Text,
                                std::vector<uint8_t>(text.begin(), text.end()));
    }

    bool appendStructured(const TeamId& team_id,
                          uint32_t peer_id,
                          bool incoming,
                          uint32_t ts,
                          team::proto::TeamChatType type,
                          const std::vector<uint8_t>& payload) override
    {
        auto& chat_log = runtime_memory().chat_logs[team_id_to_u64(team_id)];
        TeamChatLogEntry entry;
        entry.incoming = incoming;
        entry.ts = ts;
        entry.peer_id = peer_id;
        entry.type = type;
        entry.payload = payload;
        chat_log.push_back(std::move(entry));
        while (chat_log.size() > kMaxChatEntriesPerTeam)
        {
            chat_log.pop_front();
        }
        return true;
    }

    bool loadRecent(const TeamId& team_id,
                    std::size_t max_count,
                    std::vector<TeamChatLogEntry>& out) override
    {
        out.clear();
        const auto it = runtime_memory().chat_logs.find(team_id_to_u64(team_id));
        if (it == runtime_memory().chat_logs.end())
        {
            return false;
        }

        const auto& log = it->second;
        const size_t count = std::min(max_count, log.size());
        auto start = log.end();
        std::advance(start, -static_cast<long>(count));
        out.assign(start, log.end());
        return !out.empty();
    }
};

TeamUiRuntimeChatLogStore s_chat_log_store{};

} // namespace

bool TeamUiSnapshotMemoryStore::has_snapshot_ = false;
TeamUiSnapshot TeamUiSnapshotMemoryStore::snapshot_{};

bool TeamUiSnapshotMemoryStore::load(TeamUiSnapshot& out)
{
    if (!has_snapshot_)
    {
        return false;
    }

    out = snapshot_;
    return true;
}

void TeamUiSnapshotMemoryStore::save(const TeamUiSnapshot& in)
{
    snapshot_ = in;
    has_snapshot_ = true;
}

void TeamUiSnapshotMemoryStore::clear()
{
    snapshot_ = TeamUiSnapshot{};
    has_snapshot_ = false;
    runtime_memory() = TeamUiRuntimeMemory{};
}

ITeamUiSnapshotStore& team_ui_snapshot_store()
{
    return *s_snapshot_store;
}

void team_ui_set_snapshot_store(ITeamUiSnapshotStore* store)
{
    s_snapshot_store = store ? store : &s_snapshot_memory_store;
}

ITeamUiStore& team_ui_get_store()
{
    return team_ui_snapshot_store();
}

void team_ui_set_store(ITeamUiStore* store)
{
    team_ui_set_snapshot_store(store);
}

ITeamUiChatLogStore& team_ui_chat_log_store()
{
    return s_chat_log_store;
}

bool team_ui_append_key_event(const TeamId& team_id,
                              TeamKeyEventType type,
                              uint32_t event_seq,
                              uint32_t ts,
                              const uint8_t* payload,
                              size_t len)
{
    (void)ts;
    (void)payload;
    (void)len;

    TeamUiSnapshot snapshot{};
    if (!team_ui_snapshot_store().load(snapshot))
    {
        snapshot = TeamUiSnapshot{};
    }

    snapshot.team_id = team_id;
    snapshot.has_team_id = true;
    snapshot.last_event_seq = event_seq;

    if (type == TeamKeyEventType::TeamCreated)
    {
        snapshot.in_team = true;
        snapshot.self_is_leader = true;
        snapshot.kicked_out = false;
    }

    team_ui_snapshot_store().save(snapshot);
    return true;
}

bool team_ui_posring_append(const TeamId& team_id,
                            uint32_t member_id,
                            int32_t lat_e7,
                            int32_t lon_e7,
                            int16_t alt_m,
                            uint16_t speed_dmps,
                            uint32_t ts)
{
    auto& positions = runtime_memory().latest_positions[team_id_to_u64(team_id)];
    TeamPosSample sample;
    sample.member_id = member_id;
    sample.lat_e7 = lat_e7;
    sample.lon_e7 = lon_e7;
    sample.alt_m = alt_m;
    sample.speed_dmps = speed_dmps;
    sample.ts = ts;
    positions.push_back(sample);
    while (positions.size() > kMaxPosSamplesPerTeam)
    {
        positions.pop_front();
    }
    return true;
}

bool team_ui_posring_load_latest(const TeamId& team_id, std::vector<TeamPosSample>& out)
{
    out.clear();
    const auto it = runtime_memory().latest_positions.find(team_id_to_u64(team_id));
    if (it == runtime_memory().latest_positions.end())
    {
        return false;
    }

    out.assign(it->second.begin(), it->second.end());
    return !out.empty();
}

bool team_ui_chatlog_append(const TeamId& team_id,
                            uint32_t peer_id,
                            bool incoming,
                            uint32_t ts,
                            const std::string& text)
{
    return team_ui_chat_log_store().appendText(team_id,
                                               peer_id,
                                               incoming,
                                               ts,
                                               text);
}

bool team_ui_chatlog_append_structured(const TeamId& team_id,
                                       uint32_t peer_id,
                                       bool incoming,
                                       uint32_t ts,
                                       team::proto::TeamChatType type,
                                       const std::vector<uint8_t>& payload)
{
    return team_ui_chat_log_store().appendStructured(team_id,
                                                     peer_id,
                                                     incoming,
                                                     ts,
                                                     type,
                                                     payload);
}

bool team_ui_chatlog_load_recent(const TeamId& team_id,
                                 size_t max_count,
                                 std::vector<TeamChatLogEntry>& out)
{
    return team_ui_chat_log_store().loadRecent(team_id, max_count, out);
}

bool team_ui_save_keys_now(const TeamId& team_id,
                           uint32_t key_id,
                           const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk)
{
    TeamUiSnapshot snapshot{};
    if (!team_ui_snapshot_store().load(snapshot))
    {
        snapshot = TeamUiSnapshot{};
    }
    snapshot.team_id = team_id;
    snapshot.has_team_id = true;
    snapshot.security_round = key_id;
    snapshot.team_psk = psk;
    snapshot.has_team_psk = true;
    team_ui_snapshot_store().save(snapshot);
    return true;
}

bool team_ui_append_member_track(const TeamId& team_id,
                                 uint32_t member_id,
                                 const team::proto::TeamTrackMessage& track)
{
    if (track.points.empty() || track.valid_mask == 0)
    {
        return false;
    }

    bool appended = false;
    for (size_t i = 0; i < track.points.size(); ++i)
    {
        if ((track.valid_mask & (1u << static_cast<uint32_t>(i))) == 0)
        {
            continue;
        }
        const auto& point = track.points[i];
        const uint32_t ts =
            track.start_ts + static_cast<uint32_t>(track.interval_s) * static_cast<uint32_t>(i);
        appended = team_ui_posring_append(team_id,
                                          member_id,
                                          point.lat_e7,
                                          point.lon_e7,
                                          0,
                                          0,
                                          ts) ||
                   appended;
    }
    return appended;
}

bool team_ui_get_member_track_path(const TeamId& team_id, uint32_t member_id, std::string& out_path)
{
    (void)team_id;
    (void)member_id;
    out_path.clear();
    return false;
}

} // namespace team::ui
