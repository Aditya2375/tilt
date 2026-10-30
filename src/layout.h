#ifndef TILT_LAYOUT_H
#define TILT_LAYOUT_H

/* Master-stack tiling geometry, pure and display-free so it is unit
   testable without an X server. */

struct rect { int x, y, w, h; };

/* Tile n windows into a screen of w x h, with a bar of bar_h pixels at the
   top and gaps pixels around and between windows. Window 0 is the master
   (left, half width when n > 1); the rest stack vertically on the right.
   Writes n rects into out. */
void tile_layout(int n, int w, int h, int bar_h, int gaps, struct rect *out);

#endif
