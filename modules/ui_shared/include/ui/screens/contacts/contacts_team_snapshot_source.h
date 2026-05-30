#pragma once

#include "platform/ui/team_ui_snapshot_store.h"

#include <string>

namespace contacts::ui
{

struct ContactsTeamSnapshot
{
    bool available = false;
    bool in_team = false;
    bool has_team_id = false;
    ::team::TeamId team_id{};
    std::string team_name;
};

class ContactsTeamSnapshotSource
{
  public:
    explicit ContactsTeamSnapshotSource(
        ::team::ui::ITeamUiSnapshotStore& snapshot_store);

    bool load(ContactsTeamSnapshot& out);
    bool isAvailable();

  private:
    ::team::ui::ITeamUiSnapshotStore& snapshot_store_;
};

std::string contactsTeamDisplayName(const ContactsTeamSnapshot& snapshot,
                                    const char* fallback);

} // namespace contacts::ui
