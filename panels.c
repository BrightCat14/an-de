#include "panels.h"
#include "frames.h"

Window top_win, bot_win, tray_win, clock_win, menu_win;
Bool menu_vis = False;
int menu_off = 0, menu_hover = -1;

Btn btns[MAX_BTN];
int btn_cnt = 0;

TrayIcon trays[MAX_TRAY];
int tray_cnt = 0;

IconCache icache[MAX_ICACHE];
int icache_cnt = 0;

char tstr[64], dstr[64];

void raise_panels(void) {
    XRaiseWindow(dpy, top_win);
    XRaiseWindow(dpy, bot_win);
}

void init_icons(void) {
    imlib_context_set_display(dpy);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cmap);
    imlib_context_set_drawable(root);
    for (int i = 0; i < MAX_ICACHE; i++) icache[i].path = NULL;
}

void cleanup_icons(void) {
    for (int i = 0; i < icache_cnt; i++) {
        if (icache[i].img) { imlib_context_set_image(icache[i].img); imlib_free_image(); }
        if (icache[i].pix != None) XFreePixmap(dpy, icache[i].pix);
        if (icache[i].mask != None) XFreePixmap(dpy, icache[i].mask);
        if (icache[i].path) free(icache[i].path);
    }
}

Pixmap load_icon(const char *path, int sz, Pixmap *mask) {
    if (!path || !strlen(path)) return None;
    for (int i = 0; i < icache_cnt; i++)
        if (icache[i].path && !strcmp(icache[i].path, path) && icache[i].w == sz) {
            *mask = icache[i].mask; return icache[i].pix;
        }
    Imlib_Image img = imlib_load_image(path);
    if (!img) return None;
    imlib_context_set_image(img);
    int ow = imlib_image_get_width(), oh = imlib_image_get_height();
    if (ow != sz || oh != sz) {
        Imlib_Image sc = imlib_create_cropped_scaled_image(0, 0, ow, oh, sz, sz);
        imlib_free_image(); img = sc; imlib_context_set_image(img);
    }
    Pixmap p = XCreatePixmap(dpy, root, sz, sz, depth);
    imlib_context_set_drawable(p);
    imlib_render_image_on_drawable(0, 0);
    if (icache_cnt < MAX_ICACHE) {
        icache[icache_cnt].path = strdup(path); icache[icache_cnt].img = img;
        icache[icache_cnt].pix = p; icache[icache_cnt].mask = None;
        icache[icache_cnt].w = sz; icache[icache_cnt].h = sz; icache_cnt++;
    } else imlib_free_image();
    *mask = None; return p;
}

void mk_top(void) {
    top_win = XCreateSimpleWindow(dpy, root, 0, 0, sw, top_h, 0, clr_topbar, clr_topbar);
    XSelectInput(dpy, top_win, ExposureMask | ButtonPressMask);
    XDefineCursor(dpy, top_win, cur);
    XSetWindowAttributes a; a.override_redirect = False;
    XChangeWindowAttributes(dpy, top_win, CWOverrideRedirect, &a);
    Atom td = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, top_win, net_wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&td, 1);
    Atom sp = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    unsigned long st[12] = {0, 0, top_h, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    XChangeProperty(dpy, top_win, sp, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)st, 12);
    XMapWindow(dpy, top_win);
}

void draw_top(void) {
    XClearWindow(dpy, top_win);
    XftDraw *d = XftDrawCreate(dpy, top_win, vis, cmap);
    XSetForeground(dpy, gc_bar, clr_btn_hover);
    XFillRectangle(dpy, top_win, gc_bar, 5, 3, 120, top_h - 6);
    XGlyphInfo e;
    char *txt = "Applications";
    XftTextExtentsUtf8(dpy, font, (FcChar8*)txt, strlen(txt), &e);
    XftDrawStringUtf8(d, &clr_bar, font, 15, (top_h + e.height) / 2, (FcChar8*)txt, strlen(txt));
    XSetForeground(dpy, gc_bar, clr_close);
    XFillRectangle(dpy, top_win, gc_bar, sw - 50, 3, 45, top_h - 6);
    txt = "O";
    XftTextExtentsUtf8(dpy, font_bold, (FcChar8*)txt, strlen(txt), &e);
    XftDrawStringUtf8(d, &clr_btn, font_bold, sw - 35, (top_h + e.height) / 2, (FcChar8*)txt, strlen(txt));
    XftDrawDestroy(d);
}

