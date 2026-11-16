#!/usr/bin/env sh
# Build the SFML example.
#
# The example uses shim.c because AGC currently passes struct arguments to
# `extern "c"` functions as pointers, while CSFML expects sfVideoMode/sfColor
# by value.

set -eu

here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
root=$(CDPATH= cd -- "$here/../.." && pwd)
agc=${AGC:-"$root/build/agc"}
out_dir="$here/build"

mkdir -p "$out_dir"

cc -std=c11 -O2 -I "$here/headers" -c "$here/shim.c" -o "$out_dir/shim.o"

if command -v pkg-config >/dev/null 2>&1 &&
   pkg-config --exists csfml-graphics csfml-window csfml-system; then
  csfml_libs=$(pkg-config --libs csfml-graphics csfml-window csfml-system)
else
  csfml_libs="-L$here/lib -lcsfml-graphics -lcsfml-window -lcsfml-system"
fi

# shellcheck disable=SC2086
"$agc" "$here/src/main.ag" \
  --link-arg "$out_dir/shim.o" \
  $csfml_libs \
  --link-arg "-Wl,-rpath,$here/lib" \
  -o "$out_dir/sfml_demo"

printf 'Built %s\n' "$out_dir/sfml_demo"
printf 'Run:   %s\n' "$out_dir/sfml_demo"
