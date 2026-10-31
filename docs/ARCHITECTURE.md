# tilt architecture

## Shape

```text
X events -> event loop (wm.c) -> client list ops (client.c) -> layout (layout.c) -> XMoveResizeWindow
```

`layout.c` is a pure function - n windows plus screen, bar, gap, and
master-ratio dimensions produce n rects, each clamped to at least one
pixel (overflowing stacks overlap instead of producing invalid geometry).
It knows nothing about X, which is why 73 assertions pin it without a
server. Everything X lives in `wm.c` and `client.c`.

## Managing windows

Selecting `SubstructureRedirectMask` on the root makes tilt the window
manager: map requests from clients arrive as `MapRequest` events instead
of mapping directly. New windows get a border, are prepended to the
current workspace's client list (list head = master), tiled, and focused -
unless they are transients (`WM_TRANSIENT_FOR`) or EWMH dialogs, which
float centered at half the screen size and stay out of the tiling count.
`UnmapNotify` and `DestroyNotify` remove the client and reflow.

**Why hiding moves offscreen instead of unmapping.** If the WM unmaps a
window itself, the resulting `UnmapNotify` is indistinguishable from the
client withdrawing, and the client would be dropped from management.
tilt moves hidden windows to (-3000, -3000): no event, no ambiguity, and
the client's own state (scrollback, contents) survives workspace switches.

## Focus

One focused client per workspace. Focus paints the border cyan, raises the
window, sets the X input focus, and publishes the window id as
`_NET_ACTIVE_WINDOW` on the root; every other managed window on the
workspace gets the dim border. `Alt+j/k` walk the client chain, wrapping
at both ends. The bar redraws on every focus change and on title changes,
tracked through both `WM_NAME` and UTF-8 `_NET_WM_NAME` (the latter wins
when present).

## The bar

A plain top-level window (named `tilt-bar` so tests can find it), 22px
high, drawn with the core `fixed` font through one GC: tag strip on the
left (current highlighted, occupied bright, empty dim), focused title on
the right, clipped to the space left after the tags. The text baseline is
computed from the font metrics, not hardcoded. Layout reserves `BAR_H` at
the top, so windows never slide under it.

## Closing

`Alt+Shift+q` prefers the polite path: if the client advertises
`WM_DELETE_WINDOW` in `WM_PROTOCOLS` it receives a `ClientMessage`;
otherwise `XKillClient` force-closes the connection. Either way the
window's destroy/unmap events reflow the layout.

## Keys

All bindings are passive grabs on the root with the Alt modifier, grabbed
under every combination of Shift, Caps Lock, and Num Lock (Mod2), so they
fire regardless of lock state - the integration test proves this by
toggling Num Lock on with XTEST and pressing `Alt+2`. Synthetic keys from
XTEST take the same path, which is what makes the integration test drive
the WM exactly as a keyboard would.

## Errors, signals, and shutdown

The startup error handler exits only for the one fatal case: another WM
already owns the redirect. After that a logging handler swallows the X
errors that dying windows and stale grabs produce - restoring the default
handler would make any of them fatal. SIGTERM and SIGINT set a flag with
`SA_RESTART` so a signal can never interrupt an X read into a fatal XIO
error; the loop exits at the next event. Spawned terminals are reaped
with `SIGCHLD` ignored. On quit, tilt ungrabs all keys, brings every
hidden window back onscreen (nothing is lost at (-3000, -3000)), destroys
the bar, and frees the GC, font, and display connection. A root
`ConfigureNotify` (screen resize) re-lays-out and resizes the bar.
