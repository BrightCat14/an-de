#ifndef WM_H
#define WM_H

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>
#include <X11/Xutil.h>
#include <X11/Xft/Xft.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/shape.h>
#include <X11/cursorfont.h>
#include <fontconfig/fontconfig.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <stdarg.h>
#include <signal.h>

#define LOG_FILE "/tmp/notawm.log"
#define LOG_ERROR(...) log_msg("ERROR", __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...)  log_msg("WARN",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...)  log_msg("INFO",  __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DBG(...)   log_msg("DEBUG", __FILE__, __LINE__, __VA_ARGS__)

void log_msg(const char *lvl, const char *file, int line, const char *fmt, ...);

extern Display *dpy;
extern Window root;
extern int scr;
extern int sw, sh;
extern Visual *vis;
extern Colormap cmap;
extern int depth;

extern int refresh_rate;
extern int vsync_us;

extern Atom wm_del_win;
extern Atom wm_proto;
extern Atom wm_take_focus;
extern Atom net_wm_name;
 extern Atom net_wm_state;
 extern Atom net_wm_state_hidden;
 extern Atom net_wm_state_fs;
 extern Atom net_wm_state_add;
 extern Atom net_wm_state_remove;
 extern Atom net_wm_state_toggle;
extern Atom net_wm_type;
 extern Atom type_dock;
 extern Atom type_desk;
 extern Atom type_toolbar;
 extern Atom type_menu;
 extern Atom type_popup;
 extern Atom type_splash;
 extern Atom utf8_str;
extern Atom net_tray_opcode;

extern XftFont *font;
extern XftFont *font_small;
extern XftFont *font_bold;
extern XftColor clr_title;
extern XftColor clr_btn;
extern XftColor clr_bar;
extern XftColor clr_clock;
extern XftColor clr_menu;
extern XftColor clr_menu_hi;

extern unsigned long clr_border;
extern unsigned long clr_border_ac;
extern unsigned long clr_border_fc;
extern unsigned long clr_title_bg;
extern unsigned long clr_title_ac;
extern unsigned long clr_close;
extern unsigned long clr_min;
extern unsigned long clr_max;
extern unsigned long clr_topbar;
extern unsigned long clr_bottombar;
extern unsigned long clr_btn_hover;
extern unsigned long clr_menu_bg;
extern unsigned long clr_menu_hi_bg;

extern GC gc_bar;
extern GC gc_text;
extern GC gc_title;

 extern Cursor cur;
 extern Cursor cur_resize_tl, cur_resize_tr, cur_resize_bl, cur_resize_br;
 extern Cursor cur_resize_l, cur_resize_r, cur_resize_t, cur_resize_b;

extern int top_h;
extern int bot_h;

/* XShape extension for rounded buttons */
extern int shape_event_base;
extern int shape_error_base;
extern int shape_available;

/* XShape extension for rounded buttons */
extern int shape_event_base;
extern int shape_error_base;

/* Fullscreen menu tile layout */
#define TILE_SIZE 100
#define TILE_PADDING 10
#define TILE_COLS(screen_width) (((screen_width) - TILE_PADDING * 2) / (TILE_SIZE + TILE_PADDING))
#define TILE_ROWS(screen_height) (((screen_height) - top_h - TILE_PADDING * 2) / (TILE_SIZE + TILE_PADDING))
#define TILE_ICON_SIZE 64

typedef struct {
    char *name;
    char *exec;
    char *icon_path;
    char *comment;
    char *cat;
} App;

#define MAX_APP 512
extern App apps[MAX_APP];
extern int app_cnt;

XftColor mk_xft_clr(const char *hex);
void vsync(void);
void load_apps(void);
void launch(const char *exec);
void init_wm(void);
int err_handler(Display *d, XErrorEvent *e);
char* find_icon(const char *name);

#endif