void mk_bot(void) {
    bot_win = XCreateSimpleWindow(dpy, root, 0, sh - bot_h, sw, bot_h, 0, clr_bottombar, clr_bottombar);
    XSelectInput(dpy, bot_win, ExposureMask | ButtonPressMask | SubstructureNotifyMask);
    XDefineCursor(dpy, bot_win, cur);
    XSetWindowAttributes a; a.override_redirect = False;
    XChangeWindowAttributes(dpy, bot_win, CWOverrideRedirect, &a);
    Atom td = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DOCK", False);
    XChangeProperty(dpy, bot_win, net_wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&td, 1);
    Atom sp = XInternAtom(dpy, "_NET_WM_STRUT_PARTIAL", False);
    unsigned long st[12] = {0, 0, 0, bot_h, 0, 0, 0, 0, 0, 0, 0, 0};
    XChangeProperty(dpy, bot_win, sp, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)st, 12);
    tray_win = XCreateSimpleWindow(dpy, bot_win, 5, 5, 200, bot_h - 10, 0, clr_bottombar, clr_bottombar);
    XSelectInput(dpy, tray_win, SubstructureNotifyMask);
    XMapWindow(dpy, tray_win);
    clock_win = XCreateSimpleWindow(dpy, bot_win, sw - 150, 5, 140, bot_h - 10, 0, clr_bottombar, clr_bottombar);
    XSelectInput(dpy, clock_win, ExposureMask);
    XMapWindow(dpy, clock_win);
    Atom nt = XInternAtom(dpy, "_NET_SYSTEM_TRAY_S0", False);
    XSetSelectionOwner(dpy, nt, tray_win, CurrentTime);
    XMapWindow(dpy, bot_win);
    printf("Bottombar created\n");
}

void update_clock(void) {
    time_t now; struct tm *tm;
    time(&now); tm = localtime(&now);
    strftime(tstr, sizeof(tstr), "%H:%M", tm);
    strftime(dstr, sizeof(dstr), "%d.%m", tm);
    XClearWindow(dpy, clock_win);
    XftDraw *d = XftDrawCreate(dpy, clock_win, vis, cmap);
    char ds[128];
    snprintf(ds, sizeof(ds), "%s %s", tstr, dstr);
    XGlyphInfo e;
    XftTextExtentsUtf8(dpy, font_small, (FcChar8*)ds, strlen(ds), &e);
    XftDrawStringUtf8(d, &clr_clock, font_small, 5, (bot_h - 10 + e.height) / 2, (FcChar8*)ds, strlen(ds));
    XftDrawDestroy(d);
}

void add_btn(Window c) {
    char *t = get_title(c);
    char *ic = "";
    for (int i = 0; i < app_cnt; i++) if (strcasestr(t, apps[i].name)) { ic = apps[i].icon_path; break; }
    int x = 220 + btn_cnt * 40;
    Window b = XCreateSimpleWindow(dpy, bot_win, x, 5, 36, bot_h - 10, 1, clr_topbar, clr_topbar);
    XSelectInput(dpy, b, ExposureMask | ButtonPressMask);
    XDefineCursor(dpy, b, cur);
    XMapWindow(dpy, b);
    btns[btn_cnt].btn = b; btns[btn_cnt].client = c; btns[btn_cnt].x = x; btns[btn_cnt].w = 36;
    btns[btn_cnt].draw = XftDrawCreate(dpy, b, vis, cmap);
    btns[btn_cnt].icon = strdup(ic); btn_cnt++;
    free(t);
}

