#ifndef PANELS_H
#define PANELS_H

#include "wm.h"
#include <Imlib2.h>

extern Window top_win;
extern Window bot_win;
extern Window tray_win;
extern Window clock_win;
extern Window menu_win;
extern Bool menu_vis;

#define MENU_ITEM_H 32
#define MENU_MAX 20
#define MENU_W 350
extern int menu_off;
extern int menu_hover;

typedef struct {
    Window btn;
    Window client;
    int x, w;
    XftDraw *draw;
    char *icon;
} Btn;

#define MAX_BTN 256
extern Btn btns[MAX_BTN];
extern int btn_cnt;

typedef struct {
    Window id;
    int x, y;
} TrayIcon;

#define MAX_TRAY 32
extern TrayIcon trays[MAX_TRAY];
extern int tray_cnt;

typedef struct {
    char *path;
    Imlib_Image img;
    Pixmap pix;
    Pixmap mask;
    int w, h;
} IconCache;

#define MAX_ICACHE 256
extern IconCache icache[MAX_ICACHE];
extern int icache_cnt;

void init_icons(void);
void cleanup_icons(void);
Pixmap load_icon(const char *path, int sz, Pixmap *mask);
void mk_top(void);
void draw_top(void);
void mk_bot(void);
void update_clock(void);
void update_bar(void);
void add_btn(Window c);
void rm_btn(Window c);
void draw_btn(Btn *b, Bool act);
void draw_menu(void);
void mk_menu(void);
void show_power(void);
void btn_handler_panels(XButtonEvent *e);
void client_msg(XClientMessageEvent *e);
void raise_panels(void);

#endif
