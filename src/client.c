#include "tilt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct client *client_add(struct wm *wm, Window win) {
    struct workspace *ws = &wm->ws[wm->current];
    struct client *c = calloc(1, sizeof *c);
    if (!c) { perror("calloc"); exit(1); }
    c->win = win;
    c->next = ws->clients;
    ws->clients = c;
    char *name = NULL;
    if (XFetchName(wm->dpy, win, &name) && name) {
        snprintf(c->name, sizeof c->name, "%s", name);
        XFree(name);
    }
    return c;
}

struct client *client_find(struct wm *wm, Window win) {
    for (int i = 0; i < WORKSPACES; i++)
        for (struct client *c = wm->ws[i].clients; c; c = c->next)
            if (c->win == win) return c;
    return NULL;
}

int client_count(struct workspace *ws) {
    int n = 0;
    for (struct client *c = ws->clients; c; c = c->next) n++;
    return n;
}

void client_remove(struct wm *wm, Window win) {
    for (int i = 0; i < WORKSPACES; i++) {
        struct workspace *ws = &wm->ws[i];
        struct client **pp = &ws->clients;
        while (*pp) {
            if ((*pp)->win == win) {
                struct client *dead = *pp;
                *pp = dead->next;
                if (ws->focused == dead)
                    ws->focused = *pp ? *pp : ws->clients;
                free(dead);
                return;
            }
            pp = &(*pp)->next;
        }
    }
}

void client_focus(struct wm *wm, struct client *c) {
    struct workspace *ws = &wm->ws[wm->current];
    ws->focused = c;
    if (c) {
        XSetWindowBorder(wm->dpy, c->win, wm->col_focus);
        XRaiseWindow(wm->dpy, c->win);
        XSetInputFocus(wm->dpy, c->win, RevertToParent, CurrentTime);
    }
    /* unfocus everyone else on this workspace */
    for (struct client *k = ws->clients; k; k = k->next)
        if (k != c) XSetWindowBorder(wm->dpy, k->win, wm->col_normal);
    wm_draw_bar(wm);
}

void client_move_to_master(struct wm *wm, struct client *c) {
    if (!c) return;
    struct workspace *ws = &wm->ws[wm->current];
    if (ws->clients == c) return;   /* already master */
    struct client **pp = &ws->clients;
    while (*pp && *pp != c) pp = &(*pp)->next;
    if (!*pp) return;
    *pp = c->next;
    c->next = ws->clients;
    ws->clients = c;
}

void client_move_to_ws(struct wm *wm, struct client *c, int target) {
    if (!c || target == wm->current) return;
    struct workspace *from = &wm->ws[wm->current];
    struct client **pp = &from->clients;
    while (*pp && *pp != c) pp = &(*pp)->next;
    if (!*pp) return;
    *pp = c->next;
    if (from->focused == c) from->focused = from->clients;
    struct workspace *to = &wm->ws[target];
    c->next = to->clients;
    to->clients = c;
    XMoveWindow(wm->dpy, c->win, -3000, -3000);  /* hide, do not unmap */
    client_focus(wm, from->clients);
}
