#!/bin/sh
# Record the real WM for the browser demo page. tilt is an X11 window
# manager and a browser has no X server, so the demo is this honest
# capture: the actual tilt binary running under Xvfb, driven by the
# same XTEST harness (tests/xpress) the CI integration test uses,
# recorded with ffmpeg. Produces docs/demo.mp4.
set -e
cd "$(dirname "$0")/.."
OUT=docs/demo.mp4
DISP=:87
rm -f /tmp/.X87-lock /tmp/.X11-unix/X87 2>/dev/null || true

Xvfb $DISP -screen 0 800x600x24 >/dev/null 2>&1 &
XVFB=$!
sleep 1
export DISPLAY=$DISP

ffmpeg -y -loglevel error -f x11grab -video_size 800x600 -i $DISP \
    -c:v libx264 -pix_fmt yuv420p -movflags +faststart "$OUT" &
REC=$!
sleep 1

mkdir -p build/bin
printf '#!/bin/sh\nexec "%s/build/xclient" four\n' "$PWD" > build/bin/xterm
chmod +x build/bin/xterm
export PATH="$PWD/build/bin:$PATH"

./build/tilt 2>/dev/null &
TILT=$!
sleep 1.5

./build/xclient one & C1=$!
./build/xclient two & C2=$!
./build/xclient three & C3=$!
sleep 2.5

./build/xpress alt+2; sleep 2          # workspace 2 (empty)
./build/xpress alt+1; sleep 2          # back to the tiled trio
./build/xclient dlg --dialog & C4=$!
sleep 2.5                              # dialog floats centered
kill $C4 2>/dev/null; sleep 1.5
./build/xpress alt+l; sleep 1.5        # grow the master
./build/xpress alt+l; sleep 1.5
./build/xpress alt+shift+q; sleep 2    # close the focused window
./build/xpress alt+shift+q; sleep 2
./build/xpress alt+q; sleep 1.5        # quit

kill $REC $TILT $C1 $C2 $C3 $XVFB 2>/dev/null || true
wait $REC 2>/dev/null || true
echo "recorded $OUT"
