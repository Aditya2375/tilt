#ifndef TILT_H
#define TILT_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "config.h"

struct client {
    Window win;
    char name[128];
    int is_float;               /* transients/dialogs: centered, untiled */
    struct client *next;
};

struct workspace {
    struct client *clients;     /* master first */
    struct client *focused;
};

struct wm {
    Display *dpy;
    int screen;
    Window root;
    Window bar;
    GC gc;
    XFontStruct *font;
    int screen_w, screen_h;
    int mfact;                  /* master width, per-mille of area */
    struct workspace ws[WORKSPACES];
    int current;
    int running;
    Atom net_active_window;
    Atom net_wm_name;
    Atom net_wm_window_type;
    Atom net_wm_type_dialog;
    Atom utf8_string;
    Atom wm_protocols;
    Atom wm_delete_window;
    unsigned long col_focus, col_normal, col_bar_bg, col_bar_fg, col_bar_sel;
};

/* client.c */
struct client *client_add(struct wm *wm, Window win);
void client_remove(struct wm *wm, Window win);
struct client *client_find(struct wm *wm, Window win);
void client_focus(struct wm *wm, struct client *c);
void client_move_to_master(struct wm *wm, struct client *c);
void client_move_to_ws(struct wm *wm, struct client *c, int ws);
int client_count(struct workspace *ws);

/* wm.c */
void wm_arrange(struct wm *wm);
void wm_draw_bar(struct wm *wm);
void wm_fetch_title(struct wm *wm, Window win, char *buf, size_t cap);

#endif
