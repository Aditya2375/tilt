#include "layout.h"
#include "tilt.h"

#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static unsigned long color(struct wm *wm, const char *name) {
    XColor c, exact;
    if (!XAllocNamedColor(wm->dpy, DefaultColormap(wm->dpy, wm->screen),
                          name, &c, &exact)) {
        fprintf(stderr, "tilt: cannot allocate color %s\n", name);
        exit(1);
    }
    return c.pixel;
}

/* _NET_WM_NAME (UTF-8) first, legacy WM_NAME as fallback. */
void wm_fetch_title(struct wm *wm, Window win, char *buf, size_t cap) {
    buf[0] = '\0';
    Atom actual;
    int format;
    unsigned long n, after;
    unsigned char *prop = NULL;
    if (XGetWindowProperty(wm->dpy, win, wm->net_wm_name, 0, 128, False,
                           wm->utf8_string, &actual, &format, &n, &after,
                           &prop) == Success && prop) {
        snprintf(buf, cap, "%s", (char *)prop);
        XFree(prop);
        if (buf[0]) return;
    }
    char *name = NULL;
    if (XFetchName(wm->dpy, win, &name) && name) {
        snprintf(buf, cap, "%s", name);
        XFree(name);
    }
}

void wm_arrange(struct wm *wm) {
    struct workspace *ws = &wm->ws[wm->current];
    /* floats center and raise no matter how many tiled clients exist -
       including zero, so a solo dialog still gets dialog geometry */
    for (struct client *c = ws->clients; c; c = c->next) {
        if (!c->is_float) continue;
        int fw = wm->screen_w / 2, fh = wm->screen_h / 2;
        XMoveResizeWindow(wm->dpy, c->win,
                          (wm->screen_w - fw) / 2, (wm->screen_h - fh) / 2,
                          (unsigned)fw, (unsigned)fh);
        XRaiseWindow(wm->dpy, c->win);
    }
    int n = client_count(ws);
    if (n == 0) { wm_draw_bar(wm); return; }
    struct rect *rects = malloc((size_t)n * sizeof *rects);
    if (!rects) { perror("malloc"); exit(1); }
    tile_layout(n, wm->screen_w, wm->screen_h, BAR_H, GAPS, wm->mfact, rects);
    int i = 0;
    for (struct client *c = ws->clients; c; c = c->next) {
        if (c->is_float) continue;      /* placed above */
        int cw = rects[i].w - 2 * BORDER;
        int ch = rects[i].h - 2 * BORDER;
        XMoveResizeWindow(wm->dpy, c->win, rects[i].x, rects[i].y,
                          (unsigned)(cw > 0 ? cw : 1),
                          (unsigned)(ch > 0 ? ch : 1));
        XSetWindowBorderWidth(wm->dpy, c->win, BORDER);
        XSetWindowBorder(wm->dpy, c->win,
                         c == ws->focused ? wm->col_focus : wm->col_normal);
        i++;
    }
    free(rects);
}

