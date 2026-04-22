#include "wm.h"

Display *dpy;
Window root;
int scr;
int sw, sh;
Visual *vis;
Colormap cmap;
int depth;

int refresh_rate = 60;
int vsync_us = 16666;

Atom wm_del_win;
Atom wm_proto;
Atom wm_take_focus;
Atom net_wm_name;
Atom net_wm_state;
Atom net_wm_state_hidden;
Atom net_wm_state_fs;
Atom net_wm_type;
Atom type_dock;
Atom type_desk;
Atom utf8_str;
Atom net_tray_opcode;

XftFont *font;
XftFont *font_small;
XftFont *font_bold;
XftColor clr_title;
XftColor clr_btn;
XftColor clr_bar;
XftColor clr_clock;
XftColor clr_menu;
XftColor clr_menu_hi;

unsigned long clr_border;
unsigned long clr_border_ac;
unsigned long clr_border_fc;
unsigned long clr_title_bg;
unsigned long clr_title_ac;
unsigned long clr_close;
unsigned long clr_min;
unsigned long clr_max;
unsigned long clr_topbar;
unsigned long clr_bottombar;
unsigned long clr_btn_hover;
unsigned long clr_menu_bg;
unsigned long clr_menu_hi_bg;

GC gc_bar;
GC gc_text;
GC gc_title;

Cursor cur;

int top_h = 28;
int bot_h = 36;

App apps[MAX_APP];
int app_cnt = 0;

struct timeval last_frame;

void log_msg(const char *lvl, const char *file, int line, const char *fmt, ...) {
    FILE *log = fopen(LOG_FILE, "a");
    if (!log) return;
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm);
    fprintf(log, "[%s] %s (%s:%d) - ", ts, lvl, file, line);
    va_list args;
    va_start(args, fmt);
    vfprintf(log, fmt, args);
    va_end(args);
    fprintf(log, "\n");
    fflush(log);
    fclose(log);
}

int err_handler(Display *d, XErrorEvent *e) {
    char txt[256];
    XGetErrorText(d, e->error_code, txt, sizeof(txt));
    if (e->error_code == BadWindow || e->error_code == BadDrawable || e->error_code == BadMatch) {
        LOG_DBG("X11 Error (ignored): %s (res: 0x%lx)", txt, e->resourceid);
        return 0;
    }
    LOG_ERROR("X11 Error: %s (code: %d, res: 0x%lx)", txt, e->error_code, e->resourceid);
    if (e->error_code == BadAccess) {
        fprintf(stderr, "ERROR: Another WM is running.\n");
        exit(1);
    }
    return 0;
}

XftColor mk_xft_clr(const char *hex) {
    XftColor c;
    XRenderColor rc;
    unsigned int r, g, b;
    sscanf(hex + 1, "%02x%02x%02x", &r, &g, &b);
    rc.red = (r << 8) | r;
    rc.green = (g << 8) | g;
    rc.blue = (b << 8) | b;
    rc.alpha = 0xFFFF;
    XftColorAllocValue(dpy, vis, cmap, &rc, &c);
    return c;
}

void vsync(void) {
    struct timeval now;
    gettimeofday(&now, NULL);
    long elapsed = (now.tv_sec - last_frame.tv_sec) * 1000000 + (now.tv_usec - last_frame.tv_usec);
    if (elapsed < vsync_us) usleep(vsync_us - elapsed);
    gettimeofday(&last_frame, NULL);
}

void get_refresh(void) {
    int ev_base, err_base, maj, min;
    if (!XRRQueryExtension(dpy, &ev_base, &err_base) || !XRRQueryVersion(dpy, &maj, &min)) {
        LOG_WARN("Xrandr not available, using 60 Hz");
        return;
    }
    XRRScreenResources *res = XRRGetScreenResources(dpy, root);
    if (!res) return;
    for (int i = 0; i < res->ncrtc; i++) {
        XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, res, res->crtcs[i]);
        if (crtc && crtc->mode != None) {
            for (int j = 0; j < res->nmode; j++) {
                if (res->modes[j].id == crtc->mode) {
                    if (res->modes[j].dotClock && res->modes[j].hTotal && res->modes[j].vTotal) {
                        double ref = (double)res->modes[j].dotClock / (res->modes[j].hTotal * res->modes[j].vTotal);
                        refresh_rate = (int)(ref + 0.5);
                        vsync_us = 1000000 / refresh_rate;
                        LOG_INFO("Monitor: %dx%d @ %d Hz", crtc->width, crtc->height, refresh_rate);
                        XRRFreeCrtcInfo(crtc);
                        XRRFreeScreenResources(res);
                        return;
                    }
                }
            }
        }
        if (crtc) XRRFreeCrtcInfo(crtc);
    }
    XRRFreeScreenResources(res);
}

