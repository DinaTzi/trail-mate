#include "platform/gtk/gtk_uconsole_app.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <gtk/gtk.h>

#include "app/linux_app_services.h"
#include "uconsole/uconsole_chat_workspace_model.h"
#include "uconsole/uconsole_dashboard_model.h"
#include "uconsole/uconsole_map_workspace_model.h"

namespace trailmate::uconsole::gtk
{
namespace
{

constexpr std::size_t kConversationLimit = 12;
constexpr std::size_t kMessageLimit = 28;

constexpr const char* kCss = R"CSS(
window {
  background: #ecefea;
  color: #1f2523;
}
.menu-bar {
  background: #202426;
  color: #f7f8f4;
  padding: 4px 8px;
  min-height: 30px;
}
.menu-title {
  color: #f7f8f4;
  font-weight: 700;
  padding: 0 8px;
}
.menu-button {
  background: transparent;
  color: #d7ded9;
  border-radius: 4px;
  padding: 4px 9px;
}
.chip {
  border-radius: 6px;
  padding: 4px 8px;
  color: #ecf4ef;
  background: #315f5e;
}
.body {
  padding: 8px;
}
.sidebar {
  background: #252a29;
  border-radius: 6px;
  padding: 12px;
}
.nav-title {
  color: #e8ede8;
  font-weight: 700;
}
.nav-button {
  background: #303635;
  color: #d7ded9;
  border-radius: 6px;
  padding: 7px 10px;
}
.nav-button-active {
  background: #d7e8df;
  color: #15251f;
}
.workspace-title {
  font-size: 20px;
  font-weight: 700;
}
.workspace-subtitle {
  color: #61706a;
  font-size: 12px;
}
.panel {
  background: #ffffff;
  border: 1px solid #d3d9d2;
  border-radius: 6px;
  padding: 10px;
}
.panel-attention {
  background: #fff8ec;
  border-color: #d18d3d;
}
.overview-grid {
  padding: 0;
}
.overview-panel-title {
  color: #25302c;
  font-weight: 700;
}
.summary-title {
  font-size: 18px;
  font-weight: 700;
}
.summary-detail {
  color: #53625d;
  font-size: 12px;
}
.location-map {
  background: #dce3dd;
  border: 1px solid #b8c4ba;
  border-radius: 5px;
  padding: 6px;
}
.location-picture {
  border-radius: 4px;
}
.timeline-list {
  padding: 0;
}
.timeline-row {
  background: #f6f8f5;
  border-left: 3px solid #3e6f67;
  border-radius: 5px;
  padding: 7px 8px;
}
.timeline-row-alert {
  background: #fff0e4;
  border-left-color: #b45c24;
}
.detail-grid {
  padding: 0;
}
.detail-panel {
  min-width: 260px;
}
.metric-value {
  font-size: 24px;
  font-weight: 700;
}
.metric-label {
  color: #66716e;
  font-size: 12px;
}
.row {
  background: #f6f8f5;
  border-radius: 6px;
  padding: 9px;
}
.row-active {
  background: #d7e8df;
  border: 1px solid #7ea48f;
}
.row-title {
  font-weight: 700;
}
.row-meta {
  color: #66716e;
  font-size: 12px;
}
.message-out {
  background: #eef8f3;
}
.message-in {
  background: #ffffff;
}
.message-failed {
  background: #fff0ee;
}
.empty-state {
  color: #66716e;
  padding: 16px;
}
.hardware-grid {
  padding: 0;
}
.hardware-card {
  background: #f7f8f4;
  border: 1px solid #c7d1c8;
  border-radius: 6px;
  padding: 8px;
}
.hardware-card-alert {
  background: #fff3e1;
  border-color: #d18d3d;
}
.hardware-state {
  font-size: 18px;
  font-weight: 700;
}
.hardware-state-alert {
  color: #8b3f00;
}
.statusbar {
  background: #202426;
  color: #e8ede8;
  padding: 5px 8px;
}
.status-chip {
  border-radius: 4px;
  padding: 3px 7px;
  background: #303635;
  color: #d7ded9;
}
.status-alert {
  background: #6b3426;
  color: #fff4ed;
}
.status-ok {
  background: #315f5e;
  color: #ecf4ef;
}
.map-controls {
  padding: 4px 0;
}
.map-grid {
  background: #d4dad4;
  border: 1px solid #b9c3ba;
  border-radius: 6px;
  padding: 8px;
}
.tile-cell {
  background: #f7f8f4;
  border: 1px solid #c7d1c8;
  border-radius: 4px;
  padding: 4px;
}
.tile-center {
  border-color: #2f7368;
}
.source-button-active {
  background: #d7e8df;
  color: #15251f;
}
button.send {
  border-radius: 6px;
  padding: 8px 14px;
}
)CSS";

GtkWidget* makeLabel(const char* text,
                     const char* css_class = nullptr,
                     bool wrap = false)
{
    GtkWidget* label = gtk_label_new(text ? text : "");
    gtk_label_set_xalign(GTK_LABEL(label), 0.0F);
    gtk_label_set_yalign(GTK_LABEL(label), 0.5F);
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
    if (wrap)
    {
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_wrap_mode(GTK_LABEL(label), PANGO_WRAP_WORD_CHAR);
        gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_NONE);
    }
    if (css_class != nullptr)
    {
        gtk_widget_add_css_class(label, css_class);
    }
    return label;
}

void setLabel(GtkWidget* label, const std::string& text)
{
    if (label != nullptr)
    {
        gtk_label_set_text(GTK_LABEL(label), text.c_str());
    }
}

void setLabel(GtkWidget* label, const char* text)
{
    if (label != nullptr)
    {
        gtk_label_set_text(GTK_LABEL(label), text ? text : "");
    }
}

GtkWidget* makeBox(GtkOrientation orientation, int spacing)
{
    GtkWidget* box = gtk_box_new(orientation, spacing);
    gtk_widget_set_hexpand(box, TRUE);
    return box;
}

void clearBox(GtkWidget* box)
{
    if (box == nullptr) return;
    while (GtkWidget* child = gtk_widget_get_first_child(box))
    {
        gtk_box_remove(GTK_BOX(box), child);
    }
}

void clearListBox(GtkWidget* list)
{
    if (list == nullptr) return;
    while (GtkWidget* child = gtk_widget_get_first_child(list))
    {
        gtk_list_box_remove(GTK_LIST_BOX(list), child);
    }
}

void clearGrid(GtkWidget* grid)
{
    if (grid == nullptr) return;
    while (GtkWidget* child = gtk_widget_get_first_child(grid))
    {
        gtk_grid_remove(GTK_GRID(grid), child);
    }
}

GtkWidget* makePanel()
{
    GtkWidget* panel = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_add_css_class(panel, "panel");
    gtk_widget_set_hexpand(panel, TRUE);
    return panel;
}

