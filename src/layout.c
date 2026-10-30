#include "layout.h"

static int clampi(int v, int lo, int hi) {
    return v < lo ? lo : v > hi ? hi : v;
}

void tile_layout(int n, int w, int h, int bar_h, int gaps, int mfact,
                 struct rect *out) {
    if (n <= 0) return;
    mfact = clampi(mfact, 100, 900);
    int ax = gaps;
    int ay = bar_h + gaps;
    int aw = w - 2 * gaps;
    int ah = h - bar_h - 2 * gaps;
    if (aw < 1) aw = 1;
    if (ah < 1) ah = 1;

    if (n == 1) {
        out[0] = (struct rect){ax, ay, aw, ah};
        return;
    }
    int master_w = clampi(aw * mfact / 1000, 1, aw - gaps - 1);
    out[0] = (struct rect){ax, ay, master_w, ah};
    int stack_n = n - 1;
    int stack_x = ax + master_w + gaps;
    int stack_w = aw - master_w - gaps;
    if (stack_w < 1) stack_w = 1;
    int each = (ah - gaps * (stack_n - 1)) / stack_n;
    if (each < 1) each = 1;
    for (int i = 0; i < stack_n; i++) {
        int y = ay + i * (each + gaps);
        int hh = (i == stack_n - 1) ? (ay + ah - y) : each;
        if (hh < 1) hh = 1;
        if (y + hh > ay + ah) y = ay + ah - hh;   /* overlap up, never negative */
        if (y < ay) y = ay;
        out[i + 1] = (struct rect){stack_x, y, stack_w, hh};
    }
}
