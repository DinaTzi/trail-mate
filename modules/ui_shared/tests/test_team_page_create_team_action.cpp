#include "ui/screens/team/team_page_create_team_action.h"

#include <cassert>
#include <string>
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

std::array<uint8_t, team::proto::kTeamChannelPskSize> testPsk(uint8_t seed)
{
    std::array<uint8_t, team::proto::kTeamChannelPskSize> psk{};
    for (size_t i = 0; i < psk.size(); ++i)
    {
        psk[i] = static_cast<uint8_t>(seed + i);
    }
    return psk;
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
    state.last_event_seq = 10;
    return state;
}

class FakeRandom final : public team::ui::ITeamPageCreateTeamRandom
{
  public:
    uint8_t nextByte() override
    {
        const uint8_t out = next;
        next = static_cast<uint8_t>(next + 1);
        return out;
    }

    uint8_t next = 0x20;
};

class FakeKeyEventWriter final : public team::ui::ITeamPageKeyEventWriter
{
  public:
    bool appendKeyEvent(const team::TeamId& team_id,
                        team::ui::TeamKeyEventType type,
                        uint32_t event_seq,
                        uint32_t,
                        const uint8_t* payload,
                        size_t payload_size) override
    {
        append_count += 1;
        last_team_id = team_id;
        last_type = type;
        last_event_seq = event_seq;
        last_payload.assign(payload, payload + payload_size);
        return append_ok;
    }

    bool append_ok = true;
    int append_count = 0;
    team::TeamId last_team_id{};
    team::ui::TeamKeyEventType last_type =
        team::ui::TeamKeyEventType::TeamCreated;
    uint32_t last_event_seq = 0;
    std::vector<uint8_t> last_payload;
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

    bool sendKick(const team::proto::TeamKick&,
                  chat::ChannelId,
                  chat::NodeId) override
    {
        return true;
    }

    bool sendTransferLeader(const team::proto::TeamTransferLeader&,
                            chat::ChannelId,
                            chat::NodeId) override
    {
        return true;
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

    bool set_keys_ok = true;
    team::TeamService::SendError error =
        team::TeamService::SendError::None;
    int set_keys_count = 0;
    team::TeamId last_set_keys_team_id{};
    uint32_t last_set_keys_key_id = 0;
    size_t last_set_keys_len = 0;
    uint8_t last_set_keys_psk0 = 0;
};

class FakePairing final : public team::ui::ITeamPagePairingPort
{
  public:
    bool startLeader(const team::TeamId& team_id,
                     uint32_t key_id,
                     const uint8_t* psk,
                     size_t psk_len,
                     uint32_t leader_id,
                     const char* team_name) override
    {
        start_leader_count += 1;
        last_team_id = team_id;
        last_key_id = key_id;
        last_psk_len = psk_len;
        last_psk0 = psk ? psk[0] : 0;
        last_leader_id = leader_id;
        last_team_name = team_name ? team_name : "";
        return start_ok;
    }

    bool startMember(uint32_t) override { return true; }
    void stop() override {}
    team::TeamPairingStatus status() const override { return {}; }

    bool start_ok = true;
    int start_leader_count = 0;
    team::TeamId last_team_id{};
    uint32_t last_key_id = 0;
    size_t last_psk_len = 0;
    uint8_t last_psk0 = 0;
    uint32_t last_leader_id = 0;
    std::string last_team_name;
};

class FakeKeyStore final : public team::ui::ITeamPageKeyStorePort
{
  public:
    bool saveKeysNow(
        const team::TeamId& team_id,
        uint32_t key_id,
        const std::array<uint8_t, team::proto::kTeamChannelPskSize>& psk) override
    {
        save_count += 1;
        last_team_id = team_id;
        last_key_id = key_id;
        last_psk0 = psk[0];
        return save_ok;
    }

