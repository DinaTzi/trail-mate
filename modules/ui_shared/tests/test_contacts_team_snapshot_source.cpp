#include "ui/screens/contacts/contacts_team_snapshot_source.h"

#include <cassert>

namespace
{

class FakeSnapshotStore final : public team::ui::ITeamUiSnapshotStore
{
  public:
    bool load(team::ui::TeamUiSnapshot& out) override
    {
        if (!has_snapshot)
        {
            return false;
        }
        out = snapshot;
        return true;
    }

    void save(const team::ui::TeamUiSnapshot& in) override
    {
        snapshot = in;
        has_snapshot = true;
    }

    void clear() override
    {
        has_snapshot = false;
        snapshot = team::ui::TeamUiSnapshot{};
    }

    bool has_snapshot = false;
    team::ui::TeamUiSnapshot snapshot;
};

team::TeamId testTeamId()
{
    team::TeamId id{};
    id[0] = 0xCA;
    id[1] = 0xFE;
    return id;
}

void testMissingSnapshotIsUnavailable()
{
    FakeSnapshotStore store;
    contacts::ui::ContactsTeamSnapshotSource source(store);
    contacts::ui::ContactsTeamSnapshot snapshot;

    assert(!source.load(snapshot));
    assert(!source.isAvailable());
    assert(!snapshot.available);
    assert(contacts::ui::contactsTeamDisplayName(snapshot, "Team") == "Team");
}

void testSnapshotProjectsContactsTeamFields()
{
    FakeSnapshotStore store;
    store.snapshot.has_team_id = true;
    store.snapshot.in_team = true;
    store.snapshot.team_id = testTeamId();
    store.snapshot.team_name = "Trail";
    store.has_snapshot = true;

    contacts::ui::ContactsTeamSnapshotSource source(store);
    contacts::ui::ContactsTeamSnapshot snapshot;

    assert(source.load(snapshot));
    assert(source.isAvailable());
    assert(snapshot.available);
    assert(snapshot.in_team);
    assert(snapshot.has_team_id);
    assert(snapshot.team_id == testTeamId());
    assert(snapshot.team_name == "Trail");
    assert(contacts::ui::contactsTeamDisplayName(snapshot, "Team") == "Trail");
}

void testTeamIdAloneKeepsTeamChatReachable()
{
    FakeSnapshotStore store;
    store.snapshot.has_team_id = true;
    store.snapshot.in_team = false;
    store.snapshot.team_id = testTeamId();
    store.has_snapshot = true;

    contacts::ui::ContactsTeamSnapshotSource source(store);
    contacts::ui::ContactsTeamSnapshot snapshot;

    assert(source.load(snapshot));
    assert(snapshot.available);
    assert(!snapshot.in_team);
    assert(contacts::ui::contactsTeamDisplayName(snapshot, "Team") == "Team");
}

} // namespace

int main()
{
    testMissingSnapshotIsUnavailable();
    testSnapshotProjectsContactsTeamFields();
    testTeamIdAloneKeepsTeamChatReachable();
    return 0;
}
