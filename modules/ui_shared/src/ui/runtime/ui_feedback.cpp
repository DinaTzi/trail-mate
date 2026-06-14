#include "ui/runtime/ui_feedback.h"

#include "lvgl.h"
#include "ui/widgets/system_notification.h"

#include <cstring>
#include <new>

namespace ui::feedback
{
namespace
{

constexpr size_t kMaxNoticeTextBytes = 192;

struct PostedNotice
{
    char text[kMaxNoticeTextBytes];
    uint32_t duration_ms = 3000;
    Severity severity = Severity::Info;
};

class LvglSystemNotificationPresenter final : public IFeedbackPresenter
{
  public:
    void init() override
    {
        ::ui::SystemNotification::init();
    }

    void show_notice(const NoticeIntent& intent) override
    {
        ::ui::SystemNotification::show(intent.text, intent.duration_ms);
    }

    void hide_notice() override
    {
        ::ui::SystemNotification::hide();
    }
};

LvglSystemNotificationPresenter s_default_presenter;
IFeedbackPresenter* s_presenter = &s_default_presenter;
bool s_ready = false;

void copy_notice_text(char* out, size_t out_len, const char* text)
{
    if (!out || out_len == 0)
    {
        return;
    }
    std::strncpy(out, text ? text : "", out_len - 1);
    out[out_len - 1] = '\0';
}

IFeedbackPresenter& active_presenter()
{
    if (!s_presenter)
    {
        s_presenter = &s_default_presenter;
    }
    return *s_presenter;
}

void ensure_ready()
{
    if (s_ready)
    {
        return;
    }
    active_presenter().init();
    s_ready = true;
}

void show_notice_async(void* user_data)
{
    PostedNotice* payload = static_cast<PostedNotice*>(user_data);
    if (!payload)
    {
        return;
    }

    ensure_ready();
    NoticeIntent intent{};
    intent.text = payload->text;
    intent.duration_ms = payload->duration_ms;
    intent.severity = payload->severity;
    active_presenter().show_notice(intent);
    delete payload;
}

void hide_notice_async(void* /*user_data*/)
{
    ensure_ready();
    active_presenter().hide_notice();
}

} // namespace

void set_presenter(IFeedbackPresenter* presenter)
{
    s_presenter = presenter ? presenter : &s_default_presenter;
    s_ready = false;
}

IFeedbackPresenter& presenter()
{
    return active_presenter();
}

void init()
{
    ensure_ready();
}

bool is_ready()
{
    return s_ready;
}

bool show_notice(const NoticeIntent& intent)
{
    PostedNotice* payload = new (std::nothrow) PostedNotice{};
    if (!payload)
    {
        return false;
    }

    copy_notice_text(payload->text, sizeof(payload->text), intent.text);
    payload->duration_ms = intent.duration_ms;
    payload->severity = intent.severity;

    if (lv_async_call(show_notice_async, payload) != LV_RESULT_OK)
    {
        delete payload;
        return false;
    }
    return true;
}

bool show_notice(const char* text, uint32_t duration_ms)
{
    NoticeIntent intent{};
    intent.text = text ? text : "";
    intent.duration_ms = duration_ms;
    intent.severity = Severity::Info;
    return show_notice(intent);
}

bool hide_notice()
{
    return lv_async_call(hide_notice_async, nullptr) == LV_RESULT_OK;
}

} // namespace ui::feedback