GtkWidget* makeRowBox(bool active = false)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(row, "row");
    if (active)
    {
        gtk_widget_add_css_class(row, "row-active");
    }
    gtk_widget_set_hexpand(row, TRUE);
    return row;
}

GtkWidget* makeListRow(GtkWidget* child, std::size_t index)
{
    GtkWidget* row = gtk_list_box_row_new();
    gtk_list_box_row_set_child(GTK_LIST_BOX_ROW(row), child);
    g_object_set_data(G_OBJECT(row), "trailmate-index",
                      GUINT_TO_POINTER(static_cast<guint>(index)));
    return row;
}

std::string tileKey(const ::platform::linux_runtime::MapTileId& tile)
{
    std::ostringstream out;
    out << static_cast<int>(tile.source) << ':' << tile.z << ':' << tile.x
        << ':' << tile.y;
    return out.str();
}

std::string formatBytes(std::uint64_t bytes)
{
    if (bytes >= 1024ULL * 1024ULL)
    {
        return std::to_string(bytes / (1024ULL * 1024ULL)) + " MB";
    }
    if (bytes >= 1024ULL)
    {
        return std::to_string(bytes / 1024ULL) + " KB";
    }
    return std::to_string(bytes) + " B";
}

const HardwareStatusItem* findHardware(const UConsoleDashboardSnapshot& snapshot,
                                       const char* name)
{
    for (const auto& item : snapshot.hardware)
    {
        if (item.name == name)
        {
            return &item;
        }
    }
    return nullptr;
}

std::string hardwareText(const HardwareStatusItem* item)
{
    if (item == nullptr)
    {
        return "-";
    }
    return item->name + ": " + item->state;
}

void setStatusChip(GtkWidget* label, const HardwareStatusItem* item)
{
    if (label == nullptr)
    {
        return;
    }
    setLabel(label, hardwareText(item));
    gtk_widget_remove_css_class(label, "status-alert");
    gtk_widget_remove_css_class(label, "status-ok");
    if (item != nullptr && item->attention)
    {
        gtk_widget_add_css_class(label, "status-alert");
    }
    else
    {
        gtk_widget_add_css_class(label, "status-ok");
    }
}

void setAttentionClass(GtkWidget* widget, bool attention)
{
    if (widget == nullptr)
    {
        return;
    }
    if (attention)
    {
        gtk_widget_add_css_class(widget, "panel-attention");
    }
    else
    {
        gtk_widget_remove_css_class(widget, "panel-attention");
    }
}

struct GtkUConsoleAppState
{
    explicit GtkUConsoleAppState(GtkUConsoleOptions options_in)
        : options(std::move(options_in)),
          services({.default_node_name = "Trail Mate uConsole",
                    .default_short_name = "TU",
                    .demo_broadcast_text =
                        "Broadcast: Trail Mate uConsole local mesh online."}),
          dashboard_model(services),
          chat_model(services),
          map_model(services)
    {
    }

    GtkUConsoleOptions options{};
    linux_app::LinuxAppServices services;
    UConsoleDashboardModel dashboard_model;
    UConsoleChatWorkspaceModel chat_model;
    UConsoleMapWorkspaceModel map_model;

    GtkWidget* window = nullptr;
    GtkWidget* stack = nullptr;
    GtkWidget* nav_overview = nullptr;
    GtkWidget* nav_chat = nullptr;
    GtkWidget* nav_map = nullptr;
    GtkWidget* nav_hardware = nullptr;
    GtkWidget* nav_data = nullptr;
    GtkWidget* nav_settings = nullptr;

    GtkWidget* status_aio2 = nullptr;
    GtkWidget* status_lora = nullptr;
    GtkWidget* status_gps = nullptr;
    GtkWidget* status_node = nullptr;
    GtkWidget* status_unread = nullptr;

    GtkWidget* overview_location_panel = nullptr;
    GtkWidget* overview_location_state = nullptr;
    GtkWidget* overview_location_coordinates = nullptr;
    GtkWidget* overview_location_detail = nullptr;
    GtkWidget* overview_location_map_meta = nullptr;
    GtkWidget* overview_location_map = nullptr;
    GtkWidget* overview_messages_panel = nullptr;
    GtkWidget* overview_messages_title = nullptr;
    GtkWidget* overview_messages_detail = nullptr;
    GtkWidget* overview_messages_latest = nullptr;
    GtkWidget* hardware_box = nullptr;
    GtkWidget* overview_conversations = nullptr;
    GtkWidget* team_summary = nullptr;
    GtkWidget* team_timeline_box = nullptr;
    GtkWidget* capability_box = nullptr;
    GtkWidget* hardware_page_box = nullptr;
    GtkWidget* data_page_box = nullptr;
    GtkWidget* settings_page_box = nullptr;

    GtkWidget* chat_conversation_list = nullptr;
    GtkWidget* chat_message_list = nullptr;
    GtkWidget* chat_title = nullptr;
    GtkWidget* chat_meta = nullptr;
    GtkWidget* chat_status = nullptr;
    GtkWidget* chat_entry = nullptr;
    GtkWidget* chat_send_button = nullptr;

    GtkWidget* map_title = nullptr;
    GtkWidget* map_meta = nullptr;
    GtkWidget* map_grid = nullptr;
    GtkWidget* map_status = nullptr;
    GtkWidget* map_cache_status = nullptr;
    GtkWidget* map_source_osm = nullptr;
    GtkWidget* map_source_terrain = nullptr;
    GtkWidget* map_source_satellite = nullptr;

    std::future<::platform::linux_runtime::MapTileResult> map_fetch_future{};
    std::set<std::string> map_failed_tiles{};
    std::string map_fetch_status{};

    guint refresh_source = 0;
};

void refreshUi(GtkUConsoleAppState& state);

void setActiveNav(GtkUConsoleAppState& state, const char* page)
{
    const std::string current(page ? page : "");
    if (state.nav_overview != nullptr)
    {
        if (current == "overview")
            gtk_widget_add_css_class(state.nav_overview, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_overview,
                                        "nav-button-active");
    }
    if (state.nav_chat != nullptr)
    {
        if (current == "chat")
            gtk_widget_add_css_class(state.nav_chat, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_chat, "nav-button-active");
    }
    if (state.nav_map != nullptr)
    {
        if (current == "map")
            gtk_widget_add_css_class(state.nav_map, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_map, "nav-button-active");
    }
    if (state.nav_hardware != nullptr)
    {
        if (current == "hardware")
            gtk_widget_add_css_class(state.nav_hardware, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_hardware,
                                        "nav-button-active");
    }
    if (state.nav_data != nullptr)
    {
        if (current == "data")
            gtk_widget_add_css_class(state.nav_data, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_data, "nav-button-active");
    }
    if (state.nav_settings != nullptr)
    {
        if (current == "settings")
            gtk_widget_add_css_class(state.nav_settings, "nav-button-active");
        else
            gtk_widget_remove_css_class(state.nav_settings,
                                        "nav-button-active");
    }
}

