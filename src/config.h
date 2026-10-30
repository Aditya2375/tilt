#ifndef TILT_CONFIG_H
#define TILT_CONFIG_H

/* User-facing configuration: bindings, geometry, colors. */

#define MOD          Mod1Mask   /* Alt */
#define TERMINAL     "xterm"
#define BORDER       3
#define GAPS         8
#define BAR_H        22
#define WORKSPACES   9
#define MIN_WIN      40         /* windows never get smaller than this */
#define MFACT_STEP   50         /* master-resize step, per-mille of width */

#define COL_FOCUS    "#33ccff"
#define COL_NORMAL   "#444444"
#define COL_BAR_BG   "#1a1a2e"
#define COL_BAR_FG   "#e0e0e0"
#define COL_BAR_SEL  "#33ccff"

#endif