    bool save_ok = true;
    int save_count = 0;
    team::TeamId last_team_id{};
    uint32_t last_key_id = 0;
    uint8_t last_psk0 = 0;
};

void testCreateGeneratesSecretsAppliesKeysAndStartsLeaderPairing()
{
    team::ui::TeamPageCommandState state;
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    FakePairing pairing;
    FakeKeyStore key_store;
    team::ui::TeamPageRuntimePort runtime(&controller, &pairing, &key_store);
    FakeRandom random;
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects = team::ui::TeamPageCreateTeamAction().createTeam(
        state,
        key_event_state,
        makeReducer(),
        runtime,
        log,
        random,
        0x11111111);

    assert(effects.accepted);
    assert(effects.appended_key_event);
    assert(effects.applied_keys);
    assert(effects.saved_keys);
    assert(effects.started_pairing);
    assert(effects.failures.empty());
    assert(state.in_team);
    assert(state.pending_join);
    assert(state.self_is_leader);
    assert(state.has_team_id);
    assert(state.team_id[0] == 0x20);
    assert(state.team_id[1] == 0x21);
    assert(state.security_round == 1);
    assert(state.has_team_psk);
    assert(state.team_psk[0] == 0x28);
    assert(state.team_name == "TEAM-2021");
    assert(state.pairing_role == team::TeamPairingRole::Leader);
    assert(state.pairing_state == team::TeamPairingState::LeaderBeacon);
    assert(state.members.size() == 1);
    assert(state.members[0].node_id == 0);
    assert(state.members[0].leader);

    assert(controller.set_keys_count == 1);
    assert(controller.last_set_keys_team_id == state.team_id);
    assert(controller.last_set_keys_key_id == 1);
    assert(controller.last_set_keys_psk0 == 0x28);
    assert(controller.last_set_keys_len == team::proto::kTeamChannelPskSize);
    assert(key_store.save_count == 1);
    assert(key_store.last_team_id == state.team_id);
    assert(key_store.last_key_id == 1);
    assert(key_store.last_psk0 == 0x28);
    assert(pairing.start_leader_count == 1);
    assert(pairing.last_team_id == state.team_id);
    assert(pairing.last_key_id == 1);
    assert(pairing.last_psk0 == 0x28);
    assert(pairing.last_psk_len == team::proto::kTeamChannelPskSize);
    assert(pairing.last_leader_id == 0x11111111);
    assert(pairing.last_team_name == "TEAM-2021");
    assert(writer.append_count == 1);
    assert(writer.last_type == team::ui::TeamKeyEventType::TeamCreated);
    assert(writer.last_event_seq == 11);
    assert(key_event_state.last_event_seq == 11);
}

void testCreateReusesExistingIdentityAndPsk()
{
    team::ui::TeamPageCommandState state;
    state.has_team_id = true;
    state.team_id = testTeamId();
    state.team_name = "Existing";
    state.security_round = 4;
    state.has_team_psk = true;
    state.team_psk = testPsk(0xA0);
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    FakePairing pairing;
    FakeKeyStore key_store;
    team::ui::TeamPageRuntimePort runtime(&controller, &pairing, &key_store);
    FakeRandom random;
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects = team::ui::TeamPageCreateTeamAction().createTeam(
        state,
        key_event_state,
        makeReducer(),
        runtime,
        log,
        random,
        0x11111111);

    assert(effects.accepted);
    assert(state.team_id == testTeamId());
    assert(state.team_name == "Existing");
    assert(state.security_round == 4);
    assert(state.team_psk[0] == 0xA0);
    assert(pairing.last_team_name == "Existing");
    assert(random.next == 0x20);
}

void testMissingPairingReturnsPairingNotReadyAfterLocalCreate()
{
    team::ui::TeamPageCommandState state;
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    FakeKeyStore key_store;
    team::ui::TeamPageRuntimePort runtime(&controller, nullptr, &key_store);
    FakeRandom random;
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects = team::ui::TeamPageCreateTeamAction().createTeam(
        state,
        key_event_state,
        makeReducer(),
        runtime,
        log,
        random,
        0x11111111);

    assert(effects.accepted);
    assert(!effects.started_pairing);
    assert(effects.failures.size() == 1);
    assert(effects.failures[0].action ==
           team::ui::TeamPageCreateTeamFailureAction::Pairing);
    assert(effects.failures[0].kind ==
           team::ui::TeamPageCreateTeamFailureKind::PairingNotReady);
    assert(state.in_team);
    assert(!state.pending_join);
}

void testSetKeysAndPairingInitFailuresAreReported()
{
    team::ui::TeamPageCommandState state;
    auto key_event_state = makeKeyEventState();
    FakeController controller;
    controller.set_keys_ok = false;
    FakePairing pairing;
    pairing.start_ok = false;
    FakeKeyStore key_store;
    team::ui::TeamPageRuntimePort runtime(&controller, &pairing, &key_store);
    FakeRandom random;
    FakeKeyEventWriter writer;
    team::ui::TeamPageKeyEventLog log(writer, 777);

    const auto effects = team::ui::TeamPageCreateTeamAction().createTeam(
        state,
        key_event_state,
        makeReducer(),
        runtime,
        log,
        random,
        0x11111111);

    assert(effects.accepted);
    assert(!effects.started_pairing);
    assert(effects.failures.size() == 2);
    assert(effects.failures[0].action ==
           team::ui::TeamPageCreateTeamFailureAction::Keys);
    assert(effects.failures[0].kind ==
           team::ui::TeamPageCreateTeamFailureKind::SendFailed);
    assert(effects.failures[0].needs_keys);
    assert(effects.failures[1].action ==
           team::ui::TeamPageCreateTeamFailureAction::Pairing);
    assert(effects.failures[1].kind ==
           team::ui::TeamPageCreateTeamFailureKind::PairingInitFailed);
}

} // namespace

int main()
{
    testCreateGeneratesSecretsAppliesKeysAndStartsLeaderPairing();
    testCreateReusesExistingIdentityAndPsk();
    testMissingPairingReturnsPairingNotReadyAfterLocalCreate();
    testSetKeysAndPairingInitFailuresAreReported();
    return 0;
}