void showPage(GtkUConsoleAppState& state, const char* page)
{
    gtk_stack_set_visible_child_name(GTK_STACK(state.stack), page);
    setActiveNav(state, page);
    refreshUi(state);
}

void onOverviewClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "overview");
}

void onChatClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "chat");
}

void onMapClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "map");
}

void onHardwareClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "hardware");
}

void onDataClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "data");
}

void onSettingsClicked(GtkButton*, gpointer data)
{
    showPage(*static_cast<GtkUConsoleAppState*>(data), "settings");
}

void onConversationActivated(GtkListBox*,
                             GtkListBoxRow* row,
                             gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const guint index =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(row), "trailmate-index"));
    if (state.chat_model.selectConversationAt(index, kConversationLimit))
    {
        refreshUi(state);
    }
}

void onSendClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const char* text = gtk_editable_get_text(GTK_EDITABLE(state.chat_entry));
    if (state.chat_model.sendText(text ? text : ""))
    {
        gtk_editable_set_text(GTK_EDITABLE(state.chat_entry), "");
    }
    refreshUi(state);
}

void onMapSourceClicked(GtkButton* button, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    const guint source_value =
        GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button),
                                           "trailmate-source"));
    state.map_model.setSource(
        ::platform::linux_runtime::sanitize_map_base_source(source_value));
    state.map_failed_tiles.clear();
    state.map_fetch_status.clear();
    refreshUi(state);
}

void onMapZoomInClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.map_model.zoomIn();
    state.map_failed_tiles.clear();
    refreshUi(state);
}

void onMapZoomOutClicked(GtkButton*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.map_model.zoomOut();
    state.map_failed_tiles.clear();
    refreshUi(state);
}

gboolean onRefresh(gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    state.services.tick();
    refreshUi(state);
    return G_SOURCE_CONTINUE;
}

void onWindowDestroy(GtkWidget*, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    if (state.refresh_source != 0)
    {
        g_source_remove(state.refresh_source);
        state.refresh_source = 0;
    }
    if (state.map_fetch_future.valid())
    {
        state.map_fetch_future.wait();
    }
    state.services.shutdown();
}

void installCss()
{
    GtkCssProvider* provider = gtk_css_provider_new();
    gtk_css_provider_load_from_data(provider, kCss, -1);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(), GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);
}

GtkWidget* buildMenuBar(GtkUConsoleAppState& state)
{
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_add_css_class(bar, "menu-bar");
    gtk_widget_set_hexpand(bar, TRUE);

    gtk_box_append(GTK_BOX(bar), makeLabel("Trail Mate", "menu-title"));

    state.nav_overview = gtk_button_new_with_label("Overview");
    gtk_widget_add_css_class(state.nav_overview, "menu-button");
    g_signal_connect(state.nav_overview, "clicked",
                     G_CALLBACK(onOverviewClicked), &state);
    gtk_box_append(GTK_BOX(bar), state.nav_overview);

    state.nav_chat = gtk_button_new_with_label("Chat");
    gtk_widget_add_css_class(state.nav_chat, "menu-button");
    g_signal_connect(state.nav_chat, "clicked", G_CALLBACK(onChatClicked),
                     &state);
    gtk_box_append(GTK_BOX(bar), state.nav_chat);

    state.nav_map = gtk_button_new_with_label("Map");
    gtk_widget_add_css_class(state.nav_map, "menu-button");
    g_signal_connect(state.nav_map, "clicked", G_CALLBACK(onMapClicked),
                     &state);
    gtk_box_append(GTK_BOX(bar), state.nav_map);

    state.nav_hardware = gtk_button_new_with_label("Hardware");
    gtk_widget_add_css_class(state.nav_hardware, "menu-button");
    g_signal_connect(state.nav_hardware, "clicked",
                     G_CALLBACK(onHardwareClicked), &state);
    gtk_box_append(GTK_BOX(bar), state.nav_hardware);

    state.nav_data = gtk_button_new_with_label("Data");
    gtk_widget_add_css_class(state.nav_data, "menu-button");
    g_signal_connect(state.nav_data, "clicked", G_CALLBACK(onDataClicked),
                     &state);
    gtk_box_append(GTK_BOX(bar), state.nav_data);

    state.nav_settings = gtk_button_new_with_label("Settings");
    gtk_widget_add_css_class(state.nav_settings, "menu-button");
    g_signal_connect(state.nav_settings, "clicked",
                     G_CALLBACK(onSettingsClicked), &state);
    gtk_box_append(GTK_BOX(bar), state.nav_settings);

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(bar), spacer);

    GtkWidget* hint = makeLabel("uConsole Linux", "chip");
    gtk_box_append(GTK_BOX(bar), hint);
    return bar;
}

