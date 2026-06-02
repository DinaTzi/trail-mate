#pragma once

#include <stdint.h>

typedef int16_t lv_coord_t;

struct lv_font_t
{
    uint8_t stub;
};

typedef void lv_display_t;

extern const lv_font_t lv_font_montserrat_14;

lv_coord_t lv_display_get_physical_horizontal_resolution(lv_display_t* display);
lv_coord_t lv_display_get_physical_vertical_resolution(lv_display_t* display);
