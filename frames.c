#include "frames.h"
#include "panels.h"

Frame frames[MAX_FRAME];
int frame_cnt = 0;

Bool drag = False;
Window drag_win = None;
int drag_sx, drag_sy, win_sx, win_sy;

pid_t get_pid(Window w) {
    Atom at = XInternAtom(dpy, "_NET_WM_PID", False), type;
    int fmt;
    unsigned long n, a;
    unsigned char *data = NULL;
    pid_t pid = 0;
    if (XGetWindowProperty(dpy, w, at, 0, 1, False, XA_CARDINAL, &type, &fmt, &n, &a, &data) == Success && data) {
        pid = *(pid_t*)data;
        XFree(data);
    }
    return pid;
}

char* get_title(Window w) {
    char *t = NULL;
    Atom type; int fmt; unsigned long n, a; unsigned char *d = NULL;
    XWindowAttributes attr;
    if (!XGetWindowAttributes(dpy, w, &attr)) return strdup("");
    if (XGetWindowProperty(dpy, w, net_wm_name, 0, 1024, False, utf8_str, &type, &fmt, &n, &a, &d) == Success && d) {
        t = strdup((char*)d); XFree(d); return t;
    }
    if (XGetWindowProperty(dpy, w, XA_WM_NAME, 0, 1024, False, XA_STRING, &type, &fmt, &n, &a, &d) == Success && d) {
        t = strdup((char*)d); XFree(d); return t;
    }
    return strdup("");
}

void draw_title(Frame *f) {
    if (f->fullscreen) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, f->title, &a)) return;
    XClearWindow(dpy, f->title);
    if (!f->title_draw) f->title_draw = XftDrawCreate(dpy, f->title, vis, cmap);
    XSetForeground(dpy, gc_title, f->focused ? clr_title_ac : clr_title_bg);
    XFillRectangle(dpy, f->title, gc_title, 0, 0, f->w, 28);
    if (f->title_text && strlen(f->title_text)) {
        char dt[256];
        snprintf(dt, sizeof(dt), "%s", f->title_text);
        if (strlen(dt) > 40) strcpy(dt + 37, "...");
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font, (FcChar8*)dt, strlen(dt), &e);
        int tx = (f->w - e.width) / 2, ty = (28 + e.height) / 2;
        XftDrawStringUtf8(f->title_draw, &clr_title, font, tx, ty, (FcChar8*)dt, strlen(dt));
    }
    if (XGetWindowAttributes(dpy, f->btn_close, &a)) {
        XSetForeground(dpy, gc_title, clr_close);
        XFillRectangle(dpy, f->btn_close, gc_title, 0, 0, 18, 18);
        XMoveWindow(dpy, f->btn_close, f->w - 24, 5);
        XftDraw *d = XftDrawCreate(dpy, f->btn_close, vis, cmap);
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font_bold, (FcChar8*)"x", 1, &e);
        XftDrawStringUtf8(d, &clr_btn, font_bold, (18-e.width)/2, (18+e.height)/2, (FcChar8*)"x", 1);
        XftDrawDestroy(d);
    }
    if (XGetWindowAttributes(dpy, f->btn_max, &a)) {
        XSetForeground(dpy, gc_title, clr_max);
        XFillRectangle(dpy, f->btn_max, gc_title, 0, 0, 18, 18);
        XMoveWindow(dpy, f->btn_max, f->w - 48, 5);
        XftDraw *d = XftDrawCreate(dpy, f->btn_max, vis, cmap);
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font_bold, (FcChar8*)"[]", 2, &e);
        XftDrawStringUtf8(d, &clr_btn, font_bold, (18-e.width)/2, (18+e.height)/2, (FcChar8*)"[]", 2);
        XftDrawDestroy(d);
    }
    if (XGetWindowAttributes(dpy, f->btn_min, &a)) {
        XSetForeground(dpy, gc_title, clr_min);
        XFillRectangle(dpy, f->btn_min, gc_title, 0, 0, 18, 18);
        XMoveWindow(dpy, f->btn_min, f->w - 72, 5);
        XftDraw *d = XftDrawCreate(dpy, f->btn_min, vis, cmap);
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font_bold, (FcChar8*)"-", 1, &e);
        XftDrawStringUtf8(d, &clr_btn, font_bold, (18-e.width)/2, (18+e.height)/2, (FcChar8*)"-", 1);
        XftDrawDestroy(d);
    }
}