GtkWidget* buildOverview(GtkUConsoleAppState& state)
{
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);

    GtkWidget* grid = gtk_grid_new();
    gtk_widget_add_css_class(grid, "overview-grid");
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_widget_set_hexpand(grid, TRUE);
    gtk_widget_set_vexpand(grid, TRUE);

    state.overview_location_panel = makePanel();
    gtk_widget_set_size_request(state.overview_location_panel, 250, -1);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   makeLabel("Location", "overview-panel-title"));
    state.overview_location_map = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(state.overview_location_map, "location-map");
    gtk_widget_set_size_request(state.overview_location_map, 170, 132);
    gtk_widget_set_hexpand(state.overview_location_map, TRUE);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   state.overview_location_map);
    state.overview_location_state = makeLabel("", "summary-title");
    state.overview_location_coordinates = makeLabel("", "row-title");
    state.overview_location_detail = makeLabel("", "summary-detail", true);
    state.overview_location_map_meta = makeLabel("", "row-meta", true);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   state.overview_location_state);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   state.overview_location_coordinates);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   state.overview_location_detail);
    gtk_box_append(GTK_BOX(state.overview_location_panel),
                   state.overview_location_map_meta);
    gtk_grid_attach(GTK_GRID(grid), state.overview_location_panel, 0, 0, 1, 1);

    GtkWidget* hardware_panel = makePanel();
    gtk_widget_set_vexpand(hardware_panel, FALSE);
    gtk_box_append(GTK_BOX(hardware_panel),
                   makeLabel("Hardware state", "overview-panel-title"));
    state.hardware_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(state.hardware_box, "hardware-grid");
    gtk_widget_set_hexpand(state.hardware_box, TRUE);
    gtk_box_append(GTK_BOX(hardware_panel), state.hardware_box);
    gtk_grid_attach(GTK_GRID(grid), hardware_panel, 1, 0, 2, 1);

    state.overview_messages_panel = makePanel();
    gtk_widget_set_vexpand(state.overview_messages_panel, TRUE);
    gtk_box_append(GTK_BOX(state.overview_messages_panel),
                   makeLabel("Messages", "overview-panel-title"));
    state.overview_messages_title = makeLabel("", "summary-title");
    state.overview_messages_detail = makeLabel("", "summary-detail", true);
    state.overview_messages_latest = makeLabel("", nullptr, true);
    gtk_box_append(GTK_BOX(state.overview_messages_panel),
                   state.overview_messages_title);
    gtk_box_append(GTK_BOX(state.overview_messages_panel),
                   state.overview_messages_detail);
    gtk_box_append(GTK_BOX(state.overview_messages_panel),
                   state.overview_messages_latest);
    state.overview_conversations = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_vexpand(state.overview_conversations, TRUE);
    GtkWidget* message_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(message_scroll),
                                  state.overview_conversations);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(message_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(message_scroll, TRUE);
    gtk_box_append(GTK_BOX(state.overview_messages_panel), message_scroll);
    gtk_grid_attach(GTK_GRID(grid), state.overview_messages_panel, 0, 1, 1, 1);

    GtkWidget* team_panel = makePanel();
    gtk_widget_set_vexpand(team_panel, TRUE);
    gtk_box_append(GTK_BOX(team_panel),
                   makeLabel("Team activity", "overview-panel-title"));
    state.team_summary = makeLabel("", "summary-detail", true);
    gtk_box_append(GTK_BOX(team_panel), state.team_summary);
    state.team_timeline_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
    gtk_widget_add_css_class(state.team_timeline_box, "timeline-list");
    gtk_widget_set_vexpand(state.team_timeline_box, TRUE);
    GtkWidget* team_scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(team_scroll),
                                  state.team_timeline_box);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(team_scroll),
                                   GTK_POLICY_NEVER,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_vexpand(team_scroll, TRUE);
    gtk_box_append(GTK_BOX(team_panel), team_scroll);
    gtk_grid_attach(GTK_GRID(grid), team_panel, 1, 1, 1, 1);

    GtkWidget* runtime_panel = makePanel();
    gtk_widget_set_vexpand(runtime_panel, TRUE);
    gtk_widget_set_size_request(runtime_panel, 300, -1);
    gtk_box_append(GTK_BOX(runtime_panel),
                   makeLabel("Runtime", "overview-panel-title"));
    state.capability_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 7);
    gtk_box_append(GTK_BOX(runtime_panel), state.capability_box);
    gtk_grid_attach(GTK_GRID(grid), runtime_panel, 2, 1, 1, 1);

    gtk_box_append(GTK_BOX(root), grid);
    return root;
}

GtkWidget* buildChat(GtkUConsoleAppState& state)
{
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);

    state.chat_title = makeLabel("Chat workspace", "workspace-title");
    state.chat_meta = makeLabel("", "workspace-subtitle");
    gtk_box_append(GTK_BOX(root), state.chat_title);
    gtk_box_append(GTK_BOX(root), state.chat_meta);

    GtkWidget* body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_widget_set_hexpand(body, TRUE);
    gtk_widget_set_vexpand(body, TRUE);

    GtkWidget* conversation_panel = makePanel();
    gtk_widget_set_size_request(conversation_panel, 310, -1);
    gtk_widget_set_vexpand(conversation_panel, TRUE);
    gtk_box_append(GTK_BOX(conversation_panel),
                   makeLabel("Threads", "row-title"));
    state.chat_conversation_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state.chat_conversation_list),
                                    GTK_SELECTION_NONE);
    g_signal_connect(state.chat_conversation_list, "row-activated",
                     G_CALLBACK(onConversationActivated), &state);
    gtk_widget_set_vexpand(state.chat_conversation_list, TRUE);
    gtk_box_append(GTK_BOX(conversation_panel), state.chat_conversation_list);
    gtk_box_append(GTK_BOX(body), conversation_panel);

    GtkWidget* message_panel = makePanel();
    gtk_widget_set_vexpand(message_panel, TRUE);
    state.chat_message_list = gtk_list_box_new();
    gtk_list_box_set_selection_mode(GTK_LIST_BOX(state.chat_message_list),
                                    GTK_SELECTION_NONE);
    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll),
                                  state.chat_message_list);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(message_panel), scroll);

    GtkWidget* composer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    state.chat_entry = gtk_entry_new();
    gtk_widget_set_hexpand(state.chat_entry, TRUE);
    state.chat_send_button = gtk_button_new_with_label("Send");
    gtk_widget_add_css_class(state.chat_send_button, "send");
    g_signal_connect(state.chat_send_button, "clicked",
                     G_CALLBACK(onSendClicked), &state);
    gtk_box_append(GTK_BOX(composer), state.chat_entry);
    gtk_box_append(GTK_BOX(composer), state.chat_send_button);
    gtk_box_append(GTK_BOX(message_panel), composer);

    state.chat_status = makeLabel("Ready.", "row-meta");
    gtk_box_append(GTK_BOX(message_panel), state.chat_status);
    gtk_box_append(GTK_BOX(body), message_panel);
    gtk_box_append(GTK_BOX(root), body);
    return root;
}

GtkWidget* buildMapSourceButton(GtkUConsoleAppState& state,
                                GtkWidget** out_button,
                                const char* label,
                                ::platform::linux_runtime::MapBaseSource source)
{
    GtkWidget* button = gtk_button_new_with_label(label);
    gtk_widget_add_css_class(button, "nav-button");
    g_object_set_data(G_OBJECT(button), "trailmate-source",
                      GUINT_TO_POINTER(static_cast<guint>(source)));
    g_signal_connect(button, "clicked", G_CALLBACK(onMapSourceClicked),
                     &state);
    *out_button = button;
    return button;
}

GtkWidget* buildMap(GtkUConsoleAppState& state)
{
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);

    state.map_title = makeLabel("Map workspace", "workspace-title");
    state.map_meta = makeLabel("", "workspace-subtitle");
    gtk_box_append(GTK_BOX(root), state.map_title);
    gtk_box_append(GTK_BOX(root), state.map_meta);

    GtkWidget* controls = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(controls, "map-controls");
    gtk_widget_set_hexpand(controls, TRUE);
    gtk_box_append(GTK_BOX(controls),
                   buildMapSourceButton(
                       state,
                       &state.map_source_osm,
                       "OSM",
                       ::platform::linux_runtime::MapBaseSource::Osm));
    gtk_box_append(GTK_BOX(controls),
                   buildMapSourceButton(
                       state,
                       &state.map_source_terrain,
                       "Terrain",
                       ::platform::linux_runtime::MapBaseSource::Terrain));
    gtk_box_append(GTK_BOX(controls),
                   buildMapSourceButton(
                       state,
                       &state.map_source_satellite,
                       "Satellite",
                       ::platform::linux_runtime::MapBaseSource::Satellite));

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(controls), spacer);

    GtkWidget* zoom_out = gtk_button_new_with_label("-");
    gtk_widget_add_css_class(zoom_out, "nav-button");
    g_signal_connect(zoom_out, "clicked", G_CALLBACK(onMapZoomOutClicked),
                     &state);
    gtk_box_append(GTK_BOX(controls), zoom_out);

    GtkWidget* zoom_in = gtk_button_new_with_label("+");
    gtk_widget_add_css_class(zoom_in, "nav-button");
    g_signal_connect(zoom_in, "clicked", G_CALLBACK(onMapZoomInClicked),
                     &state);
    gtk_box_append(GTK_BOX(controls), zoom_in);
    gtk_box_append(GTK_BOX(root), controls);

    state.map_grid = gtk_grid_new();
    gtk_widget_add_css_class(state.map_grid, "map-grid");
    gtk_grid_set_row_spacing(GTK_GRID(state.map_grid), 6);
    gtk_grid_set_column_spacing(GTK_GRID(state.map_grid), 6);
    gtk_widget_set_hexpand(state.map_grid, TRUE);
    gtk_widget_set_vexpand(state.map_grid, TRUE);
    gtk_box_append(GTK_BOX(root), state.map_grid);

    state.map_status = makeLabel("", "row-meta", true);
    gtk_box_append(GTK_BOX(root), state.map_status);
    state.map_cache_status = makeLabel("", "row-meta", true);
    gtk_box_append(GTK_BOX(root), state.map_cache_status);
    return root;
}

