#pragma once

#include "platform/ui/team_ui_snapshot_store.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace ui::team_persistence
{

constexpr uint8_t kTeamUiSnapshotVersionV1 = 1;
constexpr uint8_t kTeamUiSnapshotVersionV2 = 2;
constexpr uint8_t kTeamUiSnapshotCurrentVersion = kTeamUiSnapshotVersionV2;

bool encodeTeamUiSnapshot(const ::team::ui::TeamUiSnapshot& snapshot,
                          uint32_t updated_s,
                          std::vector<uint8_t>& out);

bool decodeTeamUiSnapshot(const uint8_t* data,
                          std::size_t len,
                          ::team::ui::TeamUiSnapshot& out);

} // namespace ui::team_persistence
