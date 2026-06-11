#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include "platform/wayland/wayland_presenter.h"
#include "ui/shell_ui_runner.h"

namespace
{

std::chrono::milliseconds auto_exit_after()
{
    if (const char* env = std::getenv("TRAIL_MATE_AUTO_EXIT_MS"))
    {
        if (env[0] != '\0')
        {
            return std::chrono::milliseconds(std::strtoul(env, nullptr, 10));
        }
    }
    return std::chrono::milliseconds::zero();
}

} // namespace

int main()
{
    try
    {
        trailmate::cardputer_zero::platform::wayland::WaylandPresenter
            presenter;
        trailmate::cardputer_zero::linux_ui::runShellUi(
            presenter,
            auto_exit_after());
        return 0;
    }
    catch (const std::exception& error)
    {
        std::fprintf(stderr,
                     "trailmate-cardputer-zero-wayland: %s\n",
                     error.what());
        return 1;
    }
}
