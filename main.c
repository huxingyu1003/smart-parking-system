#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_project_carpark_font_50.h"
#include "carnum.h"
#include "car_serve.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

lv_obj_t *main_screen;
lv_obj_t *forever_time_lable;
lv_obj_t *scr_car_entry;
lv_obj_t *scr_car_exit;
lv_obj_t *scr_park_all;
lv_obj_t *scr_set;
lv_obj_t *scr_reports;
lv_obj_t *net_scr;
int net_flag = 0;

lv_obj_t *obj_park[8] = {0};
lv_obj_t *label_park_car[8] = {0};
lv_obj_t *today_income_label;
lv_obj_t *today_carRate_label;

static lv_style_t st_bg;
static lv_style_t st_panel;
static lv_style_t st_title;
static lv_style_t st_text;
static lv_style_t st_text_big;
static lv_style_t st_btn;
static lv_style_t st_btn_red;
static lv_style_t st_btn_green;
static lv_style_t st_btn_yellow;
static lv_style_t st_key;
static lv_style_t st_net_red;
static lv_style_t st_net_green;

static char g_entry_plate[16];
static char g_exit_plate[16];
static char g_pending_plate[16];
static lv_obj_t *g_entry_plate_label;
static lv_obj_t *g_exit_plate_label;
static int g_sel_park = 0;
static lv_obj_t *g_entry_park_btns[8];

static lv_obj_t *g_popup = NULL;
static lv_obj_t *g_popup_msg = NULL;
static void (*g_popup_ok)(lv_event_t *e) = NULL;
static void (*g_popup_cancel)(lv_event_t *e) = NULL;
static lv_obj_t *net_online_label = NULL;

typedef struct
{
    char *buf;
    int size;
    lv_obj_t *label;
    void (*done)(void);
} kb_ctx_t;

static uint32_t tick_cb(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void theme_init(void)
{
    lv_style_init(&st_bg);
    lv_style_set_bg_color(&st_bg, lv_color_hex(0x10151f));
    lv_style_set_bg_opa(&st_bg, LV_OPA_COVER);

    lv_style_init(&st_panel);
    lv_style_set_bg_color(&st_panel, lv_color_hex(0x1d2536));
    lv_style_set_bg_opa(&st_panel, LV_OPA_COVER);
    lv_style_set_radius(&st_panel, 16);
    lv_style_set_border_color(&st_panel, lv_color_hex(0x35405c));
    lv_style_set_border_width(&st_panel, 2);

    lv_style_init(&st_title);
    lv_style_set_text_color(&st_title, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_title, &lv_project_carpark_font_50);

    lv_style_init(&st_text);
    lv_style_set_text_color(&st_text, lv_color_hex(0xe7ecf5));
    lv_style_set_text_font(&st_text, &lv_project_carpark_font_50);

    lv_style_init(&st_text_big);
    lv_style_set_text_color(&st_text_big, lv_color_hex(0xffd54f));
    lv_style_set_text_font(&st_text_big, &lv_project_carpark_font_50);

    lv_style_init(&st_btn);
    lv_style_set_bg_color(&st_btn, lv_color_hex(0x2b3650));
    lv_style_set_bg_opa(&st_btn, LV_OPA_COVER);
    lv_style_set_radius(&st_btn, 14);
    lv_style_set_border_color(&st_btn, lv_color_hex(0x4a5a80));
    lv_style_set_border_width(&st_btn, 2);
    lv_style_set_text_color(&st_btn, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_btn, &lv_project_carpark_font_50);

    lv_style_init(&st_btn_red);
    lv_style_set_bg_color(&st_btn_red, lv_color_hex(0x8a2a2a));
    lv_style_set_bg_opa(&st_btn_red, LV_OPA_COVER);
    lv_style_set_radius(&st_btn_red, 14);
    lv_style_set_text_color(&st_btn_red, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_btn_red, &lv_project_carpark_font_50);

    lv_style_init(&st_btn_green);
    lv_style_set_bg_color(&st_btn_green, lv_color_hex(0x1f6d33));
    lv_style_set_bg_opa(&st_btn_green, LV_OPA_COVER);
    lv_style_set_radius(&st_btn_green, 14);
    lv_style_set_text_color(&st_btn_green, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_btn_green, &lv_project_carpark_font_50);

    lv_style_init(&st_btn_yellow);
    lv_style_set_bg_color(&st_btn_yellow, lv_color_hex(0xb8860b));
    lv_style_set_bg_opa(&st_btn_yellow, LV_OPA_COVER);
    lv_style_set_radius(&st_btn_yellow, 14);
    lv_style_set_text_color(&st_btn_yellow, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_btn_yellow, &lv_project_carpark_font_50);

    lv_style_init(&st_key);
    lv_style_set_bg_color(&st_key, lv_color_hex(0x242c44));
    lv_style_set_bg_opa(&st_key, LV_OPA_COVER);
    lv_style_set_radius(&st_key, 10);
    lv_style_set_text_color(&st_key, lv_color_hex(0xffffff));
    lv_style_set_text_font(&st_key, &lv_project_carpark_font_50);

    lv_style_init(&st_net_red);
    lv_style_set_bg_color(&st_net_red, lv_color_hex(0xFF0000));
    lv_style_set_bg_opa(&st_net_red, LV_OPA_COVER);

    lv_style_init(&st_net_green);
    lv_style_set_bg_color(&st_net_green, lv_color_hex(0x008000));
    lv_style_set_bg_opa(&st_net_green, LV_OPA_COVER);
}

static lv_obj_t *make_btn(lv_obj_t *parent, int x, int y, int w, int h, const char *txt, lv_style_t *style)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    if(style != NULL) lv_obj_add_style(b, style, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_align(l, LV_ALIGN_CENTER);
    lv_label_set_text(l, txt);
    lv_obj_add_style(l, &st_text, 0);
    return b;
}

static lv_obj_t *make_label(lv_obj_t *parent, int x, int y, const char *txt, lv_style_t *style)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, txt);
    lv_obj_add_style(l, style == NULL ? &st_text : style, 0);
    return l;
}

