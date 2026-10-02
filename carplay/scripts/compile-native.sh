#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/native
qcc -shared -fPIC -mfloat-abi=softfp -O2 -g -std=gnu99 -Wall -Wextra \
  -DMU1438_PASSIVE=0 -DMU1438_AUDI_GEOMETRY=1 -DMU1438_RUNTIME=3 \
  -Isrc/native -Isrc/native/alt111/include \
  src/native/libaltscreen111_mu1438.c src/native/alt111/src/alt111_profile.c \
  src/native/alt111/src/alt111_control.c src/native/alt111/src/alt111_video.c \
  src/native/alt111/src/alt111_resync.c -Wl,-soname,libaltscreen111.so -lsocket \
  -o out/native/libaltscreen111.so
qcc -O2 -g -std=gnu99 -Wall -Wextra -Isrc/native \
  -o out/native/cluster_runtime src/native/cluster_runtime.c -lsocket
