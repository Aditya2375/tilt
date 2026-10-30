#include "layout.h"
#include "tilt.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long color(struct wm *wm, const char *name) {
    XColor c, exact;
    if (!XAllocNamedColor(wm->dpy, DefaultColormap(wm->dpy, 0), name, &c, &exact)) {
        fprintf(stderr, "tilt: cannot allocate color %s\n", name);
        exit(1);
    }
    return c.pixel;
}

void wm_arrange(struct wm *wm) {
    struct workspace *ws = &wm->ws[wm->current];
    int n = client_count(ws);
    if (n == 0) return;
    struct rect *rects = malloc((size_t)n * sizeof *rects);
    if (!rects) { perror("malloc"); exit(1); }
    /* clients list is most-recent-first; master is the tail... no: master is
       the head of the list. Build in list order with head as master. */
    tile_layout(n, wm->screen_w, wm->screen_h, BAR_H, GAPS, rects);
    int i = 0;
    for (struct client *c = ws->clients; c; c = c->next, i++) {
        XMoveResizeWindow(wm->dpy, c->win, rects[i].x, rects[i].y,
                          (unsigned)rects[i].w - 2 * BORDER,
                          (unsigned)rects[i].h - 2 * BORDER);
        XSetWindowBorderWidth(wm->dpy, c->win, BORDER);
        XSetWindowBorder(wm->dpy, c->win,
                         c == ws->focused ? wm->col_focus : wm->col_normal);
    }
    free(rects);
}

void wm_draw_bar(struct wm *wm) {
    XGCValues gv;
    gv.font = wm->font->fid;
    XChangeGC(wm->dpy, wm->gc, GCFont, &gv);
    /* background */
    gv.foreground = wm->col_bar_bg;
    XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
    XFillRectangle(wm->dpy, wm->bar, wm->gc, 0, 0, (unsigned)wm->screen_w, BAR_H);
    int x = 6;
    for (int i = 0; i < WORKSPACES; i++) {
        char tag[4];
        snprintf(tag, sizeof tag, " %d ", i + 1);
        int occupied = wm->ws[i].clients != NULL;
        int current = i == wm->current;
        if (current) {
            gv.foreground = wm->col_bar_sel;
            XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
            XFillRectangle(wm->dpy, wm->bar, wm->gc, x - 2, 2,
                           (unsigned)XTextWidth(wm->font, tag, (int)strlen(tag)) + 4, BAR_H - 4);
            gv.foreground = wm->col_bar_bg;
        } else {
            gv.foreground = occupied ? wm->col_bar_fg : wm->col_normal;
        }
        XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
        XDrawString(wm->dpy, wm->bar, wm->gc, x, 15, tag, (int)strlen(tag));
        x += XTextWidth(wm->font, tag, (int)strlen(tag)) + 6;
    }
    /* focused window title, right-aligned */
    struct client *f = wm->ws[wm->current].focused;
    if (f && f->name[0]) {
        int tw = XTextWidth(wm->font, f->name, (int)strlen(f->name));
        gv.foreground = wm->col_bar_fg;
        XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
        XDrawString(wm->dpy, wm->bar, wm->gc, wm->screen_w - tw - 8, 15, f->name, (int)strlen(f->name));
    }
}

static void wm_switch_ws(struct wm *wm, int target) {
    if (target == wm->current) return;
    struct workspace *from = &wm->ws[wm->current];
    /* hide by moving offscreen: unmapping would bounce back as an
       UnmapNotify that looks like the window withdrawing */
    for (struct client *c = from->clients; c; c = c->next)
        XMoveWindow(wm->dpy, c->win, -3000, -3000);
    wm->current = target;
    struct workspace *to = &wm->ws[target];
    for (struct client *c = to->clients; c; c = c->next)
        XMapWindow(wm->dpy, c->win);
    wm_arrange(wm);
    client_focus(wm, to->focused ? to->focused : to->clients);
}