static lv_obj_t *add_title(lv_obj_t *scr, const char *txt)
{
    lv_obj_t *l = lv_label_create(scr);
    lv_obj_set_pos(l, 0, 5);
    lv_obj_set_width(l, 1024);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(l, txt);
    lv_obj_add_style(l, &st_title, 0);
    return l;
}

static void back_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
        lv_screen_load(main_screen);
}

static lv_obj_t *add_back(lv_obj_t *scr)
{
    lv_obj_t *b = make_btn(scr, 10, 5, 130, 50, "返回", &st_btn);
    lv_obj_add_event_cb(b, back_cb, LV_EVENT_CLICKED, NULL);
    return b;
}

static void popup_close(void)
{
    if(g_popup != NULL)
    {
        lv_obj_delete(g_popup);
        g_popup = NULL;
    }
    g_popup_msg = NULL;
    g_popup_ok = NULL;
    g_popup_cancel = NULL;
}

static void popup_ok_cb(lv_event_t *e)
{
    void (*cb)(lv_event_t *) = g_popup_ok;
    popup_close();
    if(cb != NULL) cb(e);
}

static void popup_cancel_cb(lv_event_t *e)
{
    void (*cb)(lv_event_t *) = g_popup_cancel;
    popup_close();
    if(cb != NULL) cb(e);
}

