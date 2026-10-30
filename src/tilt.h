#ifndef TILT_H
#define TILT_H

#include <X11/Xlib.h>

#define WORKSPACES 9
#define MOD Mod1Mask   /* Alt */
#define BORDER 3
#define GAPS 8
#define BAR_H 22

struct client {
    Window win;
    char name[128];
    struct client *next;
};

struct workspace {
    struct client *clients;     /* master first */
    struct client *focused;
    int visible;
};

struct wm {
    Display *dpy;
    Window root;
    Window bar;
    GC gc;
    XFontStruct *font;
    int screen_w, screen_h;
    struct workspace ws[WORKSPACES];
    int current;
    int running;
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

#endif
