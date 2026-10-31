/* xclient: minimal X client for integration tests. Opens a 200x150 window
   named after argv[1] and sits on the event loop. Sets _NET_WM_NAME too,
   like a modern client. */
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: xclient NAME\n"); return 2; }
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "xclient: no display\n"); return 1; }
    int scr = DefaultScreen(d);
    Window w = XCreateSimpleWindow(d, RootWindow(d, scr), 0, 0, 200, 150, 0,
                                   BlackPixel(d, scr), WhitePixel(d, scr));
    XStoreName(d, w, argv[1]);
    Atom net_name = XInternAtom(d, "_NET_WM_NAME", False);
    Atom utf8 = XInternAtom(d, "UTF8_STRING", False);
    XChangeProperty(d, w, net_name, utf8, 8, PropModeReplace,
                    (unsigned char *)argv[1], (int)strlen(argv[1]));
    if (argc > 2 && strcmp(argv[2], "--dialog") == 0) {
        Atom type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
        Atom dlg = XInternAtom(d, "_NET_WM_WINDOW_TYPE_DIALOG", False);
        XChangeProperty(d, w, type, XA_ATOM, 32, PropModeReplace,
                        (unsigned char *)&dlg, 1);
    }
    if (argc > 2 && strcmp(argv[2], "--dialog8") == 0) {
        /* malformed: the EWMH type property stored at format 8 instead of
           32. A WM that reads it as Atom* over-reads the buffer. */
        Atom type = XInternAtom(d, "_NET_WM_WINDOW_TYPE", False);
        const char junk[8] = "notatom!";
        XChangeProperty(d, w, type, XA_ATOM, 8, PropModeReplace,
                        (const unsigned char *)junk, 8);
    }
    XMapWindow(d, w);
    XFlush(d);
    for (;;) {
        XEvent ev;
        XNextEvent(d, &ev);
    }
    return 0;
}