static void popup_show(const char *msg, void (*ok_cb)(lv_event_t *),
                       void (*cancel_cb)(lv_event_t *))
{
    popup_close();

    lv_obj_t *ov = lv_obj_create(lv_screen_active());
    lv_obj_set_size(ov, 1024, 600);
    lv_obj_set_pos(ov, 0, 0);
    lv_obj_set_style_bg_color(ov, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(ov, LV_OPA_60, 0);

    lv_obj_t *panel = lv_obj_create(ov);
    lv_obj_set_size(panel, 560, 340);
    lv_obj_set_align(panel, LV_ALIGN_CENTER);
    lv_obj_add_style(panel, &st_panel, 0);

    g_popup_msg = lv_label_create(panel);
    lv_obj_set_align(g_popup_msg, LV_ALIGN_CENTER);
    lv_label_set_text(g_popup_msg, msg);
    lv_obj_add_style(g_popup_msg, &st_text, 0);

    lv_obj_t *ok = make_btn(panel, 0, 0, 170, 60, "确定", &st_btn_green);
    lv_obj_set_align(ok, LV_ALIGN_BOTTOM_LEFT);
    lv_obj_set_pos(ok, 70, -20);
    lv_obj_add_event_cb(ok, popup_ok_cb, LV_EVENT_CLICKED, NULL);
    g_popup_ok = ok_cb;

    if(cancel_cb != NULL)
    {
        lv_obj_t *cc = make_btn(panel, 0, 0, 170, 60, "取消", &st_btn_red);
        lv_obj_set_align(cc, LV_ALIGN_BOTTOM_RIGHT);
        lv_obj_set_pos(cc, -70, -20);
        lv_obj_add_event_cb(cc, popup_cancel_cb, LV_EVENT_CLICKED, NULL);
        g_popup_cancel = cancel_cb;
    }

    g_popup = ov;
}

static void net_ui_step(void)
{
    char msg[256];
    int online = 0;

    int has_msg = net_status_take(msg, sizeof(msg), &online);

    net_flag = (online > 0) ? 1 : 0;

    lv_obj_remove_style(net_scr, &st_net_red, 0);
    lv_obj_remove_style(net_scr, &st_net_green, 0);

    if(net_flag == 0)
        lv_obj_add_style(net_scr, &st_net_red, 0);
    else
        lv_obj_add_style(net_scr, &st_net_green, 0);

    if(net_online_label != NULL)
        lv_label_set_text_fmt(net_online_label, "在线:%d", online);

    if(has_msg)
        popup_show(msg, NULL, NULL);
}

static void kb_key_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    kb_ctx_t *ctx = (kb_ctx_t *)lv_event_get_user_data(e);
    lv_obj_t *lbl = lv_obj_get_child(lv_event_get_target(e), 0);
    const char *c = lv_label_get_text(lbl);
    int len = strlen(ctx->buf);

    if(strcmp(c, "DEL") == 0)
    {
        if(len > 0) ctx->buf[len - 1] = 0;
    }
    else if(strcmp(c, "CLR") == 0)
    {
        ctx->buf[0] = 0;
    }
    else if(strcmp(c, "OK") == 0)
    {
        if(ctx->done != NULL) ctx->done();
        return;
    }
    else
    {
        if(len < ctx->size - 1) strcat(ctx->buf, c);
    }
    lv_label_set_text(ctx->label, ctx->buf);
}

static lv_obj_t *kb_btn(lv_obj_t *parent, int x, int y, int w, int h, const char *txt, kb_ctx_t *ctx)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_pos(b, x, y);
    lv_obj_set_size(b, w, h);
    lv_obj_add_style(b, &st_key, 0);
    lv_obj_t *l = lv_label_create(b);
    lv_obj_set_align(l, LV_ALIGN_CENTER);
    lv_label_set_text(l, txt);
    lv_obj_add_style(l, &st_text, 0);
    lv_obj_add_event_cb(b, kb_key_cb, LV_EVENT_CLICKED, ctx);
    return b;
}

static void plate_keypad_create(lv_obj_t *parent, int x, int y, kb_ctx_t *ctx)
{
    static const char *provs[8] = {"湘","粤","京","沪","川","豫","鲁","冀"};

    for(int i = 0; i < 8; i++)
        kb_btn(parent, x + i * 66, y, 62, 48, provs[i], ctx);

    for(int i = 0; i < 13; i++)
    {
        char t[2] = {'A' + i, 0};
        kb_btn(parent, x + i * 40, y + 56, 36, 48, t, ctx);
    }
    for(int i = 0; i < 13; i++)
    {
        char t[2] = {'N' + i, 0};
        kb_btn(parent, x + i * 40, y + 112, 36, 48, t, ctx);
    }
    for(int i = 0; i < 10; i++)
    {
        char t[2] = {'0' + i, 0};
        kb_btn(parent, x + i * 51, y + 168, 47, 48, t, ctx);
    }

    kb_btn(parent, x, y + 224, 100, 48, "DEL", ctx);
    kb_btn(parent, x + 108, y + 224, 100, 48, "CLR", ctx);
    kb_btn(parent, x + 216, y + 224, 280, 48, "OK", ctx);
}

static void update_parking_view(void)
{
    carplatnum *pk;
    for(int i = 0; i < 8; i++)
    {
        pk = car_find_park(i + 1);
        if(pk != NULL)
        {
            lv_label_set_text_fmt(label_park_car[i], "%d\n%s", i + 1, pk->plat_num);
            lv_obj_set_style_bg_color(obj_park[i], lv_color_hex(0xff0000), 0);
        }
        else
        {
            lv_label_set_text_fmt(label_park_car[i], "%d", i + 1);
            lv_obj_set_style_bg_color(obj_park[i], lv_color_hex(0x00cc66), 0);
        }
    }
}

