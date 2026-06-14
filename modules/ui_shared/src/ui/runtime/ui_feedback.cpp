#include "ui/runtime/ui_feedback.h"

#include "lvgl.h"
#include "ui/widgets/system_notification.h"

#include <atomic>
#include <cstddef>
#include <cstring>

namespace ui::feedback
{
namespace
{

constexpr size_t kMaxNoticeTextBytes = 192;
constexpr size_t kNoticeQueueCapacity = 8;
constexpr uint32_t kDrainPeriodMs = 20;

struct PostedNotice
{
    char text[kMaxNoticeTextBytes]{};
    uint32_t duration_ms = 3000;
    Severity severity = Severity::Info;
    bool hide = false;
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
lv_timer_t* s_drain_timer = nullptr;

PostedNotice s_queue[kNoticeQueueCapacity]{};
size_t s_queue_head = 0;
size_t s_queue_count = 0;
std::atomic_flag s_queue_lock = ATOMIC_FLAG_INIT;

class QueueLock
{
  public:
    QueueLock()
    {
        while (s_queue_lock.test_and_set(std::memory_order_acquire))
        {
        }
    }

    ~QueueLock()
    {
        s_queue_lock.clear(std::memory_order_release);
    }

    QueueLock(const QueueLock&) = delete;
    QueueLock& operator=(const QueueLock&) = delete;
};

void copy_notice_text(char* out, size_t out_len, const char* text)
{
    if (!out || out_len == 0)
    {
        return;
    }
    std::strncpy(out, text ? text : "", out_len - 1);
    out[out_len - 1] = '\0';
}

void enqueue_notice(const PostedNotice& notice)
{
    QueueLock lock;
    if (s_queue_count == kNoticeQueueCapacity)
    {
        s_queue_head = (s_queue_head + 1) % kNoticeQueueCapacity;
        --s_queue_count;
    }

    const size_t tail = (s_queue_head + s_queue_count) % kNoticeQueueCapacity;
    s_queue[tail] = notice;
    ++s_queue_count;
}

bool pop_notice(PostedNotice& out)
{
    QueueLock lock;
    if (s_queue_count == 0)
    {
        return false;
    }

    out = s_queue[s_queue_head];
    s_queue_head = (s_queue_head + 1) % kNoticeQueueCapacity;
    --s_queue_count;
    return true;
}

IFeedbackPresenter& active_presenter()
{
    if (!s_presenter)
    {
        s_presenter = &s_default_presenter;
    }
    return *s_presenter;
}

void ensure_presenter_ready()
{
    if (s_ready)
    {
        return;
    }
    active_presenter().init();
    s_ready = true;
}

void drain_queued_notices()
{
    PostedNotice payload{};
    while (pop_notice(payload))
    {
        ensure_presenter_ready();
        if (payload.hide)
        {
            active_presenter().hide_notice();
            continue;
        }

        NoticeIntent intent{};
        intent.text = payload.text;
        intent.duration_ms = payload.duration_ms;
        intent.severity = payload.severity;
        active_presenter().show_notice(intent);
    }
}

void drain_timer_cb(lv_timer_t* /*timer*/)
{
    drain_queued_notices();
}

void ensure_drain_timer()
{
    if (s_drain_timer)
    {
        return;
    }
    s_drain_timer = lv_timer_create(drain_timer_cb, kDrainPeriodMs, nullptr);
    if (s_drain_timer)
    {
        lv_timer_set_repeat_count(s_drain_timer, -1);
    }
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
    ensure_presenter_ready();
    ensure_drain_timer();
    drain_queued_notices();
}

bool is_ready()
{
    return s_ready;
}

bool show_notice(const NoticeIntent& intent)
{
    PostedNotice payload{};
    copy_notice_text(payload.text, sizeof(payload.text), intent.text);
    payload.duration_ms = intent.duration_ms;
    payload.severity = intent.severity;
    payload.hide = false;
    enqueue_notice(payload);
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
    PostedNotice payload{};
    payload.hide = true;
    enqueue_notice(payload);
    return true;
}

} // namespace ui::feedback
