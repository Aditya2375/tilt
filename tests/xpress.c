/* xpress: send key events to the X server via XTEST, for driving a WM
   in tests. usage: xpress alt+j alt+2 ... */
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
    Display *d = XOpenDisplay(NULL);
    if (!d) { fprintf(stderr, "xpress: no display\n"); return 1; }
    for (int i = 1; i < argc; i++) {
        char *spec = argv[i];
        unsigned mods = 0;
        char *key = spec;
        char *plus;
        while ((plus = strchr(key, '+'))) {
            *plus = '\0';
            if (strcmp(key, "alt") == 0) mods |= Mod1Mask;
            else if (strcmp(key, "shift") == 0) mods |= ShiftMask;
            else if (strcmp(key, "ctrl") == 0) mods |= ControlMask;
            key = plus + 1;
        }
        KeySym sym;
        if (strlen(key) == 1) sym = (KeySym)key[0];
        else if (strcmp(key, "enter") == 0) sym = XK_Return;
        else if (strcmp(key, "space") == 0) sym = XK_space;
        else { fprintf(stderr, "xpress: unknown key %s\n", key); continue; }
        KeyCode kc = XKeysymToKeycode(d, sym);
        for (int m = 1; m <= 4; m <<= 1)   /* press modifiers */
            ;
        if (mods & Mod1Mask) { KeyCode alt = XKeysymToKeycode(d, XK_Alt_L); XTestFakeKeyEvent(d, alt, True, 0); }
        if (mods & ShiftMask) { KeyCode sh = XKeysymToKeycode(d, XK_Shift_L); XTestFakeKeyEvent(d, sh, True, 0); }
        if (mods & ControlMask) { KeyCode ct = XKeysymToKeycode(d, XK_Control_L); XTestFakeKeyEvent(d, ct, True, 0); }
        XTestFakeKeyEvent(d, kc, True, 0);
        XTestFakeKeyEvent(d, kc, False, 0);
        if (mods & Mod1Mask) { KeyCode alt = XKeysymToKeycode(d, XK_Alt_L); XTestFakeKeyEvent(d, alt, False, 0); }
        if (mods & ShiftMask) { KeyCode sh = XKeysymToKeycode(d, XK_Shift_L); XTestFakeKeyEvent(d, sh, False, 0); }
        if (mods & ControlMask) { KeyCode ct = XKeysymToKeycode(d, XK_Control_L); XTestFakeKeyEvent(d, ct, False, 0); }
        XFlush(d);
        usleep(120000);
    }
    XCloseDisplay(d);
    return 0;
}