static void refresh_entry_park(void)
{
    for(int i = 0; i < 8; i++)
    {
        if(g_sel_park == i + 1)
        {
            lv_obj_set_style_border_color(g_entry_park_btns[i], lv_color_hex(0xffd54f), 0);
            lv_obj_set_style_border_width(g_entry_park_btns[i], 4, 0);
        }
        else
        {
            lv_obj_set_style_border_color(g_entry_park_btns[i], lv_color_hex(0x4a5a80), 0);
            lv_obj_set_style_border_width(g_entry_park_btns[i], 2, 0);
        }
    }
}

static void entry_confirm(void)
{
    if(g_entry_plate[0] == 0)
    {
        popup_show("请输入车牌", NULL, NULL);
        return;
    }

    int id = g_sel_park;
    if(id == 0) id = free_park_id();

    if(id < 1)
    {
        popup_show("车位已满", NULL, NULL);
        return;
    }

    if(car_find_plate(g_entry_plate) != NULL)
    {
        popup_show("该车已在场", NULL, NULL);
        return;
    }

    if(car_add(g_entry_plate, id) != 0)
    {
        popup_show("入场失败", NULL, NULL);
        return;
    }

    update_parking_view();
    refresh_entry_park();
    g_entry_plate[0] = 0;
    lv_label_set_text(g_entry_plate_label, "");
    g_sel_park = 0;
    popup_show("入场成功", NULL, NULL);
}

static void update_reports_view(void);

static void exit_confirm_cb(lv_event_t *e)
{
    car_del(g_pending_plate);
    update_parking_view();
    update_reports_view();
    g_exit_plate[0] = 0;
    lv_label_set_text(g_exit_plate_label, "");
    popup_show("出场完成", NULL, NULL);
}

static void exit_cancel_cb(lv_event_t *e)
{
    g_pending_plate[0] = 0;
}

static void exit_query(void)
{
    if(g_exit_plate[0] == 0)
    {
        popup_show("请输入车牌", NULL, NULL);
        return;
    }

    carplatnum *n = car_find_plate(g_exit_plate);
    if(n == NULL)
    {
        popup_show("未找到该车", NULL, NULL);
        return;
    }

    int ex[5];
    ex[0] = g_time.y; ex[1] = g_time.mo; ex[2] = g_time.d;
    ex[3] = g_time.h; ex[4] = g_time.mi;
    int fee = calc_fee_x100(n->time_entry, ex);

    strcpy(g_pending_plate, g_exit_plate);
    char msg[128];
    snprintf(msg, sizeof(msg), "车牌:%s\n车位:%d\n费用:%d.%02d元\n确认出场?",
             n->plat_num, n->car_Park_num, fee / 100, fee % 100);
    popup_show(msg, exit_confirm_cb, exit_cancel_cb);
}

static void update_reports_view(void)
{
    char buf[64];

    int fen = income_today_x100();
    snprintf(buf, sizeof(buf), "今日收入:%d.%02d元", fen / 100, fen % 100);
    lv_label_set_text(today_income_label, buf);

    int n = car_count();
    snprintf(buf, sizeof(buf), "在场:%d辆  利用率:%d%%", n, n * 100 / 8);
    lv_label_set_text(today_carRate_label, buf);
}

static void ui_time_update(void)
{
    if(forever_time_lable == NULL) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d",
             g_time.y, g_time.mo, g_time.d, g_time.h, g_time.mi, g_time.s);
    lv_label_set_text(forever_time_lable, buf);
}

static void main_entry_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_screen_load(scr_car_entry);
}

static void main_exit_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_screen_load(scr_car_exit);
}

static void main_park_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        update_parking_view();
        lv_screen_load(scr_park_all);
    }
}

static void main_report_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED)
    {
        update_reports_view();
        lv_screen_load(scr_reports);
    }
}

static void main_set_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) == LV_EVENT_CLICKED) lv_screen_load(scr_set);
}

