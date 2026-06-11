#pragma once

#include <stdint.h>

typedef int16_t lv_coord_t;

#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1

struct lv_font_t
{
    uint8_t stub;
};

typedef void lv_obj_t;
typedef void lv_display_t;

extern const lv_font_t lv_font_montserrat_10;
extern const lv_font_t lv_font_montserrat_12;
extern const lv_font_t lv_font_montserrat_14;
extern const lv_font_t lv_font_montserrat_16;

lv_coord_t lv_display_get_physical_horizontal_resolution(lv_display_t* display);
lv_coord_t lv_display_get_physical_vertical_resolution(lv_display_t* display);
lv_coord_t lv_obj_get_width(lv_obj_t* obj);
lv_coord_t lv_obj_get_height(lv_obj_t* obj);
lv_obj_t* lv_screen_active();