void focus(Window w) {
    if (w == None || w == root) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, w, &a)) return;
    Frame *tf = NULL;
    for (int i = 0; i < frame_cnt; i++) {
        if (frames[i].frame == w || frames[i].client == w || frames[i].title == w ||
            frames[i].btn_close == w || frames[i].btn_min == w || frames[i].btn_max == w) {
            tf = &frames[i]; break;
        }
    }
    if (!tf) return;
    if (!XGetWindowAttributes(dpy, tf->client, &a)) return;
    XWMHints *h = XGetWMHints(dpy, tf->client);
    Bool acc = True;
    if (h && (h->flags & InputHint) && !h->input) acc = False;
    if (h) XFree(h);
    for (int i = 0; i < frame_cnt; i++) {
        if (frames[i].focused) {
            frames[i].focused = False;
            if (XGetWindowAttributes(dpy, frames[i].frame, &a))
                XSetWindowBorder(dpy, frames[i].frame, clr_border);
            draw_title(&frames[i]);
        }
    }
    tf->focused = True;
    if (acc) XSetInputFocus(dpy, tf->client, RevertToPointerRoot, CurrentTime);
    if (XGetWindowAttributes(dpy, tf->frame, &a))
        XSetWindowBorder(dpy, tf->frame, clr_border_fc);
    draw_title(tf);
    XRaiseWindow(dpy, tf->frame);
    Atom na = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    XChangeProperty(dpy, root, na, XA_WINDOW, 32, PropModeReplace, (unsigned char*)&tf->client, 1);
    update_bar();
}

void close_win(Window w) {
    LOG_INFO("Close win 0x%lx", w);
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, w, &a)) return;
    pid_t pid = get_pid(w);
    Atom *prot = NULL; int np = 0; Bool sup = False;
    if (XGetWMProtocols(dpy, w, &prot, &np)) {
        for (int i = 0; i < np; i++) if (prot[i] == wm_del_win) { sup = True; break; }
        XFree(prot);
    }
    if (sup) {
        XEvent ev; memset(&ev, 0, sizeof(ev));
        ev.type = ClientMessage;
        ev.xclient.window = w;
        ev.xclient.message_type = wm_proto;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = wm_del_win;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, w, False, NoEventMask, &ev);
        XSync(dpy, False);
        usleep(500000);
        if (XGetWindowAttributes(dpy, w, &a)) {
            LOG_WARN("Win didn't close, destroying window");
            XDestroyWindow(dpy, w);
            XSync(dpy, False);
        }
    } else {
        XDestroyWindow(dpy, w);
        XSync(dpy, False);
    }
}

void minimize(Frame *f) {
    if (!f) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, f->frame, &a)) return;
    if (f->minimized) {
        XMapWindow(dpy, f->frame);
        if (XGetWindowAttributes(dpy, f->client, &a)) XMapWindow(dpy, f->client);
        f->mapped = True; f->minimized = False;
        focus(f->client);
    } else {
        XUnmapWindow(dpy, f->frame);
        f->mapped = False; f->minimized = True;
    }
    update_bar();
}

void toggle_fs(Frame *f) {
    if (!f) return;
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, f->frame, &a)) return;
    if (!f->fullscreen) {
        f->saved_x = f->x; f->saved_y = f->y; f->saved_w = f->w; f->saved_h = f->h;
        XUnmapWindow(dpy, f->title);
        XSetWindowBorderWidth(dpy, f->frame, 0);
        XMoveResizeWindow(dpy, f->frame, 0, 0, sw, sh);
        if (XGetWindowAttributes(dpy, f->client, &a))
            XMoveResizeWindow(dpy, f->client, 0, 0, sw, sh);
        f->fullscreen = True; f->x = 0; f->y = 0; f->w = sw; f->h = sh;
        long d[] = {net_wm_state_fs, 0, 0, 0, 0};
        XChangeProperty(dpy, f->client, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)d, 1);
    } else {
        XMapWindow(dpy, f->title);
        XSetWindowBorderWidth(dpy, f->frame, 3);
        XMoveResizeWindow(dpy, f->frame, f->saved_x, f->saved_y, f->saved_w, f->saved_h);
        if (XGetWindowAttributes(dpy, f->client, &a))
            XMoveResizeWindow(dpy, f->client, 0, 28, f->saved_w, f->saved_h - 28);
        f->fullscreen = False; f->x = f->saved_x; f->y = f->saved_y; f->w = f->saved_w; f->h = f->saved_h;
        long d[] = {0, 0, 0, 0, 0};
        XChangeProperty(dpy, f->client, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)d, 0);
        draw_title(f);
    }
    XRaiseWindow(dpy, f->frame);
    focus(f->client);
}