static void init_main_interface(void)
{
    main_screen = lv_obj_create(NULL);
    lv_obj_add_style(main_screen, &st_bg, 0);

    forever_time_lable = lv_label_create(main_screen);
    lv_obj_set_align(forever_time_lable, LV_ALIGN_TOP_MID);
    lv_obj_add_style(forever_time_lable, &st_text_big, 0);

    net_scr = lv_obj_create(main_screen);
    lv_obj_set_align(net_scr, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_size(net_scr, 40, 40);
    lv_obj_set_style_radius(net_scr, LV_PCT(50), 0);
    lv_obj_set_style_clip_corner(net_scr, true, 0);
    lv_obj_set_style_pad_all(net_scr, 0, 0);
    if(net_flag == 0) lv_obj_add_style(net_scr, &st_net_red, 0);
    else if(net_flag == 1) lv_obj_add_style(net_scr, &st_net_green, 0);

    net_online_label = lv_label_create(main_screen);
    lv_obj_set_align(net_online_label, LV_ALIGN_TOP_RIGHT);
    lv_obj_set_pos(net_online_label, -100, 8);
    lv_obj_add_style(net_online_label, &st_text, 0);
    lv_label_set_text(net_online_label, "在线:0");

    const char *txt[5] = {"车辆入场","车辆出场","车位总览","统计报表","系统设置"};
    lv_align_t al[5] = {LV_ALIGN_LEFT_MID, LV_ALIGN_RIGHT_MID, LV_ALIGN_BOTTOM_LEFT, LV_ALIGN_BOTTOM_MID, LV_ALIGN_BOTTOM_RIGHT};
    lv_event_cb_t cbs[5] = {main_entry_cb, main_exit_cb, main_park_cb, main_report_cb, main_set_cb};
    int w[5] = {300, 300, 250, 250, 250};
    int h[5] = {165, 165, 120, 120, 120};
    for(int i = 0; i < 5; i++)
    {
        lv_obj_t *b = lv_button_create(main_screen);
        lv_obj_set_size(b, w[i], h[i]);
        lv_obj_set_align(b, al[i]);
        lv_obj_add_style(b, &st_btn, 0);
        lv_obj_t *l = lv_label_create(b);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        lv_label_set_text(l, txt[i]);
        lv_obj_add_style(l, &st_text, 0);
        lv_obj_add_event_cb(b, cbs[i], LV_EVENT_CLICKED, NULL);
    }
}

static void entry_park_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    g_sel_park = (int)(intptr_t)lv_event_get_user_data(e);
    refresh_entry_park();
}

static void init_car_entry_interface(void)
{
    scr_car_entry = lv_obj_create(NULL);
    lv_obj_add_style(scr_car_entry, &st_bg, 0);
    add_title(scr_car_entry, "车辆入场");
    add_back(scr_car_entry);

    for(int i = 0; i < 8; i++)
    {
        int px = 20 + (i % 2) * 185;
        int py = 70 + (i / 2) * 115;
        g_entry_park_btns[i] = lv_button_create(scr_car_entry);
        lv_obj_set_pos(g_entry_park_btns[i], px, py);
        lv_obj_set_size(g_entry_park_btns[i], 170, 100);
        lv_obj_add_style(g_entry_park_btns[i], &st_btn_green, 0);
        lv_obj_t *l = lv_label_create(g_entry_park_btns[i]);
        lv_obj_set_align(l, LV_ALIGN_CENTER);
        lv_label_set_text_fmt(l, "%d", i + 1);
        lv_obj_add_style(l, &st_text, 0);
        lv_obj_add_event_cb(g_entry_park_btns[i], entry_park_cb, LV_EVENT_CLICKED, (void *)(intptr_t)(i + 1));
    }

    g_entry_plate_label = make_label(scr_car_entry, 410, 70, "", &st_text_big);
    lv_obj_set_width(g_entry_plate_label, 580);

    static kb_ctx_t entry_ctx;
    entry_ctx.buf = g_entry_plate;
    entry_ctx.size = sizeof(g_entry_plate);
    entry_ctx.label = g_entry_plate_label;
    entry_ctx.done = entry_confirm;
    plate_keypad_create(scr_car_entry, 410, 130, &entry_ctx);

    refresh_entry_park();
}

