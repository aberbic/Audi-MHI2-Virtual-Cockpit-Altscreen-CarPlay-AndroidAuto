#!/usr/bin/env bash
# Rebuild the accepted geometry profile; checked-in binaries remain untouched.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/tests out/packed-1080
libs=
if test "$(uname -s)" = Linux; then libs=-ldl;fi
cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_PACKED_720=1 -DAA_ZOOM=1 \
 -o out/tests/descriptor_packed tests/test_hd_zoom.c
cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=1 -DAA_PACKED_1080=1 -DAA_ZOOM=1 \
 -o out/tests/descriptor_packed1080 tests/test_hd_zoom.c
python3 tests/test_packed_1080.py
for name in test_renderer_layout test_diagnostics; do
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_HD=1 -DAA_PACKED_1080=1 -DAA_ZOOM=1 \
  -o "out/tests/$name" "tests/$name.c" $libs
 "out/tests/$name"
done
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -shared -fPIC -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native \
 -DAA_LIVE=1 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=1 -DAA_PACKED_1080=1 -DAA_DIAGNOSTICS=1 -DAA_ZOOM=1 -DAA_RELEASE=1 \
 -Wl,-soname,libaa_observe.so -o out/packed-1080/libaa_observe.so src/native/aa_endpoint.c -lsocket
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native -DAA_CLASSIC_LAYOUT=1 -DAA_HD=1 -DAA_PACKED_1080=1 \
 -o out/packed-1080/aa_renderer src/native/aa_renderer.c -lsocket
