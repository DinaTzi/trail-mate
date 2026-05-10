#include "platform/gtk/gtk_uconsole_pages.h"
#include "platform/gtk/gtk_uconsole_shell.h"
#include "platform/gtk/gtk_uconsole_widgets.h"

#include <cstdio>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace trailmate::uconsole::gtk
{

int sortModeIndex(ChatThreadSortMode mode)
{
    switch (mode)
    {
    case ChatThreadSortMode::Hops:
        return 1;
    case ChatThreadSortMode::Distance:
        return 2;
    case ChatThreadSortMode::LastSeen:
        return 3;
    case ChatThreadSortMode::Recent:
    default:
        return 0;
    }
}

ChatThreadSortMode sortModeFromIndex(int index)
{
    switch (index)
    {
    case 1:
        return ChatThreadSortMode::Hops;
    case 2:
        return ChatThreadSortMode::Distance;
    case 3:
        return ChatThreadSortMode::LastSeen;
    case 0:
    default:
        return ChatThreadSortMode::Recent;
    }
}

void onConversationActivated(GtkListBox*,
                             GtkListBoxRow* row,
                             gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const guint index =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(row), "trailmate-index"));
    if (state.chat_model.selectConversationAt(index,
                                              kConversationLimit,
                                              state.chat_sort_mode))
    {
        refreshUi(state);
    }
}

void onConversationButtonClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const guint index = GPOINTER_TO_UINT(
        g_object_get_data(G_OBJECT(button), "trailmate-index"));
    if (state.chat_model.selectConversationAt(index,
                                              kConversationLimit,
                                              state.chat_sort_mode))
    {
        refreshUi(state);
    }
}

void onChatSortChanged(GtkComboBox* combo, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_sort_mode = sortModeFromIndex(gtk_combo_box_get_active(combo));
    refreshUi(state);
}

void onChatGroupExpandedChanged(GObject* object, GParamSpec*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const char* group =
        static_cast<const char*>(g_object_get_data(object, "trailmate-group"));
    if (group == nullptr)
    {
        return;
    }
    state.chat_group_expanded[group] =
        gtk_expander_get_expanded(GTK_EXPANDER(object));
}

void submitChatComposer(GtkUConsoleAppState& state)
{
    const char* text = gtk_editable_get_text(GTK_EDITABLE(state.chat_entry));
    if (state.chat_model.sendText(text ? text : ""))
    {
        gtk_editable_set_text(GTK_EDITABLE(state.chat_entry), "");
    }
    refreshUi(state);
}

gboolean scrollChatTranscriptToBottom(gpointer data)
{
    if (data == nullptr)
    {
        return G_SOURCE_REMOVE;
    }
    auto* scrolled = GTK_SCROLLED_WINDOW(data);
    GtkAdjustment* adjustment =
        gtk_scrolled_window_get_vadjustment(scrolled);
    if (adjustment != nullptr)
    {
        gtk_adjustment_set_value(adjustment,
                                 gtk_adjustment_get_upper(adjustment));
    }
    return G_SOURCE_REMOVE;
}

void onSendClicked(GtkButton*, gpointer data)
{
    submitChatComposer(*static_cast<GtkUConsoleAppState*>(data));
}

void onChatEntryActivate(GtkEntry*, gpointer data)
{
    submitChatComposer(*static_cast<GtkUConsoleAppState*>(data));
}

void onChatAddContactClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.addActivePeerAsContact();
    refreshUi(state);
}

void onChatRequestNodeInfoClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.requestActiveNodeInfo();
    refreshUi(state);
}

void onChatSendPositionClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.sendCurrentPosition();
    refreshUi(state);
}

void onChatSendPoiClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.sendCurrentPoi();
    refreshUi(state);
}

::chat::NodeId nodeIdFromButton(GtkButton* button)
{
    return static_cast<::chat::NodeId>(GPOINTER_TO_UINT(
        g_object_get_data(G_OBJECT(button), "trailmate-node-id")));
}

GtkWidget* makeNodeActionButton(GtkUConsoleAppState& state,
                                const char* label,
                                const ChatNodeInfoItem& item,
                                GCallback callback,
                                bool enabled = true)
{
    GtkWidget* button = gtk_button_new_with_label(label);
    gtk_widget_add_css_class(button, "chat-node-action");
    g_object_set_data(G_OBJECT(button),
                      "trailmate-node-id",
                      GUINT_TO_POINTER(static_cast<guint>(item.node_id)));
    gtk_widget_set_sensitive(button, enabled ? TRUE : FALSE);
    g_signal_connect(button, "clicked", callback, &state);
    return button;
}

void onChatNodeChatClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.selectNodeConversation(nodeIdFromButton(button));
    refreshUi(state);
}

void onChatNodeAddClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.addNodeAsContact(nodeIdFromButton(button));
    refreshUi(state);
}

void onChatNodeInfoClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.requestNodeInfo(nodeIdFromButton(button));
    refreshUi(state);
}

void onChatNodeIgnoreClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.toggleNodeIgnored(nodeIdFromButton(button));
    refreshUi(state);
}

void onChatNodeExchangeUserInfoClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.exchangeUserInfo(nodeIdFromButton(button));
    refreshUi(state);
}

void onChatNodeVerifyKeyClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.chat_model.verifyNodeKey(nodeIdFromButton(button));
    refreshUi(state);
}

GtkWidget* buildChatNodeInfoCard(GtkUConsoleAppState& state,
                                 const ChatNodeInfoItem& item)
{
    GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(card, "chat-node-card");
    if (item.via_mqtt)
    {
        gtk_widget_add_css_class(card, "chat-node-mqtt");
    }

    GtkWidget* title_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* title = makeLabel(item.title.c_str(), "row-title");
    gtk_widget_set_hexpand(title, TRUE);
    gtk_box_append(GTK_BOX(title_row), title);
    gtk_box_append(GTK_BOX(title_row),
                   makeLabel(item.via_mqtt ? "MQTT" : "LoRa", "mini-chip"));
    gtk_box_append(GTK_BOX(card), title_row);
    gtk_box_append(GTK_BOX(card),
                   makeLabel(item.subtitle.c_str(), "row-meta", true));
    gtk_box_append(GTK_BOX(card),
                   makeLabel(item.status.c_str(), "row-meta", true));
    gtk_box_append(GTK_BOX(card),
                   makeLabel(item.signal.c_str(), "row-meta", true));
    gtk_box_append(GTK_BOX(card),
                   makeLabel(item.position.c_str(),
                             item.has_position ? "chat-node-position"
                                               : "row-meta",
                             true));
    GtkWidget* actions = gtk_flow_box_new();
    gtk_widget_add_css_class(actions, "chat-node-actions");
    gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(actions), GTK_SELECTION_NONE);
    gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(actions), 3);
    gtk_flow_box_set_row_spacing(GTK_FLOW_BOX(actions), 4);
    gtk_flow_box_set_column_spacing(GTK_FLOW_BOX(actions), 4);
    gtk_flow_box_append(
        GTK_FLOW_BOX(actions),
        makeNodeActionButton(state,
                             "Chat",
                             item,
                             G_CALLBACK(onChatNodeChatClicked)));
    gtk_flow_box_append(GTK_FLOW_BOX(actions),
                        makeNodeActionButton(state,
                                             item.is_contact ? "Added" : "Add",
                                             item,
                                             G_CALLBACK(onChatNodeAddClicked),
                                             !item.is_contact));
    gtk_flow_box_append(
        GTK_FLOW_BOX(actions),
        makeNodeActionButton(state,
                             "Info",
                             item,
                             G_CALLBACK(onChatNodeInfoClicked)));
    gtk_flow_box_append(GTK_FLOW_BOX(actions),
                        makeNodeActionButton(
                            state,
                            item.is_ignored ? "Unignore" : "Ignore",
                            item,
                            G_CALLBACK(onChatNodeIgnoreClicked)));
    gtk_flow_box_append(GTK_FLOW_BOX(actions),
                        makeNodeActionButton(
                            state,
                            "Exchange",
                            item,
                            G_CALLBACK(onChatNodeExchangeUserInfoClicked)));
    gtk_flow_box_append(GTK_FLOW_BOX(actions),
                        makeNodeActionButton(
                            state,
                            item.key_verified ? "Trusted" : "Key",
                            item,
                            G_CALLBACK(onChatNodeVerifyKeyClicked),
                            !item.key_verified));
    gtk_box_append(GTK_BOX(card), actions);
    return card;
}