char* find_icon(const char *name) {
    if (!name || !strlen(name)) return strdup("");
    if (name[0] == '/') {
        if (access(name, F_OK) == 0) return strdup(name);
        return strdup("");
    }
    if (strncmp(name, "steam_icon_", 11) == 0) {
        const char *paths[] = {"/usr/share/icons/hicolor/256x256/apps", "/usr/share/pixmaps", NULL};
        for (int i = 0; paths[i]; i++) {
            char p[512];
            snprintf(p, sizeof(p), "%s/%s.png", paths[i], name);
            if (access(p, F_OK) == 0) return strdup(p);
        }
        return strdup("");
    }
    const char *paths[] = {
        "/usr/share/icons/hicolor/48x48/apps",
        "/usr/share/icons/hicolor/256x256/apps",
        "/usr/share/icons/hicolor/scalable/apps",
        "/usr/share/pixmaps", NULL
    };
    const char *ext[] = {".png", ".svg", ".xpm", NULL};
    for (int i = 0; paths[i]; i++) {
        for (int j = 0; ext[j]; j++) {
            char p[512];
            snprintf(p, sizeof(p), "%s/%s%s", paths[i], name, ext[j]);
            if (access(p, F_OK) == 0) return strdup(p);
        }
    }
    return strdup("");
}

void parse_desktop(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[2048], *name = NULL, *exec = NULL, *icon = NULL, *comment = NULL, *cat = NULL;
    int in = 0;
    Bool nodisp = False;
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        if (!strcmp(line, "[Desktop Entry]")) { in = 1; continue; }
        if (line[0] == '[') in = 0;
        if (!in) continue;
        if (!strncmp(line, "Name=", 5)) { if (name) free(name); name = strdup(line + 5); }
        else if (!strncmp(line, "Exec=", 5)) {
            if (exec) free(exec);
            exec = strdup(line + 5);
            char *p = strchr(exec, '%'); if (p) *p = 0;
            while (strlen(exec) && exec[strlen(exec)-1] == ' ') exec[strlen(exec)-1] = 0;
        }
        else if (!strncmp(line, "Icon=", 5)) { if (icon) free(icon); icon = strdup(line + 5); }
        else if (!strncmp(line, "Comment=", 8)) { if (comment) free(comment); comment = strdup(line + 8); }
        else if (!strncmp(line, "Categories=", 11)) { if (cat) free(cat); cat = strdup(line + 11); }
        else if (!strcmp(line, "NoDisplay=true") || !strcmp(line, "NoDisplay=True")) nodisp = True;
    }
    fclose(f);
    if (nodisp) { if(name)free(name); if(exec)free(exec); if(icon)free(icon); if(comment)free(comment); if(cat)free(cat); return; }
    if (name && exec && app_cnt < MAX_APP) {
        apps[app_cnt].name = name;
        apps[app_cnt].exec = exec;
        apps[app_cnt].icon_path = find_icon(icon ? icon : "");
        apps[app_cnt].comment = comment ? comment : strdup("");
        apps[app_cnt].cat = cat ? cat : strdup("");
        app_cnt++;
        if (icon) free(icon);
    } else {
        if (name) free(name); if (exec) free(exec); if (icon) free(icon);
        if (comment) free(comment); if (cat) free(cat);
    }
}

void load_apps(void) {
    const char *paths[] = {"/usr/share/applications", "/usr/local/share/applications", NULL};
    char *home = NULL;
    const char *h = getenv("HOME");
    if (h) { home = malloc(strlen(h) + 30); sprintf(home, "%s/.local/share/applications", h); paths[2] = home; }
    for (int i = 0; i < 3; i++) {
        if (!paths[i]) continue;
        DIR *d = opendir(paths[i]); if (!d) continue;
        struct dirent *e;
        while ((e = readdir(d))) {
            if (strstr(e->d_name, ".desktop")) {
                char fp[1024];
                snprintf(fp, sizeof(fp), "%s/%s", paths[i], e->d_name);
                parse_desktop(fp);
            }
        }
        closedir(d);
    }
    if (home) free(home);
    for (int i = 0; i < app_cnt - 1; i++)
        for (int j = i + 1; j < app_cnt; j++)
            if (strcasecmp(apps[i].name, apps[j].name) > 0) {
                App t = apps[i]; apps[i] = apps[j]; apps[j] = t;
            }
    LOG_INFO("Loaded %d apps", app_cnt);
    printf("Loaded %d applications\n", app_cnt);
}

void launch(const char *exec) {
    LOG_INFO("Launch: %s", exec);
    if (fork() == 0) {
        setsid();
        execl("/bin/sh", "sh", "-c", exec, NULL);
        exit(0);
    }
}

void setup_cursor(void) {
    cur = XCreateFontCursor(dpy, XC_left_ptr);
    XDefineCursor(dpy, root, cur);
}

void init_wm(void) {
    FILE *log = fopen(LOG_FILE, "w");
    if (log) { fprintf(log, "=== NotAWM Log ===\n"); fclose(log); }
    LOG_INFO("Init NotAWM...");
    get_refresh();
    gettimeofday(&last_frame, NULL);
    setup_cursor();
    load_apps();
    LOG_INFO("Init complete");
}