static void grab_keys(struct wm *wm) {
    KeyCode keys[] = {
        XKeysymToKeycode(wm->dpy, XK_j),
        XKeysymToKeycode(wm->dpy, XK_k),
        XKeysymToKeycode(wm->dpy, XK_Return),
        XKeysymToKeycode(wm->dpy, XK_q),
        0
    };
    for (int i = 0; keys[i]; i++) {
        XGrabKey(wm->dpy, keys[i], MOD, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, keys[i], MOD | ShiftMask, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, keys[i], MOD | LockMask, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, keys[i], MOD | ShiftMask | LockMask, wm->root, True, GrabModeAsync, GrabModeAsync);
    }
    for (int i = 1; i <= WORKSPACES; i++) {
        char sym = '0' + i;
        KeyCode kc = XKeysymToKeycode(wm->dpy, (KeySym)sym);
        XGrabKey(wm->dpy, kc, MOD, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, kc, MOD | ShiftMask, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, kc, MOD | LockMask, wm->root, True, GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, kc, MOD | ShiftMask | LockMask, wm->root, True, GrabModeAsync, GrabModeAsync);
    }
}

static void handle_map_request(struct wm *wm, XMapRequestEvent *ev) {
    if (client_find(wm, ev->window)) { XMapWindow(wm->dpy, ev->window); return; }
    struct client *c = client_add(wm, ev->window);
    XSelectInput(wm->dpy, ev->window, EnterWindowMask | PropertyChangeMask);
    XSetWindowBorderWidth(wm->dpy, ev->window, BORDER);
    XMapWindow(wm->dpy, ev->window);
    /* new window becomes master */
    wm_arrange(wm);
    client_focus(wm, c);
}

static void handle_unmap(struct wm *wm, Window win) {
    struct client *c = client_find(wm, win);
    if (!c) return;
    client_remove(wm, win);
    wm_arrange(wm);
    struct workspace *ws = &wm->ws[wm->current];
    if (ws->focused) client_focus(wm, ws->focused);
    else wm_draw_bar(wm);
}

static void send_delete(struct wm *wm, Window win) {
    Atom wm_delete = XInternAtom(wm->dpy, "WM_DELETE_WINDOW", False);
    Atom wm_protocols = XInternAtom(wm->dpy, "WM_PROTOCOLS", False);
    Atom *protocols = NULL;
    int n = 0;
    int supports = 0;
    if (XGetWMProtocols(wm->dpy, win, &protocols, &n)) {
        for (int i = 0; i < n; i++)
            if (protocols[i] == wm_delete) supports = 1;
        XFree(protocols);
    }
    if (supports) {
        XEvent ev = {0};
        ev.xclient.type = ClientMessage;
        ev.xclient.window = win;
        ev.xclient.message_type = wm_protocols;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = (long)wm_delete;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(wm->dpy, win, False, NoEventMask, &ev);
    } else {
        XKillClient(wm->dpy, win);
    }
}

static void handle_key(struct wm *wm, XKeyEvent *ev) {
    KeySym sym = XLookupKeysym(ev, 0);
    int shift = (ev->state & ShiftMask) != 0;
    struct workspace *ws = &wm->ws[wm->current];

    if (sym >= XK_1 && sym <= XK_9) {
        int target = (int)(sym - XK_1);
        if (shift) {
            client_move_to_ws(wm, ws->focused, target);
            wm_arrange(wm);
        } else {
            wm_switch_ws(wm, target);
        }
        return;
    }
    switch (sym) {
    case XK_q:
        if (shift && ws->focused) send_delete(wm, ws->focused->win);
        else if (!shift) wm->running = 0;
        return;
    case XK_j: case XK_k: {
        if (!ws->focused) return;
        /* cycle: j = next, k = previous (list is a chain, so k wraps via master) */
        struct client *next = NULL;
        if (sym == XK_j) {
            next = ws->focused->next ? ws->focused->next : ws->clients;
        } else {
            /* previous: find the one whose next is focused, else the tail */
            struct client *prev = NULL, *tail = NULL;
            for (struct client *c = ws->clients; c; c = c->next) {
                if (c->next == ws->focused) prev = c;
                if (!c->next) tail = c;
            }
            next = prev ? prev : tail;
        }
        if (next) client_focus(wm, next);
        return;
    }
    case XK_Return:
        if (ws->focused) {
            client_move_to_master(wm, ws->focused);
            wm_arrange(wm);
        }
        return;
    }
}

