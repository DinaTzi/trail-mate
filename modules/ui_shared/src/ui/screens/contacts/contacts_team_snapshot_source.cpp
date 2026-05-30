#include "ui/screens/contacts/contacts_team_snapshot_source.h"

namespace contacts::ui
{

ContactsTeamSnapshotSource::ContactsTeamSnapshotSource(
    ::team::ui::ITeamUiSnapshotStore& snapshot_store)
    : snapshot_store_(snapshot_store)
{
}

bool ContactsTeamSnapshotSource::load(ContactsTeamSnapshot& out)
{
    out = ContactsTeamSnapshot{};

    ::team::ui::TeamUiSnapshot snapshot;
    if (!snapshot_store_.load(snapshot))
    {
        return false;
    }

    out.in_team = snapshot.in_team;
    out.has_team_id = snapshot.has_team_id;
    out.team_id = snapshot.team_id;
    out.team_name = snapshot.team_name;
    out.available = snapshot.has_team_id;
    return out.available;
}

bool ContactsTeamSnapshotSource::isAvailable()
{
    ContactsTeamSnapshot snapshot;
    return load(snapshot) && snapshot.available;
}

std::string contactsTeamDisplayName(const ContactsTeamSnapshot& snapshot,
                                    const char* fallback)
{
    if (!snapshot.team_name.empty())
    {
        return snapshot.team_name;
    }
    return fallback != nullptr ? std::string(fallback) : std::string();
}

} // namespace contacts::ui