void wm_draw_bar(struct wm *wm) {
    XGCValues gv;
    gv.font = wm->font->fid;
    XChangeGC(wm->dpy, wm->gc, GCFont, &gv);
    int text_y = (BAR_H + wm->font->ascent - wm->font->descent) / 2;
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
                           (unsigned)XTextWidth(wm->font, tag, (int)strlen(tag)) + 4,
                           BAR_H - 4);
            gv.foreground = wm->col_bar_bg;
        } else {
            gv.foreground = occupied ? wm->col_bar_fg : wm->col_normal;
        }
        XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
        XDrawString(wm->dpy, wm->bar, wm->gc, x, text_y, tag, (int)strlen(tag));
        x += XTextWidth(wm->font, tag, (int)strlen(tag)) + 6;
    }
    /* focused window title, right-aligned, clipped so it never covers tags */
    struct client *f = wm->ws[wm->current].focused;
    if (f && f->name[0]) {
        int avail = wm->screen_w - x - 16;
        if (avail > 0) {
            int len = (int)strlen(f->name);
            while (len > 0 && XTextWidth(wm->font, f->name, len) > avail)
                len--;
            gv.foreground = wm->col_bar_fg;
            XChangeGC(wm->dpy, wm->gc, GCForeground, &gv);
            XDrawString(wm->dpy, wm->bar, wm->gc, wm->screen_w - 8
                        - XTextWidth(wm->font, f->name, len), text_y,
                        f->name, len);
        }
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

static void spawn(const char *cmd) {
    if (fork() == 0) {
        setsid();
        execlp(cmd, cmd, (char *)NULL);
        _exit(127);
    }
}

/* Grab a key under every lock-modifier combination, so bindings work with
   Caps Lock and Num Lock in any state. */
static void grab_key(struct wm *wm, KeyCode kc) {
    static const unsigned locks[] = {
        0, LockMask, Mod2Mask, LockMask | Mod2Mask
    };
    for (unsigned i = 0; i < sizeof locks / sizeof locks[0]; i++) {
        XGrabKey(wm->dpy, kc, MOD | locks[i], wm->root, True,
                 GrabModeAsync, GrabModeAsync);
        XGrabKey(wm->dpy, kc, MOD | ShiftMask | locks[i], wm->root, True,
                 GrabModeAsync, GrabModeAsync);
    }
}

static void grab_keys(struct wm *wm) {
    KeySym syms[] = { XK_j, XK_k, XK_h, XK_l, XK_Return, XK_q };
    for (unsigned i = 0; i < sizeof syms / sizeof syms[0]; i++)
        grab_key(wm, XKeysymToKeycode(wm->dpy, syms[i]));
    for (int i = 1; i <= WORKSPACES; i++)
        grab_key(wm, XKeysymToKeycode(wm->dpy, (KeySym)('0' + i)));
}

static void handle_map_request(struct wm *wm, XMapRequestEvent *ev) {
    if (client_find(wm, ev->window)) { XMapWindow(wm->dpy, ev->window); return; }
    struct client *c = client_add(wm, ev->window);
    XSelectInput(wm->dpy, ev->window, PropertyChangeMask);
    XSetWindowBorderWidth(wm->dpy, ev->window, BORDER);
    XMapWindow(wm->dpy, ev->window);
    wm_arrange(wm);        /* new window becomes master */
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
    Atom *protocols = NULL;
    int n = 0;
    int supports = 0;
    if (XGetWMProtocols(wm->dpy, win, &protocols, &n)) {
        for (int i = 0; i < n; i++)
            if (protocols[i] == wm->wm_delete_window) supports = 1;
        XFree(protocols);
    }
    if (supports) {
        XEvent ev = {0};
        ev.xclient.type = ClientMessage;
        ev.xclient.window = win;
        ev.xclient.message_type = wm->wm_protocols;
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = (long)wm->wm_delete_window;
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
    case XK_Return:
        if (shift) {
            spawn(TERMINAL);
        } else if (ws->focused) {
            client_move_to_master(wm, ws->focused);
            wm_arrange(wm);
        }
        return;
    case XK_h:
        wm->mfact -= MFACT_STEP;
        if (wm->mfact < 100) wm->mfact = 100;
        wm_arrange(wm);
        return;
    case XK_l:
        wm->mfact += MFACT_STEP;
        if (wm->mfact > 900) wm->mfact = 900;
        wm_arrange(wm);
        return;
    case XK_j: case XK_k: {
        if (!ws->focused) return;
        struct client *next = NULL;
        if (sym == XK_j) {
            next = ws->focused->next ? ws->focused->next : ws->clients;
        } else {
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
    }
}

static void handle_configure_request(struct wm *wm, XConfigureRequestEvent *ev) {
    /* floating clients configure themselves; tiled clients may restack */
    struct client *c = client_find(wm, ev->window);
    if (c && c->is_float) {
        XWindowChanges wc;
        wc.x = ev->x; wc.y = ev->y;
        wc.width = ev->width; wc.height = ev->height;
        wc.sibling = ev->above; wc.stack_mode = ev->detail;
        XConfigureWindow(wm->dpy, ev->window, (unsigned)ev->value_mask, &wc);
        return;
    }
    XWindowChanges wc;
    wc.sibling = ev->above;
    wc.stack_mode = ev->detail;
    XConfigureWindow(wm->dpy, ev->window, (unsigned)(CWSibling | CWStackMode), &wc);
}

/* Only the startup "another WM running" check exits; after that, X errors
   (a window dying mid-request, a stale grab) are logged and ignored. */
static int on_startup_error(Display *dpy, XErrorEvent *ev) {
    (void)dpy; (void)ev;
    fprintf(stderr, "tilt: another window manager is already running\n");
    exit(1);
    return 0;
}

static int on_error(Display *dpy, XErrorEvent *ev) {
    char msg[128];
    XGetErrorText(dpy, ev->error_code, msg, sizeof msg);
    fprintf(stderr, "tilt: X error: %s (request %d)\n", msg, ev->request_code);
    return 0;
}

static volatile sig_atomic_t got_signal = 0;
static void on_signal(int sig, siginfo_t *info, void *ctx) {
    (void)ctx;
    got_signal = 1;
    char buf[128];
    int n = snprintf(buf, sizeof buf, "tilt: signal %d from pid %ld\n",
                     sig, (long)(info ? info->si_pid : -1));
    (void)!write(2, buf, (size_t)n);
}

int main(void) {
    struct wm wm;
    memset(&wm, 0, sizeof wm);
    wm.dpy = XOpenDisplay(NULL);
    if (!wm.dpy) { fprintf(stderr, "tilt: cannot open display\n"); return 1; }
    wm.screen = DefaultScreen(wm.dpy);
    wm.root = RootWindow(wm.dpy, wm.screen);
    wm.screen_w = DisplayWidth(wm.dpy, wm.screen);
    wm.screen_h = DisplayHeight(wm.dpy, wm.screen);
    wm.mfact = 500;
    wm.running = 1;
    wm.net_active_window = XInternAtom(wm.dpy, "_NET_ACTIVE_WINDOW", False);
    wm.net_wm_name = XInternAtom(wm.dpy, "_NET_WM_NAME", False);
    wm.net_wm_window_type = XInternAtom(wm.dpy, "_NET_WM_WINDOW_TYPE", False);
    wm.net_wm_type_dialog = XInternAtom(wm.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    wm.utf8_string = XInternAtom(wm.dpy, "UTF8_STRING", False);
    wm.wm_protocols = XInternAtom(wm.dpy, "WM_PROTOCOLS", False);
    wm.wm_delete_window = XInternAtom(wm.dpy, "WM_DELETE_WINDOW", False);

    XSetErrorHandler(on_startup_error);
    XSelectInput(wm.dpy, wm.root, SubstructureRedirectMask | SubstructureNotifyMask
                 | StructureNotifyMask);
    XSync(wm.dpy, False);
    XSetErrorHandler(on_error);

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_sigaction = on_signal;
    sa.sa_flags = SA_SIGINFO;   /* no SA_RESTART: poll wakes with EINTR */
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    signal(SIGCHLD, SIG_IGN);   /* reap spawned terminals automatically */

    wm.col_focus = color(&wm, COL_FOCUS);
    wm.col_normal = color(&wm, COL_NORMAL);
    wm.col_bar_bg = color(&wm, COL_BAR_BG);
    wm.col_bar_fg = color(&wm, COL_BAR_FG);
    wm.col_bar_sel = color(&wm, COL_BAR_SEL);

    wm.font = XLoadQueryFont(wm.dpy, "fixed");
    if (!wm.font) { fprintf(stderr, "tilt: no 'fixed' font\n"); return 1; }
    wm.gc = XCreateGC(wm.dpy, wm.root, 0, NULL);

    wm.bar = XCreateSimpleWindow(wm.dpy, wm.root, 0, 0, (unsigned)wm.screen_w,
                                 BAR_H, 0, wm.col_normal, wm.col_bar_bg);
    XStoreName(wm.dpy, wm.bar, "tilt-bar");
    XMapWindow(wm.dpy, wm.bar);

    grab_keys(&wm);
    wm_draw_bar(&wm);
    XSync(wm.dpy, False);

    fprintf(stderr, "tilt: running on %dx%d\n", wm.screen_w, wm.screen_h);
    int xfd = XConnectionNumber(wm.dpy);
    while (wm.running && !got_signal) {
        /* drain queued events, then wait in poll (not XNextEvent) so a
           signal interrupts the wait even when no X traffic arrives */
        if (XPending(wm.dpy) == 0) {
            struct pollfd pfd = { .fd = xfd, .events = POLLIN };
            int pr = poll(&pfd, 1, 1000);
            if (pr <= 0) continue;   /* EINTR from a signal, or timeout */
        }
        XEvent ev;
        XNextEvent(wm.dpy, &ev);
        if (got_signal) break;
        switch (ev.type) {
        case MapRequest: handle_map_request(&wm, &ev.xmaprequest); break;
        case UnmapNotify:
            if (!ev.xunmap.send_event) handle_unmap(&wm, ev.xunmap.window);
            break;
        case DestroyNotify: handle_unmap(&wm, ev.xdestroywindow.window); break;
        case ConfigureRequest: handle_configure_request(&wm, &ev.xconfigurerequest); break;
        case KeyPress: handle_key(&wm, &ev.xkey); break;
        case Expose: if (ev.xexpose.window == wm.bar) wm_draw_bar(&wm); break;
        case ConfigureNotify:
            if (ev.xconfigure.window == wm.root &&
                (ev.xconfigure.width != wm.screen_w ||
                 ev.xconfigure.height != wm.screen_h)) {
                wm.screen_w = ev.xconfigure.width;
                wm.screen_h = ev.xconfigure.height;
                XResizeWindow(wm.dpy, wm.bar, (unsigned)wm.screen_w, BAR_H);
                wm_arrange(&wm);
                wm_draw_bar(&wm);
            }
            break;
        case PropertyNotify:
            if (ev.xproperty.atom == XA_WM_NAME || ev.xproperty.atom == wm.net_wm_name) {
                struct client *c = client_find(&wm, ev.xproperty.window);
                if (c) {
                    wm_fetch_title(&wm, ev.xproperty.window, c->name, sizeof c->name);
                    wm_draw_bar(&wm);
                }
            }
            break;
        }
    }

    /* clean exit: bring hidden windows back onscreen so no client is lost,
       then release everything */
    for (int i = 0; i < WORKSPACES; i++)
        for (struct client *c = wm.ws[i].clients; c; c = c->next) {
            if (i != wm.current) XMoveWindow(wm.dpy, c->win, 0, BAR_H);
            XMapWindow(wm.dpy, c->win);
        }
    XUngrabKey(wm.dpy, AnyKey, AnyModifier, wm.root);
    XDestroyWindow(wm.dpy, wm.bar);
    XFreeGC(wm.dpy, wm.gc);
    XFreeFont(wm.dpy, wm.font);
    XCloseDisplay(wm.dpy);
    return 0;
}
