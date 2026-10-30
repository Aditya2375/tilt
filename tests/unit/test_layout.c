#include "../harness.h"
#include "layout.h"

static int overlap(struct rect a, struct rect b) {
    return a.x < b.x + b.w && b.x < a.x + a.w &&
           a.y < b.y + b.h && b.y < a.y + a.h;
}

static int inside(struct rect r, int w, int h) {
    return r.x >= 0 && r.y >= 0 && r.x + r.w <= w && r.y + r.h <= h;
}

int main(void) {
    struct rect r[9];

    /* one window: full area below the bar, minus gaps */
    tile_layout(1, 800, 600, 22, 8, r);
    CHECK(r[0].x == 8 && r[0].y == 30, "single: origin below bar with gaps");
    CHECK(r[0].w == 784 && r[0].h == 562, "single: fills the area");

    /* two: master left half, stack right half */
    tile_layout(2, 800, 600, 22, 8, r);
    CHECK(r[0].w == 392 && r[0].h == 562, "master half width, full height");
    CHECK(r[1].x == 408 && r[1].y == 30, "stack right of master plus gap");
    CHECK(r[1].w == 384 && r[1].h == 562, "stack takes the rest");

    /* three: stack splits vertically; last takes the remainder */
    tile_layout(3, 800, 600, 22, 8, r);
    CHECK(r[1].y == 30 && r[2].y == 315, "stack rows spaced");
    CHECK(r[2].y + r[2].h == 592, "last row reaches the bottom");
    CHECK(r[1].h + 8 + r[2].h == 562, "rows plus gap fill the height");

    /* no overlaps, everything inside, for 1..9 windows */
    for (int n = 1; n <= 9; n++) {
        tile_layout(n, 800, 600, 22, 8, r);
        char msg[64];
        for (int i = 0; i < n; i++) {
            snprintf(msg, sizeof msg, "n=%d win %d inside", n, i);
            CHECK(inside(r[i], 800, 600), msg);
            for (int j = i + 1; j < n; j++) {
                snprintf(msg, sizeof msg, "n=%d wins %d/%d overlap", n, i, j);
                CHECK(!overlap(r[i], r[j]), msg);
            }
        }
    }

    /* zero gaps and zero bar still work */
    tile_layout(2, 800, 600, 0, 0, r);
    CHECK(r[0].x == 0 && r[0].y == 0 && r[0].w == 400, "no chrome: exact halves");
    CHECK(r[1].x == 400 && r[1].w == 400, "no chrome: stack half");

    /* odd screens: every pixel still inside */
    for (int n = 1; n <= 9; n++) {
        tile_layout(n, 801, 601, 22, 8, r);
        for (int i = 0; i < n; i++)
            CHECK(inside(r[i], 801, 601), "odd screen inside");
    }

    TEST_RESULTS();
}
