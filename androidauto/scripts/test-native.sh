#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/tests
libs=
if test "$(uname -s)" = Linux; then libs=-ldl; fi
for name in test_wire test_live test_start_order test_nvss_policy; do
  cc -std=gnu99 -O1 -g -fsanitize=address,undefined -pthread -Isrc/native \
    -o "out/tests/$name" "tests/$name.c" $libs
  "out/tests/$name"
done
for hd in 0 1; do
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_HD="$hd" -DAA_ZOOM=1 -o out/tests/test_hd_zoom tests/test_hd_zoom.c
 AA_TEST_HD="$hd" python3 tests/test_hd_zoom.py
done
for profile in baseline packed packed1080; do
 case "$profile" in
  baseline) geometry='-DAA_HD=0';;
  packed) geometry='-DAA_HD=0 -DAA_PACKED_720=1';;
  packed1080) geometry='-DAA_HD=1 -DAA_PACKED_1080=1';;
 esac
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_ZOOM=1 $geometry \
  -o "out/tests/descriptor_$profile" tests/test_hd_zoom.c
 cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
  -DAA_CLASSIC_LAYOUT=1 -DAA_ZOOM=1 $geometry \
  -o out/tests/test_renderer_layout tests/test_renderer_layout.c $libs
 out/tests/test_renderer_layout
done
python3 tests/test_packed_720.py
python3 tests/test_packed_1080.py
cc -std=gnu99 -O1 -g -fsanitize=address,undefined -Isrc/native \
 -DAA_CLASSIC_LAYOUT=1 -DAA_HD=1 -DAA_PACKED_1080=1 -DAA_ZOOM=1 \
 -o out/tests/test_diagnostics tests/test_diagnostics.c $libs
out/tests/test_diagnostics
python3 tests/test_main30.py