Bool need_frame(Window w) {
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, w, &a)) return False;
    if (a.override_redirect) return False;
    Atom type; int fmt; unsigned long n, ab; unsigned char *d = NULL;
    if (XGetWindowProperty(dpy, w, net_wm_type, 0, 1, False, XA_ATOM, &type, &fmt, &n, &ab, &d) == Success && d) {
        Atom *t = (Atom*)d;
        if (n > 0 && (t[0] == type_dock || t[0] == type_desk)) { XFree(d); return False; }
        XFree(d);
    }
    return True;
}

Window mk_frame(Window c) {
    XWindowAttributes a;
    if (!XGetWindowAttributes(dpy, c, &a)) return None;
    int fx = a.x, fy = a.y + top_h, fw = a.width, fh = a.height + 28;
    if (fy + fh > sh - bot_h) fy = sh - bot_h - fh;
    Window f = XCreateSimpleWindow(dpy, root, fx, fy, fw, fh, 3, clr_border, clr_border);
    Window t = XCreateSimpleWindow(dpy, f, 0, 0, fw, 28, 0, clr_title_bg, clr_title_bg);
    Window bc = XCreateSimpleWindow(dpy, t, fw - 24, 5, 18, 18, 0, clr_close, clr_close);
    Window bm = XCreateSimpleWindow(dpy, t, fw - 48, 5, 18, 18, 0, clr_max, clr_max);
    Window bn = XCreateSimpleWindow(dpy, t, fw - 72, 5, 18, 18, 0, clr_min, clr_min);
    frames[frame_cnt].frame = f; frames[frame_cnt].client = c; frames[frame_cnt].title = t;
    frames[frame_cnt].btn_close = bc; frames[frame_cnt].btn_max = bm; frames[frame_cnt].btn_min = bn;
    frames[frame_cnt].x = fx; frames[frame_cnt].y = fy; frames[frame_cnt].w = fw; frames[frame_cnt].h = fh;
    frames[frame_cnt].mapped = False; frames[frame_cnt].focused = False;
    frames[frame_cnt].minimized = False; frames[frame_cnt].fullscreen = False;
    frames[frame_cnt].title_text = get_title(c); frames[frame_cnt].title_draw = NULL;
    frame_cnt++;
    XReparentWindow(dpy, c, f, 0, 28);
    XSetWindowBorderWidth(dpy, c, 0);
    XSelectInput(dpy, f, SubstructureRedirectMask | SubstructureNotifyMask |
                 ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
                 ExposureMask | EnterWindowMask | StructureNotifyMask);
    XSelectInput(dpy, t, ExposureMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | EnterWindowMask);
    XSelectInput(dpy, bc, ExposureMask | ButtonPressMask);
    XSelectInput(dpy, bm, ExposureMask | ButtonPressMask);
    XSelectInput(dpy, bn, ExposureMask | ButtonPressMask);
    XSelectInput(dpy, c, PropertyChangeMask | StructureNotifyMask | FocusChangeMask);
    XDefineCursor(dpy, f, cur); XDefineCursor(dpy, t, cur);
    XDefineCursor(dpy, bc, cur); XDefineCursor(dpy, bm, cur); XDefineCursor(dpy, bn, cur); XDefineCursor(dpy, c, cur);
    long d[] = {1, 0, 0, 0, 0};
    XChangeProperty(dpy, c, net_wm_state, XA_ATOM, 32, PropModeReplace, (unsigned char*)d, 2);
    XMapWindow(dpy, t); XMapWindow(dpy, bc); XMapWindow(dpy, bm); XMapWindow(dpy, bn);
    draw_title(&frames[frame_cnt - 1]);
    add_btn(c);
    LOG_INFO("Frame 0x%lx for 0x%lx", f, c);
    return f;
}