void rm_btn(Window c) {
    for (int i = 0; i < btn_cnt; i++) {
        if (btns[i].client == c) {
            XftDrawDestroy(btns[i].draw);
            if (btns[i].icon) free(btns[i].icon);
            XDestroyWindow(dpy, btns[i].btn);
            for (int j = i; j < btn_cnt - 1; j++) {
                btns[j] = btns[j + 1];
                XMoveWindow(dpy, btns[j].btn, btns[j].x - 40, 5);
                btns[j].x = 220 + j * 40;
            }
            btn_cnt--; break;
        }
    }
}

void draw_btn(Btn *b, Bool act) {
    XClearWindow(dpy, b->btn);
    XSetForeground(dpy, gc_bar, act ? clr_title_ac : clr_topbar);
    XFillRectangle(dpy, b->btn, gc_bar, 0, 0, b->w, bot_h - 10);
    if (b->icon && strlen(b->icon)) {
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font, (FcChar8*)b->icon, strlen(b->icon), &e);
        XftDrawStringUtf8(b->draw, &clr_bar, font, (b->w - e.width) / 2, (bot_h - 10 + e.height) / 2, (FcChar8*)b->icon, strlen(b->icon));
    }
}

void update_bar(void) {
    for (int i = 0; i < btn_cnt; i++)
        for (int j = 0; j < frame_cnt; j++)
            if (frames[j].client == btns[i].client && !frames[j].minimized) {
                draw_btn(&btns[i], frames[j].focused); break;
            }
}

void mk_menu(void) {
    int vis = app_cnt < MENU_MAX ? app_cnt : MENU_MAX;
    int mh = vis * MENU_ITEM_H + 10;
    menu_win = XCreateSimpleWindow(dpy, root, 5, top_h + 5, MENU_W, mh, 2, clr_menu_bg, clr_menu_bg);
    XSelectInput(dpy, menu_win, ExposureMask | ButtonPressMask | PointerMotionMask | ButtonReleaseMask);
    XDefineCursor(dpy, menu_win, cur);
    Atom tp = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
    XChangeProperty(dpy, menu_win, net_wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&tp, 1);
}

void draw_menu(void) {
    XClearWindow(dpy, menu_win);
    XftDraw *d = XftDrawCreate(dpy, menu_win, vis, cmap);
    int vis = app_cnt < MENU_MAX ? app_cnt : MENU_MAX;
    int st = menu_off;
    for (int i = 0; i < vis && (st + i) < app_cnt; i++) {
        int idx = st + i, y = 5 + i * MENU_ITEM_H;
        if (idx == menu_hover) {
            XSetForeground(dpy, gc_bar, clr_menu_hi_bg);
            XFillRectangle(dpy, menu_win, gc_bar, 0, y, MENU_W - 12, MENU_ITEM_H);
        }
        Pixmap p = None, m = None;
        if (apps[idx].icon_path && strlen(apps[idx].icon_path))
            p = load_icon(apps[idx].icon_path, 24, &m);
        if (p != None) {
            if (m != None) { XSetClipMask(dpy, gc_bar, m); XSetClipOrigin(dpy, gc_bar, 8, y + (MENU_ITEM_H - 24) / 2); }
            XCopyArea(dpy, p, menu_win, gc_bar, 0, 0, 24, 24, 8, y + (MENU_ITEM_H - 24) / 2);
            if (m != None) XSetClipMask(dpy, gc_bar, None);
        }
        char dn[64];
        snprintf(dn, sizeof(dn), "%s", apps[idx].name);
        if (strlen(dn) > 35) strcpy(dn + 32, "...");
        XftColor tc = (idx == menu_hover) ? clr_title : clr_menu;
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font, (FcChar8*)dn, strlen(dn), &e);
        XftDrawStringUtf8(d, &tc, font, 40, y + (MENU_ITEM_H + e.height) / 2, (FcChar8*)dn, strlen(dn));
    }
    if (app_cnt > MENU_MAX) {
        XSetForeground(dpy, gc_bar, clr_title_bg);
        XFillRectangle(dpy, menu_win, gc_bar, MENU_W - 12, 0, 10, vis * MENU_ITEM_H + 10);
        int sh = (vis * MENU_ITEM_H) * vis / app_cnt; if (sh < 20) sh = 20;
        int sy = 5 + (menu_off * (vis * MENU_ITEM_H - sh) / (app_cnt - vis));
        XSetForeground(dpy, gc_bar, clr_title_ac);
        XFillRectangle(dpy, menu_win, gc_bar, MENU_W - 10, sy, 6, sh);
    }
    XftDrawDestroy(d);
}

