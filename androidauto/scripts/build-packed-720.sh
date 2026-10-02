#!/usr/bin/env bash
# Explicit experiment; does not replace checked-in baseline binaries or deploy.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/tests out/packed-720
libs=
if test "$(uname -s)" = Linux; then libs=-ldl; fi
for variant in baseline packed; do
 packed=0;test "$variant" != packed || packed=1
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_ZOOM=1 -DAA_PACKED_720="$packed" \
  -o "out/tests/descriptor_$variant" tests/test_hd_zoom.c
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_PACKED_720="$packed" \
  -o "out/tests/renderer_layout_$variant" tests/test_renderer_layout.c $libs
 "out/tests/renderer_layout_$variant"
done
python3 tests/test_packed_720.py
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -shared -fPIC -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native \
 -DAA_LIVE=1 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_PACKED_720=1 -DAA_ZOOM=1 -DAA_RELEASE=1 \
 -Wl,-soname,libaa_observe.so -o out/packed-720/libaa_observe.so src/native/aa_endpoint.c -lsocket
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_PACKED_720=1 \
 -o out/packed-720/aa_renderer src/native/aa_renderer.c -lsocket