void rm_frame(Window f) {
    LOG_INFO("Removing frame 0x%lx", f);
    for (int i = 0; i < frame_cnt; i++) {
        if (frames[i].frame == f) {
            Window c = frames[i].client;
            rm_btn(c);
            if (frames[i].title_draw) XftDrawDestroy(frames[i].title_draw);
            XWindowAttributes a;
            if (XGetWindowAttributes(dpy, frames[i].btn_close, &a)) XDestroyWindow(dpy, frames[i].btn_close);
            if (XGetWindowAttributes(dpy, frames[i].btn_max, &a)) XDestroyWindow(dpy, frames[i].btn_max);
            if (XGetWindowAttributes(dpy, frames[i].btn_min, &a)) XDestroyWindow(dpy, frames[i].btn_min);
            if (XGetWindowAttributes(dpy, frames[i].title, &a)) XDestroyWindow(dpy, frames[i].title);
            if (XGetWindowAttributes(dpy, c, &a)) { XUnmapWindow(dpy, c); XReparentWindow(dpy, c, root, frames[i].x, frames[i].y); }
            if (XGetWindowAttributes(dpy, f, &a)) XDestroyWindow(dpy, f);
            if (frames[i].title_text) free(frames[i].title_text);
            for (int j = i; j < frame_cnt - 1; j++) frames[j] = frames[j + 1];
            frame_cnt--;
            XSync(dpy, False);
            break;
        }
    }
}

Frame* find_frame(Window w) {
    for (int i = 0; i < frame_cnt; i++)
        if (frames[i].frame == w || frames[i].client == w || frames[i].title == w ||
            frames[i].btn_close == w || frames[i].btn_min == w || frames[i].btn_max == w)
            return &frames[i];
    return NULL;
}

void key_handler(XKeyEvent *e) {
    unsigned int st = e->state & ~(LockMask | Mod2Mask | Mod3Mask);
    KeySym ks = XkbKeycodeToKeysym(dpy, e->keycode, 0, 0);
    if (st & ShiftMask) ks = XkbKeycodeToKeysym(dpy, e->keycode, 0, 1);
    Frame *fc = NULL;
    for (int i = 0; i < frame_cnt; i++) if (frames[i].focused) { fc = &frames[i]; break; }
    if (ks == XK_F4 && st == Mod1Mask) { if (fc) close_win(fc->client); }
    else if ((ks == XK_Return && st == Mod1Mask) || ks == XK_F11) { if (fc) toggle_fs(fc); }
    else if (ks == XK_q && (st == Mod1Mask || st == Mod4Mask)) { LOG_INFO("Quit"); XCloseDisplay(dpy); exit(0); }
}

void btn_handler_frames(XButtonEvent *e) {
    Window p = e->window;
    Frame *f = find_frame(p);
    if (!f) return;
    if (p == f->btn_close) { close_win(f->client); return; }
    if (p == f->btn_max) { toggle_fs(f); return; }
    if (p == f->btn_min) { minimize(f); return; }
    focus(p);
    if (!f->fullscreen && (p == f->title || p == f->frame)) {
        drag = True; drag_win = f->frame;
        XWindowAttributes a;
        if (!XGetWindowAttributes(dpy, drag_win, &a)) { drag = False; return; }
        win_sx = a.x; win_sy = a.y; drag_sx = e->x_root; drag_sy = e->y_root;
        XRaiseWindow(dpy, drag_win);
        XGrabPointer(dpy, drag_win, False, PointerMotionMask | ButtonReleaseMask,
                     GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
    }
}

void motion_handler(XMotionEvent *e) {
    if (drag && drag_win != None) {
        int dx = e->x_root - drag_sx, dy = e->y_root - drag_sy;
        XWindowAttributes a;
        if (XGetWindowAttributes(dpy, drag_win, &a))
            XMoveWindow(dpy, drag_win, win_sx + dx, win_sy + dy);
        vsync();
        XSync(dpy, False);
    }
}

void release_handler(XButtonEvent *e) {
    if (e->button == Button1 && drag) {
        XUngrabPointer(dpy, CurrentTime);
        drag = False; drag_win = None;
    }
}
