#include "panels.h"
#include "frames.h"
#include "powerctl.h"

Window top_win, bot_win, tray_win, clock_win, menu_win, apps_win;
Bool menu_vis = False;
int menu_off = 0, menu_hover = -1, menu_page = 0;

#define APPS_BTN_X 5
#define APPS_BTN_W 100
#define APPS_GAP 10
#define TRAY_W 200
#define TRAY_GAP 15
#define BTN_START_X (APPS_BTN_X + APPS_BTN_W + APPS_GAP + TRAY_W + TRAY_GAP)

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
 
 char *wallpaper_path = NULL;
 Imlib_Image wallpaper_img = NULL;
 
 void set_wallpaper(const char *path) {
     if (!path) return;
     Imlib_Image img = imlib_load_image(path);
     if (!img) {
         LOG_WARN("Failed to load wallpaper: %s", path);
         return;
     }
     imlib_context_set_image(img);
     int iw = imlib_image_get_width(), ih = imlib_image_get_height();
     Imlib_Image scaled = imlib_create_cropped_scaled_image(0, 0, iw, ih, sw, sh);
     imlib_free_image();
     if (!scaled) {
         LOG_WARN("Failed to scale wallpaper");
         return;
     }
     if (wallpaper_img) imlib_free_image();
     wallpaper_img = scaled;
     imlib_context_set_image(wallpaper_img);
     imlib_context_set_drawable(root);
     imlib_render_image_on_drawable(0, 0);
     if (wallpaper_path) free(wallpaper_path);
     wallpaper_path = strdup(path);
 }
 
 void reload_wallpaper(void) {
     if (wallpaper_path) set_wallpaper(wallpaper_path);
 }
 
 void redraw_wallpaper(void) {
     if (wallpaper_img) {
         imlib_context_set_display(dpy);
         imlib_context_set_visual(vis);
         imlib_context_set_colormap(cmap);
         imlib_context_set_drawable(root);
         imlib_context_set_image(wallpaper_img);
         imlib_render_image_on_drawable(0, 0);
     }
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
    XFillRectangle(dpy, top_win, gc_bar, 5, 3, 40, top_h - 6);
    char *txt = "an/DE";  /* placeholder, there will be logo */
    XGlyphInfo e;
    XftTextExtentsUtf8(dpy, font, (FcChar8*)txt, strlen(txt), &e);
    XftDrawStringUtf8(d, &clr_bar, font,
                      5 + (40 - e.width) / 2,
                      (top_h + e.height) / 2,
                      (FcChar8*)txt, strlen(txt));
    XftDrawDestroy(d);
}

void draw_apps_btn(void) {
    XftDraw *d = XftDrawCreate(dpy, apps_win, vis, cmap);
    XSetForeground(dpy, gc_bar, clr_btn_hover);
    XFillRectangle(dpy, apps_win, gc_bar, 0, 0, APPS_BTN_W, bot_h - 10);
    char *txt = "Apps";
    XGlyphInfo e;
    XftTextExtentsUtf8(dpy, font, (FcChar8*)txt, strlen(txt), &e);
    XftDrawStringUtf8(d, &clr_bar, font,
                      (APPS_BTN_W - e.width) / 2,
                      (bot_h - 10 + e.height) / 2,
                      (FcChar8*)txt, strlen(txt));
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

    /* "apps" button (left side) */
    apps_win = XCreateSimpleWindow(dpy, bot_win, APPS_BTN_X, 5, APPS_BTN_W, bot_h - 10, 0, clr_bottombar, clr_bottombar);
    XSelectInput(dpy, apps_win, ExposureMask | ButtonPressMask);
    XMapWindow(dpy, apps_win);
    draw_apps_btn();

    /* system tray (center-left) */
    tray_win = XCreateSimpleWindow(dpy, bot_win, APPS_BTN_X + APPS_BTN_W + APPS_GAP, 5, TRAY_W, bot_h - 10, 0, clr_bottombar, clr_bottombar);
    XSelectInput(dpy, tray_win, SubstructureNotifyMask);
    XMapWindow(dpy, tray_win);

    /* clock (right side) */
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
    int x = BTN_START_X + btn_cnt * 40;
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
                 btns[j].x = BTN_START_X + j * 40;
            }
            btn_cnt--; break;
        }
    }
}