static void init_car_exit_interface(void)
{
    scr_car_exit = lv_obj_create(NULL);
    lv_obj_add_style(scr_car_exit, &st_bg, 0);
    add_title(scr_car_exit, "车辆出场");
    add_back(scr_car_exit);

    g_exit_plate_label = make_label(scr_car_exit, 410, 70, "", &st_text_big);
    lv_obj_set_width(g_exit_plate_label, 580);

    static kb_ctx_t exit_ctx;
    exit_ctx.buf = g_exit_plate;
    exit_ctx.size = sizeof(g_exit_plate);
    exit_ctx.label = g_exit_plate_label;
    exit_ctx.done = exit_query;
    plate_keypad_create(scr_car_exit, 410, 130, &exit_ctx);

    make_label(scr_car_exit, 410, 470, "车牌号", &st_text);
}

static void park_click_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    carplatnum *n = car_find_park(id);
    char msg[128];
    if(n != NULL)
        snprintf(msg, sizeof(msg), "车位:%d\n车牌:%s\n入场:%04d-%02d-%02d %02d:%02d",
                 id, n->plat_num, n->time_entry[0], n->time_entry[1], n->time_entry[2],
                 n->time_entry[3], n->time_entry[4]);
    else
        snprintf(msg, sizeof(msg), "车位:%d\n空闲", id);
    popup_show(msg, NULL, NULL);
}

static void init_parking_interface(void)
{
    scr_park_all = lv_obj_create(NULL);
    lv_obj_add_style(scr_park_all, &st_bg, 0);
    add_title(scr_park_all, "车位总览");

    static const int pos[9][2] = {
        {40, 80},  {362, 80},  {684, 80},
        {40, 250}, {362, 250}, {684, 250},
        {40, 420}, {362, 420}, {684, 420}
    };

    for(int i = 0; i < 8; i++)
    {
        int slot;

        if(i < 7)
            slot = i;
        else
            slot = 8;

        obj_park[i] = lv_button_create(scr_park_all);
        lv_obj_set_size(obj_park[i], 300, 150);
        lv_obj_set_pos(obj_park[i], pos[slot][0], pos[slot][1]);
        lv_obj_add_style(obj_park[i], &st_btn_green, 0);

        label_park_car[i] = lv_label_create(obj_park[i]);
        lv_obj_set_align(label_park_car[i], LV_ALIGN_CENTER);
        lv_obj_add_style(label_park_car[i], &st_text, 0);

        lv_obj_add_event_cb(obj_park[i], park_click_cb, LV_EVENT_CLICKED,
                            (void *)(intptr_t)(i + 1));
    }
    lv_obj_t *back = make_btn(scr_park_all, pos[7][0], pos[7][1],
                              300, 150, "返回", &st_btn_yellow);
    lv_obj_add_event_cb(back, back_cb, LV_EVENT_CLICKED, NULL);

    update_parking_view();
}

static lv_obj_t *roller_year, *roller_mon, *roller_day, *roller_hour, *roller_min, *roller_sec;

static void roller_set(lv_obj_t *r, int mn, int mx, int val)
{
    char buf[1024] = {0};
    for(int i = mn; i <= mx; i++)
    {
        char t[16];
        snprintf(t, sizeof(t), "%d\n", i);
        strcat(buf, t);
    }
    lv_roller_set_options(r, buf, LV_ROLLER_MODE_NORMAL);
    lv_roller_set_selected(r, val - mn, LV_ANIM_OFF);
}

