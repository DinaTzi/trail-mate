#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "chat/domain/chat_types.h"

namespace trailmate::linux_app
{
class LinuxAppServices;
}

namespace trailmate::uconsole
{

enum class ChatThreadSortMode
{
    Recent,
    Hops,
    Distance,
    LastSeen,
};

struct ChatConversationItem
{
    ::chat::ConversationId id{};
    std::string group{};
    std::string title{};
    std::string preview{};
    std::string meta{};
    std::string facts{};
    std::string unread_source{};
    std::uint32_t last_seen = 0;
    std::uint8_t hops_away = 0xFF;
    double distance_m = 0.0;
    int unread = 0;
    bool has_distance = false;
    bool direct = false;
    bool contact = false;
    bool broadcast = false;
    bool team = false;
    bool active = false;
};

struct ChatMessageItem
{
    std::string sender{};
    std::string text{};
    std::string meta{};
    bool outgoing = false;
    bool failed = false;
};

struct ChatNodeInfoItem
{
    ::chat::NodeId node_id = 0;
    std::string title{};
    std::string subtitle{};
    std::string signal{};
    std::string position{};
    std::string status{};
    bool via_mqtt = false;
    bool has_position = false;
    bool is_contact = false;
    bool is_ignored = false;
    bool has_public_key = false;
    bool key_verified = false;
};

struct ChatWorkspaceSnapshot
{
    std::vector<ChatConversationItem> conversations{};
    std::vector<ChatMessageItem> messages{};
    std::vector<ChatNodeInfoItem> nodes{};
    ::chat::ConversationId active_conversation{};
    std::string active_title{};
    std::string active_meta{};
    std::string action_status{};
    std::size_t total_conversations = 0;
    int total_unread = 0;
    bool can_send = true;
    bool can_contact_active_peer = false;
    bool can_request_nodeinfo = false;
    bool can_send_position = false;
    bool can_send_poi = false;
};

class UConsoleChatWorkspaceModel final
{
  public:
    explicit UConsoleChatWorkspaceModel(linux_app::LinuxAppServices& services);

    [[nodiscard]] ChatWorkspaceSnapshot snapshot(std::size_t conversation_limit,
                                                 std::size_t message_limit,
                                                 ChatThreadSortMode sort_mode =
                                                     ChatThreadSortMode::Recent);

    bool selectConversationAt(std::size_t index,
                              std::size_t conversation_limit,
                              ChatThreadSortMode sort_mode =
                                  ChatThreadSortMode::Recent);
    bool selectConversation(const ::chat::ConversationId& conversation);
    bool selectPrimaryConversation();
    bool sendText(const std::string& text);
    bool sendCurrentPosition();
    bool sendCurrentPoi();
    bool requestActiveNodeInfo();
    bool addActivePeerAsContact();
    bool selectNodeConversation(::chat::NodeId node_id);
    bool addNodeAsContact(::chat::NodeId node_id);
    bool requestNodeInfo(::chat::NodeId node_id);
    bool exchangeUserInfo(::chat::NodeId node_id);
    bool toggleNodeIgnored(::chat::NodeId node_id);
    bool verifyNodeKey(::chat::NodeId node_id);

    [[nodiscard]] const ::chat::ConversationId& activeConversation() const
    {
        return active_conversation_;
    }

  private:
    void ensureActiveConversation();
    [[nodiscard]] ::chat::ConversationId primaryConversation() const;
    [[nodiscard]] bool canSendActiveConversation() const;
    [[nodiscard]] std::vector<::chat::ConversationMeta> loadConversationPage(
        std::size_t limit,
        std::size_t* total,
        ChatThreadSortMode sort_mode) const;

    linux_app::LinuxAppServices& services_;
    ::chat::ConversationId active_conversation_{};
    std::vector<::chat::ConversationId> displayed_conversations_{};
    std::string action_status_{};
    bool active_initialized_ = false;
};

} // namespace trailmate::uconsole