GtkWidget* buildDetailsWorkspace(const char* title,
                                 const char* subtitle,
                                 GtkWidget** out_box)
{
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);
    gtk_box_append(GTK_BOX(root), makeLabel(title, "workspace-title"));
    gtk_box_append(GTK_BOX(root), makeLabel(subtitle, "workspace-subtitle"));

    GtkWidget* panel = makePanel();
    gtk_widget_add_css_class(panel, "detail-panel");
    *out_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
    gtk_widget_set_hexpand(*out_box, TRUE);
    gtk_box_append(GTK_BOX(panel), *out_box);

    GtkWidget* scroll = gtk_scrolled_window_new();
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), panel);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
                                   GTK_POLICY_AUTOMATIC,
                                   GTK_POLICY_AUTOMATIC);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_box_append(GTK_BOX(root), scroll);
    return root;
}

GtkWidget* buildHardware(GtkUConsoleAppState& state)
{
    return buildDetailsWorkspace(
        "Hardware",
        "uConsole/AIO2 endpoints and current driver binding state.",
        &state.hardware_page_box);
}

GtkWidget* buildData(GtkUConsoleAppState& state)
{
    return buildDetailsWorkspace(
        "Data",
        "Local SQLite state, message/contact counts, and map cache.",
        &state.data_page_box);
}

GtkWidget* buildSettings(GtkUConsoleAppState& state)
{
    return buildDetailsWorkspace(
        "Settings",
        "Current read-only runtime configuration for this build.",
        &state.settings_page_box);
}

GtkWidget* buildStatusBar(GtkUConsoleAppState& state)
{
    GtkWidget* bar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_widget_add_css_class(bar, "statusbar");
    gtk_widget_set_hexpand(bar, TRUE);

    state.status_aio2 = makeLabel("AIO2: -", "status-chip");
    state.status_lora = makeLabel("LoRa: -", "status-chip");
    state.status_gps = makeLabel("GPS: -", "status-chip");
    state.status_node = makeLabel("Node: -", "status-chip");
    state.status_unread = makeLabel("Unread: 0", "status-chip");

    gtk_box_append(GTK_BOX(bar), state.status_aio2);
    gtk_box_append(GTK_BOX(bar), state.status_lora);
    gtk_box_append(GTK_BOX(bar), state.status_gps);

    GtkWidget* spacer = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_set_hexpand(spacer, TRUE);
    gtk_box_append(GTK_BOX(bar), spacer);

    gtk_box_append(GTK_BOX(bar), state.status_node);
    gtk_box_append(GTK_BOX(bar), state.status_unread);
    return bar;
}

GtkWidget* buildRoot(GtkUConsoleAppState& state)
{
    GtkWidget* root = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_set_hexpand(root, TRUE);
    gtk_widget_set_vexpand(root, TRUE);
    gtk_box_append(GTK_BOX(root), buildMenuBar(state));

    GtkWidget* body = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_widget_add_css_class(body, "body");
    gtk_widget_set_hexpand(body, TRUE);
    gtk_widget_set_vexpand(body, TRUE);

    state.stack = gtk_stack_new();
    gtk_widget_set_hexpand(state.stack, TRUE);
    gtk_widget_set_vexpand(state.stack, TRUE);
    gtk_stack_add_named(GTK_STACK(state.stack), buildOverview(state),
                        "overview");
    gtk_stack_add_named(GTK_STACK(state.stack), buildChat(state), "chat");
    gtk_stack_add_named(GTK_STACK(state.stack), buildMap(state), "map");
    gtk_stack_add_named(GTK_STACK(state.stack), buildHardware(state),
                        "hardware");
    gtk_stack_add_named(GTK_STACK(state.stack), buildData(state), "data");
    gtk_stack_add_named(GTK_STACK(state.stack), buildSettings(state),
                        "settings");
    gtk_box_append(GTK_BOX(body), state.stack);

    gtk_box_append(GTK_BOX(root), body);
    gtk_box_append(GTK_BOX(root), buildStatusBar(state));
    showPage(state, "overview");
    return root;
}

GtkWidget* buildHardwareCard(const HardwareStatusItem& item)
{
    GtkWidget* card = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_add_css_class(card, "hardware-card");
    if (item.attention)
    {
        gtk_widget_add_css_class(card, "hardware-card-alert");
    }
    gtk_widget_set_hexpand(card, TRUE);

    gtk_box_append(GTK_BOX(card), makeLabel(item.name.c_str(), "metric-label"));
    GtkWidget* state_label = makeLabel(item.state.c_str(), "hardware-state");
    if (item.attention)
    {
        gtk_widget_add_css_class(state_label, "hardware-state-alert");
    }
    gtk_box_append(GTK_BOX(card), state_label);
    gtk_box_append(GTK_BOX(card),
                   makeLabel(item.detail.c_str(), "row-meta", true));
    return card;
}

void refreshHardware(GtkUConsoleAppState& state,
                     const UConsoleDashboardSnapshot& snapshot)
{
    clearBox(state.hardware_box);
    for (const auto& item : snapshot.hardware)
    {
        gtk_box_append(GTK_BOX(state.hardware_box), buildHardwareCard(item));
    }
}

