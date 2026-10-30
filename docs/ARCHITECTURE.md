# tilt architecture

## Shape

```text
X events -> event loop (wm.c) -> client list ops (client.c) -> layout (layout.c) -> XMoveResizeWindow
```

`layout.c` is a pure function - n windows plus screen, bar, and gap
dimensions produce n rects. It knows nothing about X, which is why 220
assertions pin it without a server. Everything X lives in `wm.c` and
`client.c`.

## Managing windows

Selecting `SubstructureRedirectMask` on the root makes tilt the window
manager: map requests from clients arrive as `MapRequest` events instead
of mapping directly. New windows get a border, are prepended to the
current workspace's client list (list head = master), tiled, and focused.
`UnmapNotify` and `DestroyNotify` remove the client and reflow.

**Why hiding moves offscreen instead of unmapping.** If the WM unmaps a
window itself, the resulting `UnmapNotify` is indistinguishable from the
client withdrawing, and the client would be dropped from management.
tilt moves hidden windows to (-3000, -3000): no event, no ambiguity, and
the client's own state (scrollback, contents) survives workspace switches.

## Focus

One focused client per workspace. Focus paints the border cyan, raises the
window, and sets the X input focus; every other managed window on the
workspace gets the dim border. `Alt+j/k` walk the client chain, wrapping
at both ends. The bar redraws on every focus change and on `WM_NAME`
property changes.

## The bar

A plain override-redirect-free top window, 22px high, drawn with the
core `fixed` font through one GC: tag strip on the left (current
highlighted, occupied bright, empty dim), focused title on the right.
Layout reserves `BAR_H` at the top, so windows never slide under it.

## Closing

`Alt+Shift+q` prefers the polite path: if the client advertises
`WM_DELETE_WINDOW` in `WM_PROTOCOLS` it receives a `ClientMessage`;
otherwise `XKillClient` force-closes the connection. Either way the
window's destroy/unmap events reflow the layout.

## Keys

All bindings are passive grabs on the root with the Alt modifier, grabbed
with every combination of Shift and Lock (caps) so they fire in real
conditions. Synthetic keys from XTEST take the same path, which is what
makes the integration test drive the WM exactly as a keyboard would.
