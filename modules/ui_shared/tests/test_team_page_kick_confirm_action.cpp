#include "ui/screens/team/team_page_kick_confirm_action.h"

#include <cassert>
#include <utility>
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

team::ui::TeamPageCommandState makeKickState(uint32_t security_round = 7)
{
    team::ui::TeamPageCommandState state;
    state.in_team = true;
    state.self_is_leader = true;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.team_name = "Trail";
    state.security_round = security_round;
    state.has_team_psk = true;
    state.waiting_new_keys = true;
    state.selected_member_index = 1;
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

class FakeRandom final : public team::ui::ITeamPageKickConfirmRandom
{
  public:
    uint8_t nextByte() override
    {
        const uint8_t out = next;
        next = static_cast<uint8_t>(next + 1);
        return out;
    }

    uint8_t next = 0x80;
};

class FakeDeferred final : public team::ui::ITeamPageKickConfirmDeferred
{
  public:
    void enqueueKeyDist(uint32_t node_id, uint32_t key_id) override
    {
        enqueued.push_back(std::make_pair(node_id, key_id));
    }

    std::vector<std::pair<uint32_t, uint32_t>> enqueued;
};

class FakeController final : public team::ui::ITeamPageControllerPort
{
  public:
    void clearKeys() override {}
    void resetUiState() override {}

    bool setKeysFromPsk(const team::TeamId& team_id,
                        uint32_t key_id,
                        const uint8_t* psk,
                        size_t psk_len) override
    {
        set_keys_count += 1;
        last_set_keys_team_id = team_id;
        last_set_keys_key_id = key_id;
        last_set_keys_len = psk_len;
        last_set_keys_psk0 = psk ? psk[0] : 0;
        return set_keys_ok;
    }

    bool sendKick(const team::proto::TeamKick& kick,
                  chat::ChannelId,
                  chat::NodeId dest) override
    {
        kick_count += 1;
        last_kick_target = kick.target;
        last_dest = dest;
        return kick_ok;
    }

    bool sendTransferLeader(const team::proto::TeamTransferLeader&,
                            chat::ChannelId,
                            chat::NodeId) override
    {
        return true;
    }

    bool sendKeyDist(const team::proto::TeamKeyDist& key_dist,
                     chat::ChannelId,
                     chat::NodeId dest) override
    {
        keydist_count += 1;
        last_keydist = key_dist;
        last_keydist_dest = dest;
        return keydist_ok;
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

    bool sendStatus(const team::proto::TeamStatus& status,
                    chat::ChannelId,
                    chat::NodeId dest) override
    {
        status_count += 1;
        last_status = status;
        last_status_dest = dest;
        return status_ok;
    }

    bool sendStatusPlain(const team::proto::TeamStatus& status,
                         chat::ChannelId,
                         chat::NodeId dest) override
    {
        status_plain_count += 1;
        last_status_plain = status;
        last_status_plain_dest = dest;
        return status_plain_ok;
    }

    team::TeamService::SendError lastSendError() const override
    {
        return error;
    }

    bool kick_ok = true;
    bool keydist_ok = true;
    bool set_keys_ok = true;
    bool status_ok = true;
    bool status_plain_ok = true;
    team::TeamService::SendError error =
        team::TeamService::SendError::None;
    int kick_count = 0;
    int keydist_count = 0;
    int set_keys_count = 0;
    int status_count = 0;
    int status_plain_count = 0;
    uint32_t last_kick_target = 0;
    chat::NodeId last_dest = 0;
    chat::NodeId last_keydist_dest = 0;
    chat::NodeId last_status_dest = 1;
    chat::NodeId last_status_plain_dest = 1;
    team::TeamId last_set_keys_team_id{};
    uint32_t last_set_keys_key_id = 0;
    size_t last_set_keys_len = 0;
    uint8_t last_set_keys_psk0 = 0;
    team::proto::TeamKeyDist last_keydist;
    team::proto::TeamStatus last_status;
    team::proto::TeamStatus last_status_plain;
};

bool hasStatusMember(const team::proto::TeamStatus& status, uint32_t node_id)
{
    for (const auto member : status.members)
    {
        if (member == node_id)
        {
            return true;
        }
    }
    return false;
}

void testInvalidSelectionIsIgnored()
{
    auto state = makeKickState();
    state.selected_member_index = 9;
    auto reducer = makeReducer();
    FakeController controller;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeRandom random;
    FakeDeferred deferred;

    const auto effects = team::ui::TeamPageKickConfirmAction().confirmKick(
        state,
        reducer,
        runtime,
        random,
        deferred,
        0x11111111);

    assert(!effects.accepted);
    assert(controller.kick_count == 0);
    assert(controller.keydist_count == 0);
    assert(deferred.enqueued.empty());
    assert(state.members.size() == 3);
    assert(state.selected_member_index == 9);
}

void testKickRotatesKeysSendsKeyDistAndStatusWithoutTarget()
{
    auto state = makeKickState();
    auto reducer = makeReducer();
    FakeController controller;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeRandom random;
    FakeDeferred deferred;

    const auto effects = team::ui::TeamPageKickConfirmAction().confirmKick(
        state,
        reducer,
        runtime,
        random,
        deferred,
        0x11111111);

    assert(effects.accepted);
    assert(effects.command.member_kicked);
    assert(effects.command.member_kicked_id == 0x22222222);
    assert(effects.sent_kick);
    assert(effects.sent_keydist);
    assert(effects.enqueued_keydist);
    assert(effects.applied_keys);
    assert(effects.sent_status);
    assert(effects.failures.empty());

    assert(controller.kick_count == 1);
    assert(controller.last_kick_target == 0x22222222);
    assert(controller.keydist_count == 1);
    assert(controller.last_keydist_dest == 0x33333333);
    assert(controller.last_keydist.team_id == testTeamId());
    assert(controller.last_keydist.key_id == 8);
    assert(controller.last_keydist.channel_psk_len ==
           team::proto::kTeamChannelPskSize);
    assert(controller.last_keydist.channel_psk[0] == 0x80);
    assert(controller.last_keydist.channel_psk[1] == 0x81);

    assert(deferred.enqueued.size() == 1);
    assert(deferred.enqueued[0].first == 0x33333333);
    assert(deferred.enqueued[0].second == 8);

    assert(state.security_round == 8);
    assert(state.has_team_psk);
    assert(state.team_psk[0] == 0x80);
    assert(!state.waiting_new_keys);
    assert(state.selected_member_index == -1);
    assert(state.members.size() == 2);
    assert(state.members[0].node_id == 0);
    assert(state.members[1].node_id == 0x33333333);

    assert(controller.set_keys_count == 1);
    assert(controller.last_set_keys_team_id == testTeamId());
    assert(controller.last_set_keys_key_id == 8);
    assert(controller.last_set_keys_len == team::proto::kTeamChannelPskSize);
    assert(controller.last_set_keys_psk0 == 0x80);

    assert(controller.status_count == 1);
    assert(controller.status_plain_count == 1);
    assert(controller.last_status.key_id == 8);
    assert(controller.last_status.has_members);
    assert(controller.last_status.leader_id == 0x11111111);
    assert(hasStatusMember(controller.last_status, 0x11111111));
    assert(hasStatusMember(controller.last_status, 0x33333333));
    assert(!hasStatusMember(controller.last_status, 0x22222222));
}

void testZeroSecurityRoundRotatesFromFallbackRound()
{
    auto state = makeKickState(0);
    auto reducer = makeReducer();
    FakeController controller;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeRandom random;
    FakeDeferred deferred;

    const auto effects = team::ui::TeamPageKickConfirmAction().confirmKick(
        state,
        reducer,
        runtime,
        random,
        deferred,
        0x11111111);

    assert(effects.accepted);
    assert(state.security_round == 2);
    assert(controller.last_keydist.key_id == 2);
    assert(deferred.enqueued[0].second == 2);
}

void testFailuresAreReportedAsEffects()
{
    auto state = makeKickState();
    auto reducer = makeReducer();
    FakeController controller;
    controller.kick_ok = false;
    controller.keydist_ok = false;
    controller.set_keys_ok = false;
    controller.status_ok = false;
    controller.status_plain_ok = false;
    controller.error = team::TeamService::SendError::MeshSendFail;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, nullptr);
    FakeRandom random;
    FakeDeferred deferred;

    const auto effects = team::ui::TeamPageKickConfirmAction().confirmKick(
        state,
        reducer,
        runtime,
        random,
        deferred,
        0x11111111);

    assert(effects.accepted);
    assert(effects.failures.size() == 5);
    assert(effects.failures[0].action ==
           team::ui::TeamPageKickConfirmFailureAction::Kick);
    assert(effects.failures[0].kind ==
           team::ui::TeamPageKickConfirmFailureKind::SendFailed);
    assert(effects.failures[0].needs_keys);
    assert(effects.failures[1].action ==
           team::ui::TeamPageKickConfirmFailureAction::KeyDist);
    assert(effects.failures[1].kind ==
           team::ui::TeamPageKickConfirmFailureKind::SendFailedDetail);
    assert(effects.failures[1].error ==
           team::TeamService::SendError::MeshSendFail);
    assert(effects.failures[2].action ==
           team::ui::TeamPageKickConfirmFailureAction::Keys);
    assert(effects.failures[2].needs_keys);
    assert(effects.failures[3].action ==
           team::ui::TeamPageKickConfirmFailureAction::Status);
    assert(effects.failures[3].needs_keys);
    assert(effects.failures[4].action ==
           team::ui::TeamPageKickConfirmFailureAction::Status);
    assert(!effects.failures[4].needs_keys);
    assert(deferred.enqueued.size() == 1);
}

void testMissingControllerStillReducesLocalKick()
{
    auto state = makeKickState();
    auto reducer = makeReducer();
    team::ui::TeamPageRuntimePort runtime(nullptr, nullptr, nullptr);
    FakeRandom random;
    FakeDeferred deferred;

    const auto effects = team::ui::TeamPageKickConfirmAction().confirmKick(
        state,
        reducer,
        runtime,
        random,
        deferred,
        0x11111111);

    assert(effects.accepted);
    assert(!effects.sent_kick);
    assert(!effects.sent_keydist);
    assert(!effects.enqueued_keydist);
    assert(!effects.applied_keys);
    assert(!effects.sent_status);
    assert(effects.failures.empty());
    assert(state.security_round == 7);
    assert(state.members.size() == 2);
    assert(state.members[1].node_id == 0x33333333);
}

} // namespace

int main()
{
    testInvalidSelectionIsIgnored();
    testKickRotatesKeysSendsKeyDistAndStatusWithoutTarget();
    testZeroSecurityRoundRotatesFromFallbackRound();
    testFailuresAreReportedAsEffects();
    testMissingControllerStillReducesLocalKick();
    return 0;
}