void refreshLocationMiniMap(GtkUConsoleAppState& state,
                            const MapWorkspaceSnapshot& map_snapshot)
{
    clearBox(state.overview_location_map);
    if (!map_snapshot.has_fix)
    {
        gtk_box_append(GTK_BOX(state.overview_location_map),
                       makeLabel("No map center", "row-title"));
        gtk_box_append(GTK_BOX(state.overview_location_map),
                       makeLabel("GPS/NMEA source is not available.",
                                 "row-meta",
                                 true));
        return;
    }

    const MapTileItem* center_tile = nullptr;
    if (map_snapshot.tiles.size() > 4U)
    {
        center_tile = &map_snapshot.tiles[4U];
    }

    if (center_tile != nullptr && center_tile->available)
    {
        GtkWidget* picture =
            gtk_picture_new_for_filename(center_tile->path.string().c_str());
        gtk_widget_add_css_class(picture, "location-picture");
        gtk_picture_set_content_fit(GTK_PICTURE(picture),
                                    GTK_CONTENT_FIT_COVER);
        gtk_widget_set_size_request(picture, 160, 112);
        gtk_widget_set_hexpand(picture, TRUE);
        gtk_widget_set_vexpand(picture, TRUE);
        gtk_box_append(GTK_BOX(state.overview_location_map), picture);
    }
    else
    {
        gtk_box_append(GTK_BOX(state.overview_location_map),
                       makeLabel("Tile pending", "row-title"));
        gtk_box_append(GTK_BOX(state.overview_location_map),
                       makeLabel("Center tile is not cached yet.",
                                 "row-meta",
                                 true));
    }

    if (center_tile != nullptr)
    {
        const std::string tile_meta =
            "z" + std::to_string(center_tile->id.z) + " " +
            std::to_string(center_tile->id.x) + "/" +
            std::to_string(center_tile->id.y);
        gtk_box_append(GTK_BOX(state.overview_location_map),
                       makeLabel(tile_meta.c_str(), "row-meta"));
    }
}

GtkWidget* buildTimelineRow(const TeamTimelineItem& item)
{
    GtkWidget* row = gtk_box_new(GTK_ORIENTATION_VERTICAL, 3);
    gtk_widget_add_css_class(row, "timeline-row");
    if (item.attention)
    {
        gtk_widget_add_css_class(row, "timeline-row-alert");
    }
    gtk_widget_set_hexpand(row, TRUE);
    gtk_box_append(GTK_BOX(row), makeLabel(item.title.c_str(), "row-title"));
    gtk_box_append(GTK_BOX(row),
                   makeLabel(item.detail.c_str(), "row-meta", true));
    return row;
}

void refreshOverview(GtkUConsoleAppState& state,
                     const UConsoleDashboardSnapshot& snapshot,
                     const MapWorkspaceSnapshot& map_snapshot)
{
    setAttentionClass(state.overview_location_panel,
                      snapshot.location.attention);
    setAttentionClass(state.overview_messages_panel,
                      snapshot.messages.attention);
    setLabel(state.overview_location_state, snapshot.location.state);
    setLabel(state.overview_location_coordinates,
             snapshot.location.coordinates);
    setLabel(state.overview_location_detail, snapshot.location.detail);
    const std::string map_meta =
        map_snapshot.source_label + " / z" + std::to_string(map_snapshot.zoom) +
        " / " + std::to_string(map_snapshot.cache_stats.cached_tiles) +
        " cached tiles";
    setLabel(state.overview_location_map_meta,
             map_meta.empty() ? snapshot.location.map_meta : map_meta);
    refreshLocationMiniMap(state, map_snapshot);

    refreshHardware(state, snapshot);

    setLabel(state.overview_messages_title, snapshot.messages.title);
    setLabel(state.overview_messages_detail, snapshot.messages.detail);
    setLabel(state.overview_messages_latest, snapshot.messages.latest);

    clearBox(state.overview_conversations);
    if (snapshot.conversations.empty())
    {
        gtk_box_append(GTK_BOX(state.overview_conversations),
                       makeLabel("No messages yet.", "empty-state"));
    }
    for (const auto& item : snapshot.conversations)
    {
        GtkWidget* row = makeRowBox();
        gtk_box_append(GTK_BOX(row),
                       makeLabel(item.title.c_str(), "row-title"));
        gtk_box_append(GTK_BOX(row), makeLabel(item.preview.c_str(), nullptr,
                                               true));
        gtk_box_append(GTK_BOX(row), makeLabel(item.meta.c_str(), "row-meta"));
        gtk_box_append(GTK_BOX(state.overview_conversations), row);
    }

    setLabel(state.team_summary, snapshot.team_summary);
    clearBox(state.team_timeline_box);
    if (snapshot.team_timeline.empty())
    {
        gtk_box_append(GTK_BOX(state.team_timeline_box),
                       makeLabel("No team activity stored locally.",
                                 "empty-state"));
    }
    for (const auto& item : snapshot.team_timeline)
    {
        gtk_box_append(GTK_BOX(state.team_timeline_box),
                       buildTimelineRow(item));
    }
}

void refreshCapabilities(GtkUConsoleAppState& state,
                         const UConsoleDashboardSnapshot& snapshot)
{
    clearBox(state.capability_box);
    for (const auto& line : snapshot.capability_lines)
    {
        gtk_box_append(GTK_BOX(state.capability_box),
                       makeLabel(line.c_str(), "row-meta", true));
    }
}

GtkWidget* buildDetailRow(const std::string& title,
                          const std::string& detail,
                          bool attention = false)
{
    GtkWidget* row = makeRowBox();
    if (attention)
    {
        gtk_widget_add_css_class(row, "hardware-card-alert");
    }
    gtk_box_append(GTK_BOX(row), makeLabel(title.c_str(), "row-title"));
    gtk_box_append(GTK_BOX(row), makeLabel(detail.c_str(), "row-meta", true));
    return row;
}

void refreshHardwarePage(GtkUConsoleAppState& state,
                         const UConsoleDashboardSnapshot& snapshot)
{
    clearBox(state.hardware_page_box);
    for (const auto& item : snapshot.hardware)
    {
        gtk_box_append(GTK_BOX(state.hardware_page_box),
                       buildDetailRow(item.name + " / " + item.state,
                                      item.detail,
                                      item.attention));
    }
    gtk_box_append(GTK_BOX(state.hardware_page_box),
                   buildDetailRow("Driver boundary",
                                  "Detected Linux endpoints only prove that the hardware is present. LoRa/GPS protocol drivers still report separately when they are bound and producing data.",
                                  false));
}

void refreshDataPage(GtkUConsoleAppState& state,
                     const UConsoleDashboardSnapshot& dashboard,
                     const MapWorkspaceSnapshot& map_snapshot)
{
    clearBox(state.data_page_box);
    gtk_box_append(GTK_BOX(state.data_page_box),
                   buildDetailRow("SQLite database",
                                  map_snapshot.cache_stats.database.string()));
    gtk_box_append(GTK_BOX(state.data_page_box),
                   buildDetailRow("Messages",
                                  std::to_string(dashboard.conversation_count) +
                                      " threads / " +
                                      std::to_string(dashboard.unread_count) +
                                      " unread"));
    gtk_box_append(GTK_BOX(state.data_page_box),
                   buildDetailRow("Contacts",
                                  std::to_string(dashboard.contact_count) +
                                      " saved / " +
                                      std::to_string(dashboard.nearby_count) +
                                      " nearby / " +
                                      std::to_string(dashboard.ignored_count) +
                                      " ignored"));
    gtk_box_append(GTK_BOX(state.data_page_box),
                   buildDetailRow("Map cache",
                                  std::to_string(
                                      map_snapshot.cache_stats.cached_tiles) +
                                      " cached / " +
                                      std::to_string(
                                          map_snapshot.cache_stats.failed_tiles) +
                                      " failed / " +
                                      formatBytes(
                                          map_snapshot.cache_stats.total_bytes)));
    gtk_box_append(GTK_BOX(state.data_page_box),
                   buildDetailRow("Map cache root",
                                  map_snapshot.cache_stats.root.string()));
}

