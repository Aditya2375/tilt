#!/bin/sh
# tilt integration test: run the real WM under Xvfb with real X clients
# (tests/xclient), drive it with XTEST key events (tests/xpress), and
# verify geometry, focus, workspaces, master swap, master resize, NumLock
# grabs, terminal spawn, closing, and clean exit on alt+q and SIGTERM.
set -e
cd "$(dirname "$0")/.."

say() { printf '%s... ' "$1"; }
ok() { echo "ok"; }
fail() { echo "FAIL: $1" >&2; kill $TILT_PID $XVFB_PID $CLIENTS 2>/dev/null || true; exit 1; }

DISP=""
for d in 91 92 93 94 95 96 97 98 99; do
    rm -f "/tmp/.X$d-lock" "/tmp/.X11-unix/X$d" 2>/dev/null
    Xvfb ":$d" -screen 0 800x600x24 >/dev/null 2>&1 &
    XVFB_PID=$!
    sleep 1
    if kill -0 "$XVFB_PID" 2>/dev/null && DISPLAY=":$d" xwininfo -root >/dev/null 2>&1; then
        DISP=":$d"
        break
    fi
    kill "$XVFB_PID" 2>/dev/null || true
done
[ -n "$DISP" ] || { echo "FAIL: no free Xvfb display" >&2; exit 1; }
export DISPLAY=$DISP

CLIENTS=""
TILT_PID=""

geom() { xwininfo -root -children | grep "\"$1\"" | sed 's/  */ /g' | awk '{print $4}'; }
geoms() { for c in one two three; do geom $c; done | sort; }
active() { xprop -root _NET_ACTIVE_WINDOW | awk '{print $NF}'; }
winid() { xwininfo -root -children | grep "\"$1\"" | awk '{print $1}'; }

wait_cond() {  # wait_cond SECONDS CONDITION-STRING
    n=$(($1 * 10))
    while [ "$n" -gt 0 ]; do
        if eval "$2" >/dev/null 2>&1; then return 0; fi
        sleep 0.1
        n=$((n - 1))
    done
    return 1
}

# a fake "xterm" on PATH so the terminal-spawn binding has something to run
mkdir -p build/bin
printf '#!/bin/sh\nexec "%s/build/xclient" four\n' "$PWD" > build/bin/xterm
chmod +x build/bin/xterm
export PATH="$PWD/build/bin:$PATH"

./build/tilt 2>/tmp/tilt-test.err &
TILT_PID=$!
wait_cond 5 'xwininfo -root -children | grep -q "\"tilt-bar\""' || fail "tilt did not start"

./build/xclient one & CLIENTS="$CLIENTS $!"
./build/xclient two & CLIENTS="$CLIENTS $!"
./build/xclient three & CLIENTS="$CLIENTS $!"

want='378x271+408+30
378x271+408+315
386x556+8+30'
wait_cond 5 '[ "$(geoms)" = "$want" ]' || fail "tiled geometry: [$(geoms | tr '\n' ' ')] dump: [$(xwininfo -root -children | tail -8 | tr '\n' '|')]"
say "three clients tile as master + 2-stack"; ok

say "the focused client is the master"
three_id=$(winid three); two_id=$(winid two)
master_geom() { xwininfo -root -children | grep "^ *$(active) " | sed 's/  */ /g' | awk '{print $4}'; }
[ "$(master_geom)" = '386x556+8+30' ] || fail "focused client not master: $(master_geom)"
ok

say "alt+2 hides all clients offscreen"
./build/xpress alt+2
wait_cond 3 'geom one | grep -q -- "+-3000+-3000"' || fail "not hidden: $(geom one)"
ok

say "alt+1 restores the tiled layout"
./build/xpress alt+1
wait_cond 5 '[ "$(geoms)" = "$want" ]' || fail "not restored: $(geoms | tr '\n' ' ')"
ok

say "alt+shift+2 moves the focused window to workspace 2"
fid=$(active)
fgeom() { xwininfo -root -children | grep "^ *$fid " | sed 's/  */ /g' | awk '{print $4}'; }
./build/xpress alt+shift+2
wait_cond 3 'fgeom | grep -q -- "+-3000+-3000"' || fail "not moved: $(fgeom)"
visible=$(xwininfo -root -children | sed 's/  */ /g' | grep -cE '^ *0x[0-9a-f]+ "(one|two|three)": \(\) [0-9]+x[0-9]+\+[0-9]+\+30 ')
[ "$visible" -eq 2 ] || fail "visible=$visible"
ok