void show_power(void) {
    Window pm = XCreateSimpleWindow(dpy, root, sw - 150, top_h + 5, 140, 120, 1, clr_topbar, clr_topbar);
    XSelectInput(dpy, pm, ExposureMask | ButtonPressMask);
    XDefineCursor(dpy, pm, cur);
    XMapWindow(dpy, pm);
    XftDraw *d = XftDrawCreate(dpy, pm, vis, cmap);
    char *opts[] = {"Lock", "Reboot", "Shutdown", "Logout"};
    int y = 10;
    for (int i = 0; i < 4; i++) {
        XSetForeground(dpy, gc_bar, clr_title_bg);
        XFillRectangle(dpy, pm, gc_bar, 0, y, 140, 25);
        XftDrawStringUtf8(d, &clr_bar, font, 10, y + 18, (FcChar8*)opts[i], strlen(opts[i]));
        y += 25;
    }
    XftDrawDestroy(d);
    XEvent ev;
    while (1) {
        XNextEvent(dpy, &ev);
        if (ev.type == ButtonPress) {
            int opt = (ev.xbutton.y - 10) / 25;
            if (opt == 0) system("loginctl lock-session &");
            else if (opt == 1) system("systemctl reboot &");
            else if (opt == 2) system("systemctl poweroff &");
            else if (opt == 3) exit(0);
            break;
        }
    }
    XDestroyWindow(dpy, pm);
}

void btn_handler_panels(XButtonEvent *e) {
    Window p = e->window;
    if (p == top_win) {
        if (e->x < 130) {
            if (!menu_vis) {
                menu_off = 0; menu_hover = -1;
                mk_menu(); XMapWindow(dpy, menu_win); draw_menu(); menu_vis = True;
            } else {
                XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
            }
        } else if (e->x > sw - 60) show_power();
        raise_panels();
        return;
    }
    if (menu_vis && p == menu_win) {
        int x = e->x, y = e->y;
        if (x > MENU_W - 12) {
            if (app_cnt > MENU_MAX) {
                float r = (float)(y - 5) / (MENU_MAX * MENU_ITEM_H);
                if (r < 0) r = 0; if (r > 1) r = 1;
                menu_off = r * (app_cnt - MENU_MAX); menu_hover = -1; draw_menu();
            }
            return;
        }
        int idx = menu_off + (y - 5) / MENU_ITEM_H;
        if (idx >= 0 && idx < app_cnt && x < MENU_W - 12) launch(apps[idx].exec);
        XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
        raise_panels();
        return;
    }
    if (menu_vis && p != menu_win) {
        XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
        raise_panels();
    }
    for (int i = 0; i < btn_cnt; i++) {
        if (p == btns[i].btn) {
            for (int j = 0; j < frame_cnt; j++) {
                if (frames[j].client == btns[i].client) {
                    if (frames[j].minimized) minimize(&frames[j]);
                    else focus(frames[j].client);
                    break;
                }
            }
            raise_panels();
            return;
        }
    }
}

void client_msg(XClientMessageEvent *e) {
    if (e->message_type == net_tray_opcode && e->format == 32) {
        Window ic = e->data.l[2];
        if (ic) {
            XReparentWindow(dpy, ic, tray_win, tray_cnt * 28, 5);
            XResizeWindow(dpy, ic, 24, 24);
            XDefineCursor(dpy, ic, cur);
            XMapWindow(dpy, ic);
            trays[tray_cnt].id = ic; trays[tray_cnt].x = tray_cnt * 28; tray_cnt++;
        }
    }
}
