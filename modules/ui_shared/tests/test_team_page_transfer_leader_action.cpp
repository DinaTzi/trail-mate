#include "ui/screens/team/team_page_transfer_leader_action.h"

#include <cassert>
#include <vector>

namespace
{

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0xCA;
    id[1] = 0xFE;
    return id;
}

team::ui::TeamMemberUi makeMember(uint32_t node_id,
                                  const char* name,
                                  bool leader = false)
{
    team::ui::TeamMemberUi member;
    member.node_id = node_id;
    member.name = name;
    member.leader = leader;
    member.last_seen_s = 900;
    return member;
}

team::ui::TeamPageCommandState makeState()
{
    team::ui::TeamPageCommandState state;
    state.in_team = true;
    state.self_is_leader = true;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.team_name = "Trail";
    state.security_round = 7;
    state.has_team_psk = true;
    state.selected_member_index = 2;
    state.members.push_back(makeMember(0, "You", true));
    state.members.push_back(makeMember(0x22222222, "Ada"));
    state.members.push_back(makeMember(0x33333333, "Ben"));
    return state;
}

team::ui::TeamPageCommandReducer makeReducer()
{
    team::ui::TeamPageCommandContext context;
    context.now_s = 1234;
    context.self_node_id = 0x11111111;
    return team::ui::TeamPageCommandReducer(context);
}

team::ui::TeamPageKeyEventState makeKeyEventState()
{
    team::ui::TeamPageKeyEventState state;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.security_round = 7;
    state.last_event_seq = 4;
    return state;
}

class FakeKeyEventWriter final : public team::ui::ITeamPageKeyEventWriter
{
  public:
    bool appendKeyEvent(const team::TeamId& team_id,
                        team::ui::TeamKeyEventType type,
                        uint32_t event_seq,
                        uint32_t timestamp_s,
                        const uint8_t* payload,
                        size_t payload_size) override
    {
        append_count += 1;
        last_team_id = team_id;
        last_type = type;
        last_event_seq = event_seq;
        last_timestamp_s = timestamp_s;
        last_payload.assign(payload, payload + payload_size);
        return append_ok;
    }

    bool append_ok = true;
    int append_count = 0;
    team::TeamId last_team_id{};
    team::ui::TeamKeyEventType last_type =
        team::ui::TeamKeyEventType::TeamCreated;
    uint32_t last_event_seq = 0;
    uint32_t last_timestamp_s = 0;
    std::vector<uint8_t> last_payload;
};

class FakeController final : public team::ui::ITeamPageControllerPort
{
  public:
    void clearKeys() override {}
    void resetUiState() override {}

    bool setKeysFromPsk(const team::TeamId&,
                        uint32_t,
                        const uint8_t*,
                        size_t) override
    {
        return true;
    }

    bool sendKick(const team::proto::TeamKick&,
                  chat::ChannelId,
                  chat::NodeId) override
    {
        return true;
    }

    bool sendTransferLeader(
        const team::proto::TeamTransferLeader& transfer,
        chat::ChannelId,
        chat::NodeId dest) override
    {
        transfer_count += 1;
        last_transfer_target = transfer.target;
        last_dest = dest;
        return transfer_ok;
    }

    bool sendKeyDist(const team::proto::TeamKeyDist&,
                     chat::ChannelId,
                     chat::NodeId) override
    {
        return true;
    }

    bool sendKeyDistPlain(const team::proto::TeamKeyDist&,
                          chat::ChannelId,
                          chat::NodeId) override
    {
        return true;
    }

    bool sendKeyRequest(const team::proto::TeamKeyRequest&,
                        chat::ChannelId,
                        chat::NodeId) override
    {
        return true;
    }

    bool sendStatus(const team::proto::TeamStatus&,
                    chat::ChannelId,
                    chat::NodeId) override
    {
        return true;
    }

    bool sendStatusPlain(const team::proto::TeamStatus&,
                         chat::ChannelId,
                         chat::NodeId) override
    {
        return true;
    }

    team::TeamService::SendError lastSendError() const override
    {
        return error;
    }

    bool transfer_ok = true;
    team::TeamService::SendError error =
        team::TeamService::SendError::None;
    int transfer_count = 0;
    uint32_t last_transfer_target = 0;
    chat::NodeId last_dest = 1;
};

void testInvalidSelectionIsIgnored()
{
    auto state = makeState();
    state.selected_member_index = -1;
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects =
        team::ui::TeamPageTransferLeaderAction().transferLeader(
            state,
            key_event_state,
            makeReducer(),
            runtime,
            log);

    assert(!effects.accepted);
    assert(controller.transfer_count == 0);
    assert(writer.append_count == 0);
    assert(state.self_is_leader);
    assert(state.members[0].leader);
}

void testTransferSendsRuntimeUpdatesStateAndWritesKeyEvent()
{
    auto state = makeState();
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects =
        team::ui::TeamPageTransferLeaderAction().transferLeader(
            state,
            key_event_state,
            makeReducer(),
            runtime,
            log);

    assert(effects.accepted);
    assert(effects.sent_transfer);
    assert(effects.appended_key_event);
    assert(effects.failures.empty());
    assert(controller.transfer_count == 1);
    assert(controller.last_transfer_target == 0x33333333);
    assert(controller.last_dest == 0);
    assert(!state.self_is_leader);
    assert(!state.members[0].leader);
    assert(!state.members[1].leader);
    assert(state.members[2].leader);
    assert(writer.append_count == 1);
    assert(writer.last_team_id == testTeamId());
    assert(writer.last_type ==
           team::ui::TeamKeyEventType::LeaderTransferred);
    assert(writer.last_event_seq == 5);
    assert(writer.last_timestamp_s == 777);
    assert(key_event_state.last_event_seq == 5);
}

void testTransferFailureIsReturnedAsEffect()
{
    auto state = makeState();
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    controller.transfer_ok = false;
    controller.error = team::TeamService::SendError::MeshSendFail;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects =
        team::ui::TeamPageTransferLeaderAction().transferLeader(
            state,
            key_event_state,
            makeReducer(),
            runtime,
            log);

    assert(effects.accepted);
    assert(effects.failures.size() == 1);
    assert(effects.failures[0].action ==
           team::ui::TeamPageTransferLeaderFailureAction::Transfer);
    assert(effects.failures[0].kind ==
           team::ui::TeamPageTransferLeaderFailureKind::SendFailed);
    assert(effects.failures[0].needs_keys);
    assert(writer.append_count == 1);
}

void testMissingControllerStillReducesAndWritesKeyEvent()
{
    auto state = makeState();
    auto key_event_state = makeKeyEventState();
    team::ui::TeamPageRuntimePort runtime(nullptr, nullptr, nullptr);
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects =
        team::ui::TeamPageTransferLeaderAction().transferLeader(
            state,
            key_event_state,
            makeReducer(),
            runtime,
            log);

    assert(effects.accepted);
    assert(!effects.sent_transfer);
    assert(effects.appended_key_event);
    assert(!state.self_is_leader);
    assert(state.members[2].leader);
    assert(writer.append_count == 1);
}

} // namespace

int main()
{
    testInvalidSelectionIsIgnored();
    testTransferSendsRuntimeUpdatesStateAndWritesKeyEvent();
    testTransferFailureIsReturnedAsEffect();
    testMissingControllerStillReducesAndWritesKeyEvent();
    return 0;
}
