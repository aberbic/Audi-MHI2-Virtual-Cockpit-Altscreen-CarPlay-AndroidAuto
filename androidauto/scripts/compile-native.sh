#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/native
qcc -shared -fPIC -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native \
  -DAA_LIVE=1 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 -DAA_ZOOM=1 -DAA_RELEASE=1 \
  -Wl,-soname,libaa_observe.so -o out/native/libaa_observe.so src/native/aa_endpoint.c -lsocket
qcc -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native -DAA_CLASSIC_LAYOUT=1 -DAA_HD=0 \
  -o out/native/aa_renderer src/native/aa_renderer.c -lsocket
