#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "carnum.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern lv_obj_t *obj_park[8];
extern lv_obj_t *label_park_car[8];

static lv_style_t style_green;
static lv_style_t style_red;
static int style_inited = 0;

void color_cb_park(int flag, int a)
{
    if(a < 1 || a > 8)
    {
        return;
    }

    if(style_inited == 0)
    {
        style_inited = 1;
        lv_style_init(&style_green);
        lv_style_set_bg_color(&style_green, lv_color_hex(0x00FF00));
        lv_style_init(&style_red);
        lv_style_set_bg_color(&style_red, lv_color_hex(0xFF0000));
    }

    if(flag == 1)
    {
        lv_obj_add_style(obj_park[a - 1], &style_red, 0);
    }
    else if(flag == 0)
    {
        lv_obj_add_style(obj_park[a - 1], &style_green, 0);
    }
}