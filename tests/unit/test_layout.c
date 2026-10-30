#include "../harness.h"
#include "../../src/layout.h"

int main(void) {
    struct rect r[80];

    /* single window takes the whole area below the bar */
    tile_layout(1, 800, 600, 22, 8, 500, r);
    CHECK(r[0].x == 8 && r[0].y == 30 && r[0].w == 784 && r[0].h == 562,
          "single window geometry");

    /* two windows split in half */
    tile_layout(2, 800, 600, 22, 8, 500, r);
    CHECK(r[0].w == 392, "master takes half");
    CHECK(r[1].x == 8 + 392 + 8 && r[1].w == 800 - 16 - 392 - 8, "stack takes the rest");

    /* mfact moves the split */
    tile_layout(2, 800, 600, 22, 8, 700, r);
    CHECK(r[0].w == 784 * 700 / 1000, "mfact 700 widens master");
    tile_layout(2, 800, 600, 22, 8, 5, r);
    CHECK(r[0].w >= 1, "tiny mfact clamps, master survives");

    /* 60 windows on a small screen: every rect sane, nothing negative */
    tile_layout(60, 800, 600, 22, 8, 500, r);
    for (int i = 0; i < 60; i++) {
        CHECK(r[i].w >= 1 && r[i].h >= 1, "no zero or negative rect at 60 windows");
        if (r[i].w < 1 || r[i].h < 1) break;
    }
    tile_layout(60, 800, 600, 22, 8, 500, r);
    int overlap_ok = 1;
    for (int i = 1; i < 60; i++)
        if (r[i].y + r[i].h > 600) { overlap_ok = 0; break; }
    CHECK(overlap_ok, "stack stays on screen even when it must overlap");

    /* absurd screens do not blow up */
    tile_layout(3, 40, 40, 22, 8, 500, r);
    for (int i = 0; i < 3; i++)
        CHECK(r[i].w >= 1 && r[i].h >= 1, "tiny screen still yields rects");

    TEST_RESULTS();
}