void refreshSettingsPage(GtkUConsoleAppState& state,
                         const UConsoleDashboardSnapshot& dashboard,
                         const MapWorkspaceSnapshot& map_snapshot)
{
    clearBox(state.settings_page_box);
    const auto& config = state.services.config();
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("Identity",
                                  std::string(config.node_name) + " / " +
                                      config.short_name + " / node " +
                                      dashboard.self_node));
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("Mesh protocol", dashboard.mesh_protocol));
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("GPS",
                                  std::string(config.gps_enabled ? "enabled"
                                                                 : "disabled") +
                                      " / interval " +
                                      std::to_string(config.gps_interval_ms) +
                                      " ms"));
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("Map",
                                  map_snapshot.source_label + " / z" +
                                      std::to_string(map_snapshot.zoom)));
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("BLE",
                                  "not used on Linux/uConsole"));
    gtk_box_append(GTK_BOX(state.settings_page_box),
                   buildDetailRow("Settings surface",
                                  "Read-only for this slice. Editable controls will be added when the corresponding Linux runtime actions are wired.",
                                  true));
}

void refreshChat(GtkUConsoleAppState& state)
{
    ChatWorkspaceSnapshot snapshot =
        state.chat_model.snapshot(kConversationLimit, kMessageLimit);

    setLabel(state.chat_title, snapshot.active_title);
    setLabel(state.chat_meta, snapshot.active_meta);
    setLabel(state.chat_status,
             snapshot.action_status.empty() ? "Ready." : snapshot.action_status);
    gtk_widget_set_sensitive(state.chat_send_button,
                             snapshot.can_send ? TRUE : FALSE);
    gtk_widget_set_sensitive(state.chat_entry, snapshot.can_send ? TRUE : FALSE);

    clearListBox(state.chat_conversation_list);
    if (snapshot.conversations.empty())
    {
        GtkWidget* empty = makeRowBox();
        gtk_box_append(GTK_BOX(empty), makeLabel("No threads", "row-title"));
        gtk_box_append(GTK_BOX(empty),
                       makeLabel("No messages are stored locally.",
                                 "row-meta"));
        gtk_list_box_append(GTK_LIST_BOX(state.chat_conversation_list), empty);
    }
    for (std::size_t index = 0; index < snapshot.conversations.size(); ++index)
    {
        const auto& item = snapshot.conversations[index];
        GtkWidget* row_box = makeRowBox(item.active);
        gtk_box_append(GTK_BOX(row_box),
                       makeLabel(item.title.c_str(), "row-title"));
        gtk_box_append(GTK_BOX(row_box),
                       makeLabel(item.preview.c_str(), nullptr, true));
        gtk_box_append(GTK_BOX(row_box),
                       makeLabel(item.meta.c_str(), "row-meta"));
        gtk_list_box_append(GTK_LIST_BOX(state.chat_conversation_list),
                            makeListRow(row_box, index));
    }

    clearListBox(state.chat_message_list);
    if (snapshot.messages.empty())
    {
        GtkWidget* empty = makeRowBox();
        gtk_box_append(GTK_BOX(empty), makeLabel("No messages", "row-title"));
        gtk_box_append(GTK_BOX(empty),
                       makeLabel("This conversation has no stored messages.",
                                 "row-meta"));
        gtk_list_box_append(GTK_LIST_BOX(state.chat_message_list), empty);
    }
    for (const auto& item : snapshot.messages)
    {
        GtkWidget* row = makeRowBox();
        gtk_widget_add_css_class(row, item.failed ? "message-failed"
                                                  : (item.outgoing
                                                         ? "message-out"
                                                         : "message-in"));
        gtk_box_append(GTK_BOX(row), makeLabel(item.sender.c_str(),
                                               "row-title"));
        gtk_box_append(GTK_BOX(row), makeLabel(item.text.c_str(), nullptr,
                                               true));
        gtk_box_append(GTK_BOX(row), makeLabel(item.meta.c_str(), "row-meta"));
        gtk_list_box_append(GTK_LIST_BOX(state.chat_message_list), row);
    }
}

void setActiveMapSourceButton(
    GtkWidget* button,
    ::platform::linux_runtime::MapBaseSource expected,
    const std::string& active_label)
{
    if (button == nullptr) return;
    const bool active =
        active_label ==
        ::platform::linux_runtime::map_base_source_label(expected);
    if (active)
        gtk_widget_add_css_class(button, "source-button-active");
    else
        gtk_widget_remove_css_class(button, "source-button-active");
}

void pollMapFetch(GtkUConsoleAppState& state)
{
    if (!state.map_fetch_future.valid())
    {
        return;
    }

    using namespace std::chrono_literals;
    if (state.map_fetch_future.wait_for(0ms) != std::future_status::ready)
    {
        return;
    }

    const auto result = state.map_fetch_future.get();
    if (result.status == ::platform::linux_runtime::MapTileStatus::Failed)
    {
        state.map_failed_tiles.insert(tileKey(result.tile));
        state.map_fetch_status =
            "Tile fetch failed: " + result.message;
        return;
    }

    state.map_fetch_status = "Tile cache updated.";
}

void maybeStartMapFetch(GtkUConsoleAppState& state,
                        const MapWorkspaceSnapshot& snapshot)
{
    pollMapFetch(state);
    if (state.map_fetch_future.valid() || !snapshot.has_fix)
    {
        return;
    }

    for (const auto& item : snapshot.tiles)
    {
        if (item.available)
        {
            continue;
        }
        const std::string key = tileKey(item.id);
        if (state.map_failed_tiles.find(key) != state.map_failed_tiles.end())
        {
            continue;
        }

        const auto tile = item.id;
        state.map_fetch_status =
            "Fetching " + snapshot.source_label + " z" +
            std::to_string(tile.z) + " " + std::to_string(tile.x) + "/" +
            std::to_string(tile.y);
        auto* model = &state.map_model;
        state.map_fetch_future =
            std::async(std::launch::async,
                       [model, tile]()
                       {
                           return model->ensureTile(tile);
                       });
        return;
    }
}

