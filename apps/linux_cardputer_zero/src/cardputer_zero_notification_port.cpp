#include "cardputer_zero_notification_port.h"

#include <cstring>

namespace trailmate
{
namespace apps
{
namespace linux_cardputer_zero
{
namespace
{

constexpr FreedesktopNotificationsContract kContract = {
    "org.freedesktop.Notifications",
    "/org/freedesktop/Notifications",
    "org.freedesktop.Notifications",
    "Notify",
    "susssasa{sv}i",
    "u",
    "CloseNotification",
    "GetCapabilities",
    "GetServerInformation",
    "NotificationClosed",
    "ActionInvoked",
    "body",
    "actions",
    "persistence",
    "urgency",
    "desktop-entry",
    "transient",
    "category",
    "resident",
    "cardputer-zero/notifyd.sock",
    false,
};

const char* safeString(const char* value)
{
    return value != nullptr ? value : "";
}

std::uint8_t urgencyHint(NotificationUrgency urgency)
{
    switch (urgency)
    {
    case NotificationUrgency::Low:
        return 0;
    case NotificationUrgency::Critical:
        return 2;
    case NotificationUrgency::Normal:
    default:
        return 1;
    }
}

} // namespace

const FreedesktopNotificationsContract& CardputerZeroNotificationPort::contract() const
{
    return kContract;
}

FreedesktopNotifyCall
CardputerZeroNotificationPort::makeNotifyCall(const NotificationRequest& request) const
{
    FreedesktopNotifyCall call;
    call.contract = &contract();
    call.app_name = safeString(request.app_name);
    call.replaces_id = request.replaces_id;
    call.app_icon = safeString(request.app_icon);
    call.summary = safeString(request.summary);
    call.body = safeString(request.body);
    call.urgency_hint = urgencyHint(request.urgency);
    call.expire_timeout_ms = request.expire_timeout_ms;
    return call;
}

bool CardputerZeroNotificationPort::validate() const
{
    const auto& c = contract();
    return std::strcmp(c.bus_name, "org.freedesktop.Notifications") == 0 &&
           std::strcmp(c.object_path, "/org/freedesktop/Notifications") == 0 &&
           std::strcmp(c.interface_name, "org.freedesktop.Notifications") == 0 &&
           std::strcmp(c.notify_method, "Notify") == 0 &&
           std::strcmp(c.notify_signature, "susssasa{sv}i") == 0 &&
           std::strcmp(c.notify_return_signature, "u") == 0 &&
           std::strcmp(c.close_method, "CloseNotification") == 0 &&
           std::strcmp(c.get_capabilities_method, "GetCapabilities") == 0 &&
           std::strcmp(c.get_server_information_method, "GetServerInformation") == 0 &&
           std::strcmp(c.notification_closed_signal, "NotificationClosed") == 0 &&
           std::strcmp(c.action_invoked_signal, "ActionInvoked") == 0 &&
           std::strcmp(c.capability_body, "body") == 0 &&
           std::strcmp(c.capability_actions, "actions") == 0 &&
           std::strcmp(c.capability_persistence, "persistence") == 0 &&
           std::strcmp(c.hint_urgency, "urgency") == 0 &&
           std::strcmp(c.hint_desktop_entry, "desktop-entry") == 0 &&
           std::strcmp(c.hint_transient, "transient") == 0 &&
           std::strcmp(c.hint_category, "category") == 0 &&
           std::strcmp(c.hint_resident, "resident") == 0 &&
           std::strcmp(c.shell_ipc_socket_suffix, "cardputer-zero/notifyd.sock") == 0 &&
           !c.shell_ipc_creates_notifications;
}

} // namespace linux_cardputer_zero
} // namespace apps
} // namespace trailmate