GtkWidget* makeConversationButton(const ChatConversationItem& item,
                                  std::size_t index,
                                  GtkUConsoleAppState& state)
{
    GtkWidget* row_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(row_box, "chat-thread-row");
    if (item.active)
    {
        gtk_widget_add_css_class(row_box, "chat-thread-active");
    }
    if (item.team)
    {
        gtk_widget_add_css_class(row_box, "chat-thread-team");
    }
    if (item.broadcast)
    {
        gtk_widget_add_css_class(row_box, "chat-thread-broadcast");
    }

    GtkWidget* title_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GtkWidget* title = makeLabel(item.title.c_str(), "chat-thread-title");
    gtk_widget_set_hexpand(title, TRUE);
    gtk_box_append(GTK_BOX(title_row), title);
    if (item.unread > 0)
    {
        char unread[16] = {};
        std::snprintf(unread, sizeof(unread), "%d", item.unread);
        GtkWidget* unread_label = makeLabel(unread, "chat-thread-unread");
        gtk_label_set_xalign(GTK_LABEL(unread_label), 0.5F);
        gtk_box_append(GTK_BOX(title_row), unread_label);
    }
    gtk_box_append(GTK_BOX(row_box), title_row);
    gtk_box_append(GTK_BOX(row_box),
                   makeLabel(item.preview.c_str(),
                             "chat-thread-preview",
                             true));
    if (!item.unread_source.empty())
    {
        gtk_box_append(GTK_BOX(row_box),
                       makeLabel(item.unread_source.c_str(),
                                 "chat-thread-unread-source",
                                 true));
    }
    gtk_box_append(GTK_BOX(row_box),
                   makeLabel(item.facts.c_str(), "chat-thread-facts", true));
    gtk_box_append(GTK_BOX(row_box),
                   makeLabel(item.meta.c_str(), "row-meta", true));

    GtkWidget* button = gtk_button_new();
    gtk_widget_add_css_class(button, "chat-thread-button");
    gtk_button_set_child(GTK_BUTTON(button), row_box);
    g_object_set_data(G_OBJECT(button),
                      "trailmate-index",
                      GUINT_TO_POINTER(static_cast<guint>(index)));
    g_signal_connect(button,
                     "clicked",
                     G_CALLBACK(onConversationButtonClicked),
                     &state);
    return button;
}

std::vector<std::pair<std::string, std::string>> chatGroupOrder()
{
    return {{"Nearby", "Nearby people"},
            {"Contacts", "Contacts"},
            {"Broadcast", "Broadcast"},
            {"Team", "Team"}};
}

void refreshConversationGroups(GtkUConsoleAppState& state,
                               const ChatWorkspaceSnapshot& snapshot)
{
    clearBox(state.chat_conversation_list);
    if (snapshot.conversations.empty())
    {
        GtkWidget* empty = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_add_css_class(empty, "empty-state");
        gtk_box_append(GTK_BOX(empty), makeLabel("No threads", "row-title"));
        gtk_box_append(GTK_BOX(empty),
                       makeLabel("No messages are stored locally.",
                                 "row-meta"));
        gtk_box_append(GTK_BOX(state.chat_conversation_list), empty);
        return;
    }

    for (const auto& group : chatGroupOrder())
    {
        std::vector<std::size_t> indexes{};
        for (std::size_t index = 0; index < snapshot.conversations.size();
             ++index)
        {
            if (snapshot.conversations[index].group == group.first)
            {
                indexes.push_back(index);
            }
        }
        if (indexes.empty())
        {
            continue;
        }

        const std::string label =
            group.second + " (" + std::to_string(indexes.size()) + ")";
        GtkWidget* expander = gtk_expander_new(label.c_str());
        gtk_widget_add_css_class(expander, "chat-group");
        const auto expanded_it = state.chat_group_expanded.find(group.first);
        gtk_expander_set_expanded(
            GTK_EXPANDER(expander),
            expanded_it == state.chat_group_expanded.end()
                ? TRUE
                : (expanded_it->second ? TRUE : FALSE));
        g_object_set_data_full(G_OBJECT(expander),
                               "trailmate-group",
                               g_strdup(group.first.c_str()),
                               g_free);
        g_signal_connect(expander,
                         "notify::expanded",
                         G_CALLBACK(onChatGroupExpandedChanged),
                         &state);

        GtkWidget* group_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_add_css_class(group_box, "chat-group-list");
        for (const std::size_t index : indexes)
        {
            gtk_box_append(GTK_BOX(group_box),
                           makeConversationButton(snapshot.conversations[index],
                                                  index,
                                                  state));
        }
        gtk_expander_set_child(GTK_EXPANDER(expander), group_box);
        gtk_box_append(GTK_BOX(state.chat_conversation_list), expander);
    }
}

