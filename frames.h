#ifndef FRAMES_H
#define FRAMES_H

#include "wm.h"

typedef struct {
    Window frame;
    Window client;
    Window title;
    Window btn_close;
    Window btn_min;
    Window btn_max;
    int x, y, w, h;
    int saved_x, saved_y, saved_w, saved_h;
    Bool mapped;
    Bool focused;
    Bool minimized;
    Bool fullscreen;
    char *title_text;
    XftDraw *title_draw;
} Frame;

#define MAX_FRAME 256
extern Frame frames[MAX_FRAME];
extern int frame_cnt;

extern Bool drag;
extern Window drag_win;
extern int drag_sx, drag_sy;
extern int win_sx, win_sy;

 void draw_title(Frame *f);
 void focus(Window w);
 void close_win(Window w);
 void minimize(Frame *f);
 void toggle_fs(Frame *f);
 Window mk_frame(Window c);
 void rm_frame(Window f);
 Frame* find_frame(Window w);
 Bool need_frame(Window w);
 char* get_title(Window w);
 pid_t get_pid(Window w);
 void key_handler(XKeyEvent *e);
 void btn_handler_frames(XButtonEvent *e);
 void motion_handler(XMotionEvent *e);
 void release_handler(XButtonEvent *e);
 int get_resize_dir(Frame *f, int mx, int my);
 Cursor get_resize_cursor(int dir);

#endif