static void save_time_cb(lv_event_t *e)
{
    if(lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    g_time.y = 2025 + lv_roller_get_selected(roller_year);
    g_time.mo = 1 + lv_roller_get_selected(roller_mon);
    g_time.d = 1 + lv_roller_get_selected(roller_day);
    g_time.h = lv_roller_get_selected(roller_hour);
    g_time.mi = lv_roller_get_selected(roller_min);
    g_time.s = lv_roller_get_selected(roller_sec);
    file_forsave();   /* 时间修改也立即落盘（内部会顺带做跨天清零） */
    popup_show("时间已设置", NULL, NULL);
}

static void init_settings_interface(void)
{
    scr_set = lv_obj_create(NULL);
    lv_obj_add_style(scr_set, &st_bg, 0);
    add_title(scr_set, "系统设置");
    add_back(scr_set);

    make_label(scr_set, 120, 90, "调整时间", &st_title);

    roller_year = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_year, 40, 180);
    lv_obj_set_size(roller_year, 150, 200);
    lv_obj_add_style(roller_year, &st_panel, 0);
    lv_obj_set_style_text_font(roller_year, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_year, 2025, 2035, g_time.y);

    roller_mon = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_mon, 200, 180);
    lv_obj_set_size(roller_mon, 130, 200);
    lv_obj_add_style(roller_mon, &st_panel, 0);
    lv_obj_set_style_text_font(roller_mon, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_mon, 1, 12, g_time.mo);

    roller_day = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_day, 340, 180);
    lv_obj_set_size(roller_day, 130, 200);
    lv_obj_add_style(roller_day, &st_panel, 0);
    lv_obj_set_style_text_font(roller_day, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_day, 1, 31, g_time.d);

    roller_hour = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_hour, 480, 180);
    lv_obj_set_size(roller_hour, 130, 200);
    lv_obj_add_style(roller_hour, &st_panel, 0);
    lv_obj_set_style_text_font(roller_hour, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_hour, 0, 23, g_time.h);

    roller_min = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_min, 620, 180);
    lv_obj_set_size(roller_min, 130, 200);
    lv_obj_add_style(roller_min, &st_panel, 0);
    lv_obj_set_style_text_font(roller_min, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_min, 0, 59, g_time.mi);

    roller_sec = lv_roller_create(scr_set);
    lv_obj_set_pos(roller_sec, 760, 180);
    lv_obj_set_size(roller_sec, 130, 200);
    lv_obj_add_style(roller_sec, &st_panel, 0);
    lv_obj_set_style_text_font(roller_sec, &lv_project_carpark_font_50, LV_PART_SELECTED);
    roller_set(roller_sec, 0, 59, g_time.s);

    lv_obj_t *b = make_btn(scr_set, 400, 460, 240, 70, "设置时间", &st_btn_green);
    lv_obj_add_event_cb(b, save_time_cb, LV_EVENT_CLICKED, NULL);
}

static void init_reports_interface(void)
{
    scr_reports = lv_obj_create(NULL);
    lv_obj_add_style(scr_reports, &st_bg, 0);
    add_title(scr_reports, "统计报表");
    add_back(scr_reports);

    lv_obj_t *p1 = lv_obj_create(scr_reports);
    lv_obj_set_pos(p1, 60, 100);
    lv_obj_set_size(p1, 420, 140);
    lv_obj_add_style(p1, &st_panel, 0);
    today_income_label = lv_label_create(p1);
    lv_obj_set_align(today_income_label, LV_ALIGN_CENTER);
    lv_obj_add_style(today_income_label, &st_text_big, 0);

    lv_obj_t *p2 = lv_obj_create(scr_reports);
    lv_obj_set_pos(p2, 540, 100);
    lv_obj_set_size(p2, 420, 140);
    lv_obj_add_style(p2, &st_panel, 0);
    today_carRate_label = lv_label_create(p2);
    lv_obj_set_align(today_carRate_label, LV_ALIGN_CENTER);
    lv_obj_add_style(today_carRate_label, &st_text, 0);

    update_reports_view();
}

int main(void)
{
    lv_init();
    lv_tick_set_cb(tick_cb);
    lv_display_t *disp = lv_linux_fbdev_create();
    if(disp == NULL)
    {
        printf("FBDEV CREATE FAIL\n");
        return -1;
    }
    lv_linux_fbdev_set_file(disp, "/dev/fb0");
    lv_indev_t *indev = lv_evdev_create(LV_INDEV_TYPE_POINTER, "/dev/input/event6");
    g_time.y = 2025; g_time.mo = 1; g_time.d = 1;
    g_time.h = 8; g_time.mi = 0; g_time.s = 0;
    file_load();   /* 恢复断电前数据；无文件或损坏时保持上面的默认值 */
    theme_init();
    init_main_interface();
    init_car_entry_interface();
    init_car_exit_interface();
    init_parking_interface();
    init_settings_interface();
    init_reports_interface();
    lv_screen_load(main_screen);
    lv_refr_now(NULL);
    ui_time_update();

    pthread_t net_tid;
    if(pthread_create(&net_tid, NULL, net_serve, NULL) != 0)
    {
        perror("pthread_create net_serve error");
    }
    else
    {
        pthread_detach(net_tid);
    }

    uint32_t last = lv_tick_get();
    while(1)
    {
        lv_timer_handler();
        uint32_t now = lv_tick_get();
        if(now - last >= 1000)
        {
            last = now;
            time_tick();
            ui_time_update();
            net_ui_step();
        }
        usleep(5000);
    }
    return 0;
}