void draw_btn(Btn *b, Bool act) {
    Frame *f = find_frame(b->client);
    Bool minimized = (f && f->minimized);
    XClearWindow(dpy, b->btn);
    XSetForeground(dpy, gc_bar, minimized ? clr_btn_hover : (act ? clr_title_ac : clr_topbar));
    XFillRectangle(dpy, b->btn, gc_bar, 0, 0, b->w, bot_h - 10);
    if (b->icon && strlen(b->icon)) {
        Pixmap p = None, m = None;
        p = load_icon(b->icon, 24, &m);
        if (p != None) {
            int ix = (b->w - 24) / 2;
            int iy = (bot_h - 10 - 24) / 2;
            if (m != None) { XSetClipMask(dpy, gc_bar, m); XSetClipOrigin(dpy, gc_bar, ix, iy); }
            XCopyArea(dpy, p, b->btn, gc_bar, 0, 0, 24, 24, ix, iy);
            if (m != None) XSetClipMask(dpy, gc_bar, None);
        }
    }
}

void update_bar(void) {
    for (int i = 0; i < btn_cnt; i++) {
        Frame *f = find_frame(btns[i].client);
        Bool focused = (f && f->focused);
        draw_btn(&btns[i], focused);
    }
}

void mk_menu(void) {
    /* fullscreen menu window */
    menu_win = XCreateSimpleWindow(dpy, root, 0, 0, sw, sh, 0, clr_menu_bg, clr_menu_bg);
    XSelectInput(dpy, menu_win, ExposureMask | ButtonPressMask | PointerMotionMask | ButtonReleaseMask);
    XDefineCursor(dpy, menu_win, cur);
    /* use popup menu type but fullscreen */
    Atom tp = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_POPUP_MENU", False);
    XChangeProperty(dpy, menu_win, net_wm_type, XA_ATOM, 32, PropModeReplace, (unsigned char*)&tp, 1);
    /* hide top bar when menu is shown */
    XUnmapWindow(dpy, top_win);
}

void draw_menu(void) {
    XClearWindow(dpy, menu_win);
    XftDraw *d = XftDrawCreate(dpy, menu_win, vis, cmap);
    
    /* calculate grid dimensions */
    int cols = TILE_COLS(sw);
    int rows = TILE_ROWS(sh);
    int tiles_per_page = cols * rows;
    int page_start = menu_page * tiles_per_page;
    int page_end = page_start + tiles_per_page;
    if (page_end > app_cnt) page_end = app_cnt;
    
    /* draw tiles in grid */
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = page_start + r * cols + c;
            if (idx >= app_cnt) break;
            
            int x = TILE_PADDING + c * (TILE_SIZE + TILE_PADDING);
            int y = top_h + TILE_PADDING + r * (TILE_SIZE + TILE_PADDING);
            
            /* highlight hovered tile */
            if (idx == menu_hover) {
                XSetForeground(dpy, gc_bar, clr_menu_hi_bg);
                XFillRectangle(dpy, menu_win, gc_bar, x, y, TILE_SIZE, TILE_SIZE);
            }
            
            /* load and draw icon */
            Pixmap p = None, m = None;
            if (apps[idx].icon_path && strlen(apps[idx].icon_path))
                p = load_icon(apps[idx].icon_path, TILE_ICON_SIZE, &m);
            if (p != None) {
                int icon_x = x + (TILE_SIZE - TILE_ICON_SIZE) / 2;
                int icon_y = y + (TILE_SIZE - TILE_ICON_SIZE) / 2 - 10;
                if (m != None) {
                    XSetClipMask(dpy, gc_bar, m);
                    XSetClipOrigin(dpy, gc_bar, icon_x, icon_y);
                }
                XCopyArea(dpy, p, menu_win, gc_bar, 0, 0, TILE_ICON_SIZE, TILE_ICON_SIZE, icon_x, icon_y);
                if (m != None) XSetClipMask(dpy, gc_bar, None);
            }
            
            /* draw application name */
            char name[128];
            snprintf(name, sizeof(name), "%s", apps[idx].name);
            /* truncate if too long (later replace with a scrolling text) */
            if (strlen(name) > 15) strcpy(name + 12, "...");
            
            XftColor tc = (idx == menu_hover) ? clr_title : clr_menu;
            XGlyphInfo e;
            XftTextExtentsUtf8(dpy, font, (FcChar8*)name, strlen(name), &e);
            int text_x = x + (TILE_SIZE - e.width) / 2;
            int text_y = y + TILE_SIZE - 10;
            XftDrawStringUtf8(d, &tc, font, text_x, text_y, (FcChar8*)name, strlen(name));
        }
    }
    
    /* draw page indicator if multiple pages */
    int total_pages = (app_cnt + tiles_per_page - 1) / tiles_per_page;
    if (total_pages > 1) {
        char page_str[32];
        snprintf(page_str, sizeof(page_str), "Page %d/%d", menu_page + 1, total_pages);
        XGlyphInfo e;
        XftTextExtentsUtf8(dpy, font, (FcChar8*)page_str, strlen(page_str), &e);
        XftDrawStringUtf8(d, &clr_menu, font, (sw - e.width) / 2, sh - 30, (FcChar8*)page_str, strlen(page_str));
    }
    
    XftDrawDestroy(d);
}

