#ifndef TILT_LAYOUT_H
#define TILT_LAYOUT_H

/* Master-stack tiling geometry, pure and display-free so it is unit
   testable without an X server. */

struct rect { int x, y, w, h; };

/* Tile n windows into a screen of w x h, with a bar of bar_h pixels at the
   top and gaps pixels around and between windows. Window 0 is the master:
   left side, mfact per-mille of the area width (500 = half). The rest
   stack vertically on the right. No rect is ever smaller than 1x1; when
   the stack cannot fit, later windows overlap rather than produce a
   zero or negative height. Writes n rects into out. */
void tile_layout(int n, int w, int h, int bar_h, int gaps, int mfact,
                 struct rect *out);

#endif