static void refreshChat(GtkUConsoleAppState& state)
{
    ChatWorkspaceSnapshot snapshot = state.chat_model.snapshot(
        kConversationLimit,
        kMessageLimit,
        state.chat_sort_mode);

    if (state.chat_sort_combo != nullptr &&
        gtk_combo_box_get_active(GTK_COMBO_BOX(state.chat_sort_combo)) !=
            sortModeIndex(state.chat_sort_mode))
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(state.chat_sort_combo),
                                 sortModeIndex(state.chat_sort_mode));
    }

    std::ostringstream message_signature;
    message_signature << snapshot.active_title << '\n'
                      << snapshot.active_meta;
    for (const auto& item : snapshot.messages)
    {
        message_signature << '\n'
                          << (item.outgoing ? '>' : '<')
                          << (item.failed ? '!' : '.') << item.sender << '\t'
                          << item.meta << '\t' << item.text;
    }
    const std::string next_message_signature = message_signature.str();
    const bool message_list_changed =
        next_message_signature != state.chat_message_signature;
    state.chat_message_signature = next_message_signature;

    setLabel(state.chat_title, snapshot.active_title);
    setLabel(state.chat_meta, snapshot.active_meta);
    setLabel(state.chat_status,
             snapshot.action_status.empty() ? "Ready."
                                            : snapshot.action_status);
    gtk_widget_set_sensitive(state.chat_send_button,
                             snapshot.can_send ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_entry, snapshot.can_send ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_add_contact_button,
                             snapshot.can_contact_active_peer ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_request_nodeinfo_button,
                             snapshot.can_request_nodeinfo ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_send_position_button,
                             snapshot.can_send_position ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_send_poi_button,
                             snapshot.can_send_poi ? TRUE : FALSE);

    refreshConversationGroups(state, snapshot);

    if (state.chat_node_box != nullptr)
    {
        clearBox(state.chat_node_box);
        if (snapshot.nodes.empty())
        {
            GtkWidget* empty = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
            gtk_widget_add_css_class(empty, "empty-state");
            gtk_box_append(GTK_BOX(empty),
                           makeLabel("No node info", "row-title"));
            gtk_box_append(GTK_BOX(empty),
                           makeLabel("NodeInfo, Position and MQTT node details appear here after they are decoded.",
                                     "row-meta",
                                     true));
            gtk_box_append(GTK_BOX(state.chat_node_box), empty);
        }
        for (const auto& node : snapshot.nodes)
        {
            gtk_box_append(GTK_BOX(state.chat_node_box),
                           buildChatNodeInfoCard(state, node));
        }
    }

    clearListBox(state.chat_message_list);
    if (snapshot.messages.empty())
    {
        GtkWidget* empty = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_add_css_class(empty, "empty-state");
        gtk_box_append(GTK_BOX(empty), makeLabel("No messages", "row-title"));
        gtk_box_append(GTK_BOX(empty),
                       makeLabel("This conversation has no stored messages.",
                                 "row-meta"));
        gtk_list_box_append(GTK_LIST_BOX(state.chat_message_list), empty);
    }
    for (const auto& item : snapshot.messages)
    {
        GtkWidget* shell = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
        gtk_widget_add_css_class(shell, "chat-message-shell");
        gtk_widget_set_hexpand(shell, TRUE);

        GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_widget_set_hexpand(spacer, TRUE);

        GtkWidget* bubble = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
        gtk_widget_add_css_class(bubble, "chat-bubble");
        gtk_widget_add_css_class(bubble,
                                 item.failed
                                     ? "chat-bubble-failed"
                                     : (item.outgoing ? "chat-bubble-out"
                                                      : "chat-bubble-in"));
        gtk_widget_set_halign(bubble,
                              item.outgoing ? GTK_ALIGN_END : GTK_ALIGN_START);
        gtk_widget_set_hexpand(bubble, FALSE);

        GtkWidget* sender = makeLabel(item.sender.c_str(), "chat-sender");
        GtkWidget* text = makeLabel(item.text.c_str(), "chat-text", true);
        gtk_label_set_max_width_chars(GTK_LABEL(text), 56);
        GtkWidget* meta = makeLabel(item.meta.c_str(), "chat-message-meta");
        gtk_label_set_max_width_chars(GTK_LABEL(meta), 56);
        gtk_box_append(GTK_BOX(bubble), sender);
        gtk_box_append(GTK_BOX(bubble), text);
        gtk_box_append(GTK_BOX(bubble), meta);

        if (item.outgoing)
        {
            gtk_box_append(GTK_BOX(shell), spacer);
            gtk_box_append(GTK_BOX(shell), bubble);
        }
        else
        {
            gtk_box_append(GTK_BOX(shell), bubble);
            gtk_box_append(GTK_BOX(shell), spacer);
        }
        gtk_list_box_append(GTK_LIST_BOX(state.chat_message_list), shell);
    }
    if (message_list_changed && state.chat_message_scroll != nullptr)
    {
        g_idle_add(scrollChatTranscriptToBottom, state.chat_message_scroll);
    }
}

void refreshChatLogic(GtkUConsoleAppState& state,
                      const GtkUConsoleRefreshSnapshot&)
{
    refreshChat(state);
}

GtkUConsolePageLifecycle makeChatPageLifecycle()
{
    return {.name = "chat",
            .title = "Chat",
            .onLaunch = launchChatLayout,
            .onShow = nullptr,
            .onHide = nullptr,
            .onRefresh = refreshChatLogic,
            .onDestroy = nullptr};
}

} // namespace trailmate::uconsole::gtk
