# tilt

A tiling window manager for X11 in C, on raw Xlib - no libraries beyond
the X client library itself. Master-stack layout, nine workspaces, a
status bar, keyboard-driven focus and window movement, and graceful
ICCCM window closing. Windows are hidden by moving offscreen, never
unmapped, so clients keep their state across workspace switches.

## Quick start

```bash
make          # needs libX11 headers (X11_INC=/path/to/include to override)
```

Run it from `.xinitrc`, or inside a nested server:

```bash
Xvfb :99 -screen 0 1280x800x24 &
DISPLAY=:99 ./build/tilt &
DISPLAY=:99 xterm &
```

## Keys

All bindings use **Alt** as the modifier.

| Keys | Action |
|---|---|
| `Alt+j` / `Alt+k` | focus next / previous window |
| `Alt+Enter` | make the focused window the master |
| `Alt+Shift+q` | close the focused window (WM_DELETE, else kill) |
| `Alt+1..9` | switch to workspace 1..9 |
| `Alt+Shift+1..9` | move the focused window to workspace 1..9 |
| `Alt+q` | quit tilt |

The bar shows all nine tags; the current tag is highlighted, occupied tags
are bright, and the focused window's title sits on the right.

## Layout

New windows become the master (left half). Everything else stacks in equal
rows on the right; the last row absorbs the rounding remainder so windows
always reach the bottom edge exactly. 8px gaps, 3px borders; the focused
window's border is cyan.

## Architecture and tests

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). `./tests/run-tests.sh`:

- **Unit** - 220 checks on the pure layout function: exact geometry,
  no overlaps, everything inside the screen for 1-9 windows, zero-chrome
  and odd-size screens.
- **Integration** - runs the real WM under Xvfb with real clients
  (xlogo, xeyes, xclock), drives it with synthetic XTEST keys
  (`tests/xpress.c`), and asserts tiled geometry, workspace hiding and
  restore, moving windows between workspaces, and closing with reflow,
  all through `xwininfo`.

## Tested / not tested

Tested here: everything in the integration suite above on a real X server
(Xvfb), including a screenshot review of bar, borders, and tiling.

Not tested: multiple monitors (Xinerama/RandR), floating windows and
dialogs (everything tiles; a dialog gets a full tile), EWMH compliance
beyond what the bar needs (no `_NET_*` hints, so panels and pagers won't
talk to it), mouse-driven focus or resizing, and compositing effects.