static void handle_configure_request(struct wm *wm, XConfigureRequestEvent *ev) {
    /* tiling WMs dictate geometry; allow only stacking changes */
    XWindowChanges wc;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(wm->dpy, ev->window, (unsigned)(CWSibling | CWStackMode), &wc);
}

static int on_other_wm(Display *dpy, XErrorEvent *ev) {
    (void)dpy; (void)ev;
    fprintf(stderr, "tilt: another window manager is already running\n");
    exit(1);
}

int main(void) {
    struct wm wm;
    memset(&wm, 0, sizeof wm);
    wm.dpy = XOpenDisplay(NULL);
    if (!wm.dpy) { fprintf(stderr, "tilt: cannot open display\n"); return 1; }
    wm.root = DefaultRootWindow(wm.dpy);
    wm.screen_w = DisplayWidth(wm.dpy, 0);
    wm.screen_h = DisplayHeight(wm.dpy, 0);
    wm.running = 1;

    XSetErrorHandler(on_other_wm);
    XSelectInput(wm.dpy, wm.root, SubstructureRedirectMask | SubstructureNotifyMask);
    XSync(wm.dpy, False);
    XSetErrorHandler(NULL);  /* from here, errors are non-fatal */

    wm.col_focus = color(&wm, "#33ccff");
    wm.col_normal = color(&wm, "#444444");
    wm.col_bar_bg = color(&wm, "#1a1a2e");
    wm.col_bar_fg = color(&wm, "#e0e0e0");
    wm.col_bar_sel = color(&wm, "#33ccff");

    wm.font = XLoadQueryFont(wm.dpy, "fixed");
    if (!wm.font) { fprintf(stderr, "tilt: no 'fixed' font\n"); return 1; }
    wm.gc = XCreateGC(wm.dpy, wm.root, 0, NULL);

    wm.bar = XCreateSimpleWindow(wm.dpy, wm.root, 0, 0, (unsigned)wm.screen_w, BAR_H,
                                 0, wm.col_normal, wm.col_bar_bg);
    XMapWindow(wm.dpy, wm.bar);

    grab_keys(&wm);
    wm_draw_bar(&wm);
    XSync(wm.dpy, False);

    fprintf(stderr, "tilt: running on %dx%d\n", wm.screen_w, wm.screen_h);
    while (wm.running) {
        XEvent ev;
        XNextEvent(wm.dpy, &ev);
        switch (ev.type) {
        case MapRequest: handle_map_request(&wm, &ev.xmaprequest); break;
        case UnmapNotify:
            if (!ev.xunmap.send_event) handle_unmap(&wm, ev.xunmap.window);
            break;
        case DestroyNotify: handle_unmap(&wm, ev.xdestroywindow.window); break;
        case ConfigureRequest: handle_configure_request(&wm, &ev.xconfigurerequest); break;
        case KeyPress: handle_key(&wm, &ev.xkey); break;
        case Expose: if (ev.xexpose.window == wm.bar) wm_draw_bar(&wm); break;
        case PropertyNotify:
            if (ev.xproperty.atom == XA_WM_NAME) {
                char *name = NULL;
                struct client *c = client_find(&wm, ev.xproperty.window);
                if (c && XFetchName(wm.dpy, ev.xproperty.window, &name) && name) {
                    snprintf(c->name, sizeof c->name, "%s", name);
                    XFree(name);
                    wm_draw_bar(&wm);
                }
            }
            break;
        }
    }

    XUngrabKey(wm.dpy, AnyKey, AnyModifier, wm.root);
    XCloseDisplay(wm.dpy);
    return 0;
}
