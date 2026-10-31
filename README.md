# tilt

A tiling window manager for X11 in C, on raw Xlib - no libraries beyond
the X client library itself. Master-stack layout with an adjustable
split, nine workspaces, a status bar, keyboard-driven focus and window
movement, floating for dialogs, and graceful ICCCM window closing.
Windows are hidden by moving offscreen, never unmapped, so clients keep
their state across workspace switches.

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

All bindings use **Alt** as the modifier and work with Caps Lock and
Num Lock in any state.

| Keys | Action |
|---|---|
| `Alt+j` / `Alt+k` | focus next / previous window |
| `Alt+h` / `Alt+l` | shrink / grow the master area |
| `Alt+Enter` | make the focused window the master |
| `Alt+Shift+Enter` | open a terminal (`TERMINAL` in `src/config.h`) |
| `Alt+Shift+q` | close the focused window (WM_DELETE, else kill) |
| `Alt+1..9` | switch to workspace 1..9 |
| `Alt+Shift+1..9` | move the focused window to workspace 1..9 |
| `Alt+q` | quit tilt (hidden windows are brought back onscreen) |

The bar shows all nine tags; the current tag is highlighted, occupied tags
are bright, and the focused window's title sits on the right, clipped so
it never covers the tags.

## Layout

New windows become the master. The master takes a configurable share of
the width (50% by default, adjustable with `Alt+h/l` in 5% steps between
10% and 90%); everything else stacks in equal rows on the right, the last
row absorbing the rounding remainder so windows reach the bottom edge
exactly. 8px gaps, 3px borders; the focused window's border is cyan.
Transient windows and `_NET_WM_WINDOW_TYPE_DIALOG` float centered at half
the screen size instead of tiling. No window is ever sized below one
pixel: when more windows are open than the stack can fit, later rows
overlap rather than produce an invalid geometry.

## Configuration

`src/config.h`: modifier, terminal command, border/gap/bar sizes, master
step, and all colors. Rebuild after editing.

## Architecture and tests

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md). `./tests/run-tests.sh`:

- **Unit** - 73 checks on the pure layout function: exact geometry,
  master-ratio bounds, no zero or negative rects with 60 windows on a
  small screen, overlap instead of overflow, absurdly small screens.
- **Integration** - runs the real WM under Xvfb with real client windows
  (`tests/xclient.c`, built by the suite), drives it with synthetic XTEST
  keys (`tests/xpress.c`, needs libXtst), and asserts tiled geometry,
  focus via `_NET_ACTIVE_WINDOW`, workspace hiding and restore, moving
  windows between workspaces, master swap, master resize, bindings under
  Num Lock, terminal spawn, closing with reflow, and clean exit on both
  `Alt+q` and SIGTERM - all through `xwininfo`/`xprop` with polling
  instead of fixed sleeps.

## Tested / not tested

Tested here: everything in the integration suite above on a real X server
(Xvfb), including focus, workspace switches, master resizing, terminal
spawning, and shutdown behavior.

Not tested: multiple monitors (no Xinerama; RandR screen-size changes are
handled via root `ConfigureNotify`), mouse-driven focus or resizing,
compositing effects, and full EWMH compliance (tilt publishes
`_NET_ACTIVE_WINDOW` and reads `_NET_WM_NAME`, but panels and pagers get
no other hints).

Known limit: window titles are fetched correctly (UTF-8 `_NET_WM_NAME`
wins over `WM_NAME`), but the bar draws them with the core X "fixed"
font, so titles outside Latin-1 render as mojibake. Proper rendering
needs an Xft or fontset-based bar, which tilt deliberately does not
depend on.
