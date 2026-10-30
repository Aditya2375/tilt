#!/bin/sh
# tilt test suite: layout unit checks, then the real-WM Xvfb integration
# test (skips with a message when Xvfb/xlogo/xwd are absent).
set -e
cd "$(dirname "$0")/.."
OUT=build/tests
mkdir -p "$OUT"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
    tests/unit/test_layout.c src/layout.c -o "$OUT/test_layout"
"$OUT/test_layout"

cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"${X11_INC:-/usr/include}" \
    tests/xpress.c -o build/xpress -l:libX11.so.6 -l:libXtst.so.6 2>/dev/null || \
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"${X11_INC:-/usr/include}" \
    tests/xpress.c -o build/xpress -lX11 -lXtst

if command -v Xvfb >/dev/null && command -v xlogo >/dev/null && command -v xwininfo >/dev/null; then
    ./tests/integration.sh
else
    echo "SKIP: integration test needs Xvfb, xlogo, xeyes, xclock, xwininfo"
fi
echo "ALL TESTS PASSED"
