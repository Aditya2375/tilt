#!/bin/sh
# Fail when a numeric claim in the docs drifts from reality.
set -e
cd "$(dirname "$0")/.."
mkdir -p build
cc -std=gnu11 -O2 -g -Wall -Wextra -Werror -Isrc \
   tests/unit/test_layout.c src/layout.c -o build/test_layout
got=$(./build/test_layout | grep -o '^[0-9]* checks passed' | grep -o '^[0-9]*')
for doc in README.md docs/ARCHITECTURE.md; do
    claimed=$(grep -o '[0-9]* \(checks\|assertions\)' "$doc" | grep -o '^[0-9]*' | head -1)
    if [ -n "$claimed" ] && [ "$claimed" != "$got" ]; then
        echo "DOC DRIFT: $doc claims $claimed checks, suite runs $got" >&2
        exit 1
    fi
done
echo "docs match: $got checks"
