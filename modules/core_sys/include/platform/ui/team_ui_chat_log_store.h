/**
 * @file team_ui_chat_log_store.h
 * @brief Narrow Team chat log persistence contract.
 */

#pragma once

#include "team/domain/team_types.h"
#include "team/protocol/team_chat.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace team
{
namespace ui
{

struct TeamChatLogEntry
{
    bool incoming = false;
    uint32_t ts = 0;
    uint32_t peer_id = 0;
    team::proto::TeamChatType type = team::proto::TeamChatType::Text;
    std::vector<uint8_t> payload;
};

class ITeamUiChatLogStore
{
  public:
    virtual ~ITeamUiChatLogStore() = default;

    virtual bool appendText(const TeamId& team_id,
                            uint32_t peer_id,
                            bool incoming,
                            uint32_t ts,
                            const std::string& text) = 0;
    virtual bool appendStructured(const TeamId& team_id,
                                  uint32_t peer_id,
                                  bool incoming,
                                  uint32_t ts,
                                  team::proto::TeamChatType type,
                                  const std::vector<uint8_t>& payload) = 0;
    virtual bool loadRecent(const TeamId& team_id,
                            std::size_t max_count,
                            std::vector<TeamChatLogEntry>& out) = 0;
};

ITeamUiChatLogStore& team_ui_chat_log_store();

} // namespace ui
} // namespace team