void show_power(void) {
    Window pm = XCreateSimpleWindow(dpy, root, 5, top_h + 5, 140, 120, 1, clr_topbar, clr_topbar);
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
            if (opt == 0) powerctl_lock();
            else if (opt == 1) powerctl_reboot();
            else if (opt == 2) powerctl_poweroff();
            else if (opt == 3) exit(0);
            break;
        }
    }
    XDestroyWindow(dpy, pm);
}

void btn_handler_panels(XButtonEvent *e) {
    Window p = e->window;
    if (p == top_win) {
        /* opens power menu */
        if (e->x < 50) {
            show_power();
        }
        raise_panels();
        return;
    }
    if (p == apps_win) {
        if (!menu_vis) {
            menu_off = 0; menu_hover = -1; menu_page = 0;
            mk_menu(); XMapWindow(dpy, menu_win); draw_menu(); menu_vis = True;
        } else {
            XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
            XMapWindow(dpy, top_win);
            redraw_wallpaper();
        }
        raise_panels();
        return;
    }
    if (menu_vis && p == menu_win) {
          int x = e->x, y = e->y;
          if (e->button == Button4) {  /* scroll up - previous page */
              if (menu_page > 0) { menu_page--; draw_menu(); }
              return;
          } else if (e->button == Button5) {  /* scroll down - next page */
              int tiles_per_page = TILE_COLS(sw) * TILE_ROWS(sh);
              int total_pages = (app_cnt + tiles_per_page - 1) / tiles_per_page;
              if (menu_page < total_pages - 1) { menu_page++; draw_menu(); }
              return;
          }
          /* check if clicked on a tile */
          int cols = TILE_COLS(sw);
          int rows = TILE_ROWS(sh);
          int tiles_per_page = cols * rows;
          int page_start = menu_page * tiles_per_page;
          
          /* calculate which tile was clicked */
          int tile_x = (x - TILE_PADDING) / (TILE_SIZE + TILE_PADDING);
          int tile_y = (y - top_h - TILE_PADDING) / (TILE_SIZE + TILE_PADDING);
          
          if (tile_x >= 0 && tile_x < cols && tile_y >= 0 && tile_y < rows) {
              int idx = page_start + tile_y * cols + tile_x;
              if (idx < app_cnt) {
                  launch(apps[idx].exec);
                  XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
                  XMapWindow(dpy, top_win);
                  redraw_wallpaper();
                  raise_panels();
                  return;
              }
          }

          return;
      }
       if (menu_vis && p != menu_win) {
           XUnmapWindow(dpy, menu_win); XDestroyWindow(dpy, menu_win); menu_vis = False;
           XMapWindow(dpy, top_win);
           redraw_wallpaper();
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
     else if (e->message_type == net_wm_state && e->format == 32) {
         Frame *f = find_frame(e->window);
         if (!f) return;
         Atom action = (Atom)e->data.l[0];
         Atom state1 = (Atom)e->data.l[1];
         Atom state2 = (Atom)e->data.l[2];
         Atom states[2] = { state1, state2 };
         for (int i = 0; i < 2; i++) {
             Atom state = states[i];
             if (state == None || state == 0) continue;
             if (state == net_wm_state_hidden) {
                 if (action == net_wm_state_toggle) {
                     minimize(f);
                 } else if (action == net_wm_state_add && !f->minimized) {
                     minimize(f);
                 } else if (action == net_wm_state_remove && f->minimized) {
                     minimize(f);
                 }
             } else if (state == net_wm_state_fs) {
                 if (action == net_wm_state_toggle) {
                     toggle_fs(f);
                 } else if (action == net_wm_state_add && !f->fullscreen) {
                     toggle_fs(f);
                 } else if (action == net_wm_state_remove && f->fullscreen) {
                     toggle_fs(f);
                 }
             }
         }
     }
 }
