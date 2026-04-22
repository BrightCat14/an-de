#include "wm.h"
#include "frames.h"
#include "panels.h"

int main(void) {
    dpy = XOpenDisplay(NULL);
    if (!dpy) { fprintf(stderr, "Cannot open display\n"); return 1; }
    scr = DefaultScreen(dpy); root = RootWindow(dpy, scr);
    vis = DefaultVisual(dpy, scr); cmap = DefaultColormap(dpy, scr); depth = DefaultDepth(dpy, scr);
    sw = DisplayWidth(dpy, scr); sh = DisplayHeight(dpy, scr);
    FcInit();
    font = XftFontOpenName(dpy, scr, "Sans-10");
    font_small = XftFontOpenName(dpy, scr, "Sans-9");
    font_bold = XftFontOpenName(dpy, scr, "Sans Bold-10");
    XColor c, d; Colormap cm = DefaultColormap(dpy, scr);
    XAllocNamedColor(dpy, cm, "#555555", &c, &d); clr_border = c.pixel;
    XAllocNamedColor(dpy, cm, "#888888", &c, &d); clr_border_ac = c.pixel;
    XAllocNamedColor(dpy, cm, "#0078D4", &c, &d); clr_border_fc = c.pixel;
    XAllocNamedColor(dpy, cm, "#2D2D2D", &c, &d); clr_title_bg = c.pixel;
    XAllocNamedColor(dpy, cm, "#0078D4", &c, &d); clr_title_ac = c.pixel;
    XAllocNamedColor(dpy, cm, "#E81123", &c, &d); clr_close = c.pixel;
    XAllocNamedColor(dpy, cm, "#FFB900", &c, &d); clr_min = c.pixel;
    XAllocNamedColor(dpy, cm, "#00CC6A", &c, &d); clr_max = c.pixel;
    XAllocNamedColor(dpy, cm, "#1E1E1E", &c, &d); clr_topbar = c.pixel;
    XAllocNamedColor(dpy, cm, "#1A1A1A", &c, &d); clr_bottombar = c.pixel;
    XAllocNamedColor(dpy, cm, "#3A3A3A", &c, &d); clr_btn_hover = c.pixel;
    XAllocNamedColor(dpy, cm, "#2B2B2B", &c, &d); clr_menu_bg = c.pixel;
    XAllocNamedColor(dpy, cm, "#3C3C3C", &c, &d); clr_menu_hi_bg = c.pixel;
    clr_title = mk_xft_clr("#FFFFFF"); clr_btn = mk_xft_clr("#FFFFFF");
    clr_bar = mk_xft_clr("#FFFFFF"); clr_clock = mk_xft_clr("#00FF00");
    clr_menu = mk_xft_clr("#E0E0E0"); clr_menu_hi = mk_xft_clr("#FFFFFF");
    XGCValues gv; gc_bar = XCreateGC(dpy, root, 0, &gv); gc_text = XCreateGC(dpy, root, 0, &gv); gc_title = XCreateGC(dpy, root, 0, &gv);
    wm_proto = XInternAtom(dpy, "WM_PROTOCOLS", False); wm_del_win = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wm_take_focus = XInternAtom(dpy, "WM_TAKE_FOCUS", False); net_wm_name = XInternAtom(dpy, "_NET_WM_NAME", False);
    net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False); net_wm_state_hidden = XInternAtom(dpy, "_NET_WM_STATE_HIDDEN", False);
    net_wm_state_fs = XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False); net_wm_type = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    type_dock = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False); type_desk = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DESKTOP", False);
    utf8_str = XInternAtom(dpy, "UTF8_STRING", False); net_tray_opcode = XInternAtom(dpy, "_NET_SYSTEM_TRAY_OPCODE", False);
    XSetErrorHandler(err_handler);
    XSelectInput(dpy, root, SubstructureRedirectMask | SubstructureNotifyMask | FocusChangeMask | PropertyChangeMask | KeyPressMask);
    KeyCode kf4 = XKeysymToKeycode(dpy, XK_F4), kq = XKeysymToKeycode(dpy, XK_q);
    KeyCode kent = XKeysymToKeycode(dpy, XK_Return), kf11 = XKeysymToKeycode(dpy, XK_F11);
    XGrabKey(dpy, kf4, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, kq, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, kq, Mod4Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, kent, Mod1Mask, root, True, GrabModeAsync, GrabModeAsync);
    XGrabKey(dpy, kf11, 0, root, True, GrabModeAsync, GrabModeAsync);
    init_icons(); atexit(cleanup_icons);
    init_wm();
    mk_top(); mk_bot(); draw_top(); update_clock();
    XSync(dpy, False);
    printf("NotAWM Alpha started\n");
    XEvent ev; time_t last_clk = 0;
    while (1) {
        if (XPending(dpy) == 0) {
            time_t now = time(NULL);
            if (now - last_clk >= 1) { update_clock(); last_clk = now; }
            usleep(1000);
            continue;
        }
        XNextEvent(dpy, &ev);
        if (menu_vis && ev.type == MotionNotify && ev.xmotion.window == menu_win) {
            int nh = menu_off + (ev.xmotion.y - 5) / MENU_ITEM_H;
            if (nh >= app_cnt) nh = -1;
            if (nh != menu_hover) { menu_hover = nh; draw_menu(); }
            continue;
        }
        switch (ev.type) {
            case KeyPress: key_handler(&ev.xkey); break;
            case ButtonPress: btn_handler_panels(&ev.xbutton); btn_handler_frames(&ev.xbutton); raise_panels(); break;
            case MotionNotify: while (XCheckTypedEvent(dpy, MotionNotify, &ev)); motion_handler(&ev.xmotion); break;
            case ButtonRelease: release_handler(&ev.xbutton); break;
            case ClientMessage: client_msg(&ev.xclient); break;
            case Expose:
                if (ev.xexpose.window == top_win) draw_top();
                else if (ev.xexpose.window == bot_win) { update_bar(); update_clock(); }
                else if (ev.xexpose.window == clock_win) update_clock();
                else if (menu_vis && ev.xexpose.window == menu_win) draw_menu();
                else { Frame *f = find_frame(ev.xexpose.window); if (f) draw_title(f); }
                break;
            case MapRequest: {
                Window w = ev.xmaprequest.window;
                if (need_frame(w)) { Window f = mk_frame(w); XMapWindow(dpy, f); XMapWindow(dpy, w); focus(w); }
                else XMapWindow(dpy, w);
                raise_panels();
                break;
            }
            case ConfigureRequest: {
                XConfigureRequestEvent *e = &ev.xconfigurerequest;
                Frame *f = find_frame(e->window);
                if (f && f->fullscreen) break;
                XWindowChanges ch = {.x = e->x, .y = e->y + top_h, .width = e->width, .height = e->height + 28,
                                     .border_width = e->border_width, .sibling = e->above, .stack_mode = e->detail};
                if (f) {
                    XConfigureWindow(dpy, f->frame, e->value_mask, &ch);
                    XMoveResizeWindow(dpy, f->title, 0, 0, ch.width ? ch.width : f->w, 28);
                    ch.x = 0; ch.y = 28;
                    XConfigureWindow(dpy, e->window, e->value_mask & ~(CWX | CWY), &ch);
                    if (ch.width) f->w = ch.width; if (ch.height) f->h = ch.height;
                    draw_title(f);
                } else { ch.y = e->y + top_h; XConfigureWindow(dpy, e->window, e->value_mask, &ch); }
                raise_panels();
                break;
            }
            case DestroyNotify: {
                Window w = ev.xdestroywindow.window;
                Frame *f = find_frame(w);
                if (f) { rm_frame(f->frame); update_bar(); }
                raise_panels();
                break;
            }
            case PropertyNotify: {
                Window w = ev.xproperty.window;
                Atom p = ev.xproperty.atom;
                if (p == XA_WM_NAME || p == net_wm_name) {
                    Frame *f = find_frame(w);
                    if (f) { if (f->title_text) free(f->title_text); f->title_text = get_title(w); draw_title(f); }
                }
                break;
            }
        }
    }
    return 0;
}