say "workspace 2 shows the moved window full-width"
./build/xpress alt+2
wait_cond 3 '[ "$(fgeom)" = "778x556+8+30" ]' || fail "ws2 layout: $(fgeom)"
./build/xpress alt+shift+1   # move it back to ws1 (it is focused on ws2)
wait_cond 3 'fgeom | grep -q -- "+-3000+-3000"'
./build/xpress alt+1
wait_cond 5 '[ "$(geoms)" = "$want" ]' || fail "back: $(geoms | tr '\n' ' ')"
ok

say "alt+j / alt+k cycle focus"
f0=$(active)
./build/xpress alt+j
wait_cond 3 '[ "$(active)" != "$f0" ]' || fail "alt+j did not move focus"
f1=$(active)
./build/xpress alt+k
wait_cond 3 '[ "$(active)" = "$f0" ]' || fail "alt+k did not return focus: $(active) want $f0"
ok

say "alt+Return makes the focused client master"
./build/xpress alt+j
wait_cond 3 '[ "$(active)" != "$f0" ]' || fail "alt+j for swap test"
fid=$(active)
./build/xpress alt+enter
wait_cond 3 '[ "$(xwininfo -root -children | grep "^ *$fid " | sed "s/  */ /g" | awk "{print \$4}")" = "386x556+8+30" ]'     || fail "swap: $(xwininfo -root -children | grep "^ *$fid ")"
ok

say "alt+l / alt+h resize the master area"
master_w() { xwininfo -root -children | sed 's/  */ /g' | awk '$4 ~ /x556\+8\+30$/ {sub(/x.*/, "", $4); print $4}' | head -1; }
before=$(master_w)
./build/xpress alt+l
wait_cond 3 '[ "$(master_w)" != "$before" ]' || fail "alt+l no resize"
wider=$(master_w)
[ "$wider" -gt "$before" ] || fail "alt+l shrank master: $before -> $wider"
./build/xpress alt+h
wait_cond 3 '[ "$(master_w)" = "$before" ]' || fail "alt+h did not restore: $wider vs $before"
ok

say "bindings still work with NumLock on"
./build/xpress numlock
./build/xpress alt+2
wait_cond 3 'geom one | grep -q -- "+-3000+-3000"' || fail "NumLock blocked alt+2"
./build/xpress numlock
./build/xpress alt+1
wait_cond 5 '[ "$(geoms)" = "$want" ]' || fail "back from NumLock test: $(geoms | tr '\n' ' ')"
ok

say "alt+shift+Return spawns the configured terminal"
./build/xpress alt+shift+enter
wait_cond 5 'xwininfo -root -children | grep -q "\"four\""' || fail "no terminal window"
./build/xpress alt+shift+q   # close it again to keep later geometry simple
wait_cond 3 '! xwininfo -root -children | grep -q "\"four\""' || fail "terminal did not close"
ok

say "alt+shift+q closes the focused window and the rest reflow"
./build/xpress alt+shift+q
wait_cond 3 '[ "$(for c in one two three; do geom $c; done | grep -c .)" -eq 2 ]' \
    || fail "close: $(for c in one two three; do geom $c; done | tr '\n' ' ')"
remaining=$(for c in one two three; do geom $c; done | grep . | sort)
want2='378x556+408+30
386x556+8+30'
[ "$remaining" = "$want2" ] || fail "no reflow to 2-window tile: $(echo "$remaining" | tr '\n' ' ')"
ok

say "alt+q exits cleanly and hidden windows come back onscreen"
./build/xpress alt+shift+3   # move the focused window to ws3 (hidden)
sleep 0.3
./build/xpress alt+q
gone=0; n=50
while [ "$n" -gt 0 ]; do kill -0 $TILT_PID 2>/dev/null || { gone=1; break; }; sleep 0.1; n=$((n-1)); done
[ "$gone" -eq 1 ] || fail "tilt did not exit on alt+q"
sleep 0.5
offscreen=0
for c in one two three; do
    case "$(geom $c)" in *+-3000*) offscreen=$((offscreen+1));; esac
done
[ "$offscreen" -eq 0 ] || fail "$offscreen windows lost offscreen after quit"
kill $CLIENTS 2>/dev/null || true; CLIENTS=""
ok

say "SIGTERM also shuts down cleanly"
./build/tilt 2>/tmp/tilt-test.err &
TILT_PID=$!
wait_cond 5 'xwininfo -root -children | grep -q "\"tilt-bar\""' || fail "restart failed"
kill -TERM $TILT_PID
./build/xclient five & CLIENTS="$CLIENTS $!"   # any event wakes the loop
gone=0; n=50
while [ "$n" -gt 0 ]; do kill -0 $TILT_PID 2>/dev/null || { gone=1; break; }; sleep 0.1; n=$((n-1)); done
[ "$gone" -eq 1 ] || fail "tilt did not exit on SIGTERM"
ok

kill $CLIENTS $XVFB_PID 2>/dev/null || true
wait 2>/dev/null
echo "INTEGRATION TEST PASSED"
