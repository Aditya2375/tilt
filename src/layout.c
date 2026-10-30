#include "layout.h"

void tile_layout(int n, int w, int h, int bar_h, int gaps, struct rect *out) {
    if (n <= 0) return;
    int ax = gaps;
    int ay = bar_h + gaps;
    int aw = w - 2 * gaps;
    int ah = h - bar_h - 2 * gaps;

    if (n == 1) {
        out[0] = (struct rect){ax, ay, aw, ah};
        return;
    }
    int master_w = aw / 2;
    out[0] = (struct rect){ax, ay, master_w, ah};
    int stack_n = n - 1;
    int stack_x = ax + master_w + gaps;
    int stack_w = aw - master_w - gaps;
    int each = (ah - gaps * (stack_n - 1)) / stack_n;
    for (int i = 0; i < stack_n; i++) {
        int y = ay + i * (each + gaps);
        int hh = (i == stack_n - 1) ? (ay + ah - y) : each;  /* last takes the remainder */
        out[i + 1] = (struct rect){stack_x, y, stack_w, hh};
    }
}
