#pragma once

#include "ui/screens/gps/gps_page_shell.h"

namespace gps::ui::runtime
{

bool is_available();
void enter(const shell::Host* host, lv_obj_t* parent);
void exit(lv_obj_t* parent);
void remember_gps_view_state();
bool restore_gps_view_state();

} // namespace gps::ui::runtime