GtkWidget* buildTileCell(const MapTileItem& item, bool center)
{
    GtkWidget* cell = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_widget_add_css_class(cell, "tile-cell");
    if (center)
    {
        gtk_widget_add_css_class(cell, "tile-center");
    }
    gtk_widget_set_size_request(cell, 156, 156);
    gtk_widget_set_hexpand(cell, TRUE);
    gtk_widget_set_vexpand(cell, TRUE);

    if (item.available)
    {
        GtkWidget* picture =
            gtk_picture_new_for_filename(item.path.string().c_str());
        gtk_picture_set_content_fit(GTK_PICTURE(picture),
                                    GTK_CONTENT_FIT_COVER);
        gtk_widget_set_size_request(picture, 148, 128);
        gtk_widget_set_hexpand(picture, TRUE);
        gtk_widget_set_vexpand(picture, TRUE);
        gtk_box_append(GTK_BOX(cell), picture);
    }
    else
    {
        gtk_widget_set_vexpand(cell, TRUE);
        gtk_box_append(GTK_BOX(cell), makeLabel("Queued", "row-title"));
    }

    const std::string meta = "z" + std::to_string(item.id.z) + " " +
                             std::to_string(item.id.x) + "/" +
                             std::to_string(item.id.y);
    gtk_box_append(GTK_BOX(cell), makeLabel(meta.c_str(), "row-meta"));
    return cell;
}

void refreshMap(GtkUConsoleAppState& state)
{
    const MapWorkspaceSnapshot snapshot = state.map_model.snapshot();
    maybeStartMapFetch(state, snapshot);

    setActiveMapSourceButton(
        state.map_source_osm,
        ::platform::linux_runtime::MapBaseSource::Osm,
        snapshot.source_label);
    setActiveMapSourceButton(
        state.map_source_terrain,
        ::platform::linux_runtime::MapBaseSource::Terrain,
        snapshot.source_label);
    setActiveMapSourceButton(
        state.map_source_satellite,
        ::platform::linux_runtime::MapBaseSource::Satellite,
        snapshot.source_label);

    std::string title = snapshot.source_label + " map";
    title += " / z" + std::to_string(snapshot.zoom);
    setLabel(state.map_title, title);

    if (snapshot.has_fix)
    {
        std::string meta = snapshot.fix_label + " / lat " +
                           std::to_string(snapshot.lat) + " / lon " +
                           std::to_string(snapshot.lon);
        if (snapshot.has_altitude)
        {
            meta += " / alt " + std::to_string(static_cast<int>(std::lround(snapshot.altitude_m))) +
                    " m";
        }
        if (snapshot.satellites > 0)
        {
            meta += " / sats " + std::to_string(snapshot.satellites);
        }
        setLabel(state.map_meta, meta);
    }
    else
    {
        setLabel(state.map_meta,
                 "Waiting for a configured GPS/NMEA source.");
    }

    clearGrid(state.map_grid);
    if (!snapshot.has_fix)
    {
        GtkWidget* empty = makeRowBox();
        gtk_box_append(GTK_BOX(empty), makeLabel("No map center", "row-title"));
        gtk_box_append(GTK_BOX(empty),
                       makeLabel("GPS/NMEA source is not available.",
                                 "row-meta"));
        gtk_grid_attach(GTK_GRID(state.map_grid), empty, 0, 0, 3, 1);
    }
    else
    {
        for (std::size_t index = 0; index < snapshot.tiles.size(); ++index)
        {
            const int col = static_cast<int>(index % 3U);
            const int row = static_cast<int>(index / 3U);
            gtk_grid_attach(GTK_GRID(state.map_grid),
                            buildTileCell(snapshot.tiles[index], index == 4U),
                            col,
                            row,
                            1,
                            1);
        }
    }

    setLabel(state.map_status,
             state.map_fetch_status.empty() ? "Map cache idle."
                                            : state.map_fetch_status);

    std::string cache = "Cache: " +
                        std::to_string(snapshot.cache_stats.cached_tiles) +
                        " tiles / " +
                        formatBytes(snapshot.cache_stats.total_bytes);
    if (snapshot.cache_stats.failed_tiles > 0)
    {
        cache += " / " + std::to_string(snapshot.cache_stats.failed_tiles) +
                 " failed";
    }
    cache += " / " + snapshot.cache_stats.root.string();
    setLabel(state.map_cache_status, cache);
}

void refreshUi(GtkUConsoleAppState& state)
{
    const UConsoleDashboardSnapshot dashboard = state.dashboard_model.snapshot();
    const MapWorkspaceSnapshot map_snapshot = state.map_model.snapshot();
    setStatusChip(state.status_aio2, findHardware(dashboard, "AIO2"));
    setStatusChip(state.status_lora, findHardware(dashboard, "LoRa"));
    setStatusChip(state.status_gps, findHardware(dashboard, "GPS"));
    setLabel(state.status_node, "Node: " + dashboard.self_node);
    setLabel(state.status_unread,
             "Unread: " + std::to_string(dashboard.unread_count));
    refreshOverview(state, dashboard, map_snapshot);
    refreshCapabilities(state, dashboard);
    refreshHardwarePage(state, dashboard);
    refreshDataPage(state, dashboard, map_snapshot);
    refreshSettingsPage(state, dashboard, map_snapshot);
    refreshChat(state);
    refreshMap(state);
}

void onActivate(GtkApplication* app, gpointer data)
{
    auto& state = *static_cast<GtkUConsoleAppState*>(data);
    installCss();

    state.window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(state.window), state.options.title.c_str());
    gtk_window_set_default_size(GTK_WINDOW(state.window),
                                std::max(320, state.options.width),
                                std::max(240, state.options.height));
    g_signal_connect(state.window, "destroy", G_CALLBACK(onWindowDestroy),
                     &state);

    if (!state.services.initialize())
    {
        GtkWidget* error = makeLabel("Startup failed.", "empty-state");
        gtk_window_set_child(GTK_WINDOW(state.window), error);
    }
    else
    {
        gtk_window_set_child(GTK_WINDOW(state.window), buildRoot(state));
        state.refresh_source = g_timeout_add(500, onRefresh, &state);
    }

    if (state.options.fullscreen)
    {
        gtk_window_fullscreen(GTK_WINDOW(state.window));
    }
    gtk_window_present(GTK_WINDOW(state.window));
}

} // namespace

int runGtkUConsoleApp(GtkUConsoleOptions options)
{
    auto state = std::make_unique<GtkUConsoleAppState>(std::move(options));
    GtkApplication* app =
        gtk_application_new("dev.trailmate.uconsole", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(onActivate), state.get());

    const int status = g_application_run(G_APPLICATION(app), 0, nullptr);
    if (state->refresh_source != 0)
    {
        g_source_remove(state->refresh_source);
        state->refresh_source = 0;
    }
    if (state->map_fetch_future.valid())
    {
        state->map_fetch_future.wait();
    }
    state->services.shutdown();
    g_object_unref(app);
    return status;
}

} // namespace trailmate::uconsole::gtk
