#!/bin/sh
# tilt test suite: layout unit checks, then the real-WM Xvfb integration
# test (skips with a message when Xvfb or the XTEST extension is absent).
set -e
cd "$(dirname "$0")/.."
OUT=build/tests
mkdir -p "$OUT"

${CC:-cc} -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
    tests/unit/test_layout.c src/layout.c -o "$OUT/test_layout"
"$OUT/test_layout"

XINC="${X11_INC:-/usr/include}"
${CC:-cc} -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"$XINC" \
    tests/xpress.c -o build/xpress -l:libX11.so.6 -l:libXtst.so.6 2>/dev/null || \
${CC:-cc} -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"$XINC" \
    tests/xpress.c -o build/xpress -lX11 -lXtst
${CC:-cc} -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"$XINC" \
    tests/xclient.c -o build/xclient -l:libX11.so.6 2>/dev/null || \
${CC:-cc} -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc -I"$XINC" \
    tests/xclient.c -o build/xclient -lX11

if command -v Xvfb >/dev/null && command -v xwininfo >/dev/null && command -v xprop >/dev/null; then
    ./tests/integration.sh
else
    echo "SKIP: integration test needs Xvfb, xwininfo, xprop"
fi
echo "ALL TESTS PASSED"
