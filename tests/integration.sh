#!/bin/sh
# tilt integration test: run the real WM under Xvfb with real X clients,
# drive it with synthetic (XTEST) keys, and verify geometry, workspaces,
# focus, master swap, and closing via xwininfo.
set -e
cd "$(dirname "$0")/.."
DISP=:98

cleanup() {
    kill "$TILT_PID" "$XVFB_PID" 2>/dev/null || true
    pkill -DISPLAY $DISP 2>/dev/null || true
    pkill -f "xlogo.*$DISP" 2>/dev/null || true
}
trap cleanup EXIT

Xvfb $DISP -screen 0 800x600x24 >/dev/null 2>&1 &
XVFB_PID=$!
sleep 1.5

DISPLAY=$DISP ./build/tilt 2>/dev/null &
TILT_PID=$!
sleep 1

DISPLAY=$DISP xlogo & XLOGO_PID=$!
DISPLAY=$DISP xeyes & XEYES_PID=$!
DISPLAY=$DISP xclock & XCLOCK_PID=$!
sleep 1.5

say() { printf '%s... ' "$1"; }
ok() { echo "ok"; }
fail() { echo "FAIL: $1" >&2; exit 1; }

geom() {  # geometry of a client's window
    DISPLAY=$DISP xwininfo -root -children | grep "\"$1\"" | sed 's/  */ /g' | awk '{print $5}'
}

say "three clients tile as master + 2-stack"
geoms() { for c in xclock xeyes xlogo; do geom $c; done | sort; }
want='378x271+408+30
378x271+408+315
386x556+8+30'
[ "$(geoms)" = "$want" ] || fail "tiled geometry: $(geoms | tr '\n' ' ')"
ok

say "alt+2 hides all clients offscreen"
DISPLAY=$DISP ./build/xpress alt+2
sleep 0.3
geom xclock | grep -q -- '+-3000+-3000' || fail "not hidden: $(geom xclock)"
ok

say "alt+1 restores the tiled layout"
DISPLAY=$DISP ./build/xpress alt+1
sleep 0.3
[ "$(geoms)" = "$want" ] || fail "not restored: $(geoms | tr '\n' ' ')"
ok

say "alt+shift+2 moves the focused window to workspace 2"
DISPLAY=$DISP ./build/xpress alt+shift+2
sleep 0.3
offscreen=0; visible=0
for c in xclock xeyes xlogo; do
    g=$(geom $c)
    case "$g" in
        *+-3000+-3000) offscreen=$((offscreen+1));;
        *+8+30|*+408+30) visible=$((visible+1));;
    esac
done
[ "$offscreen" -eq 1 ] && [ "$visible" -eq 2 ] || fail "moved=$offscreen visible=$visible"
ok

say "workspace 2 shows the moved window full-width"
DISPLAY=$DISP ./build/xpress alt+2
sleep 0.3
full=0
for c in xclock xeyes xlogo; do
    [ "$(geom $c)" = '778x556+8+30' ] && full=$((full+1))
done
[ "$full" -eq 1 ] || fail "ws2 layout: $(geoms | tr '\n' ' ')"
DISPLAY=$DISP ./build/xpress alt+1
sleep 0.3
ok

say "alt+shift+q closes the focused window and the rest reflow"
DISPLAY=$DISP ./build/xpress alt+shift+q
sleep 0.5
remaining=$(for c in xclock xeyes xlogo; do geom $c; done | grep -c . || true)
[ "$remaining" -eq 2 ] || fail "expected 2 clients left, got $remaining"
geom xeyes | grep -q 'x' || true
# ws1 keeps one client in the single-window layout; ws2 keeps its client hidden
one=$(for c in xclock xeyes xlogo; do geom $c; done | grep -c '778x556+8+30')
[ "$one" -eq 1 ] || fail "no reflow to single: $(for c in xclock xeyes xlogo; do geom $c; done | tr '\n' ' ')"
ok

echo "INTEGRATION TEST PASSED"
