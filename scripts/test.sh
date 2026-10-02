#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 scripts/verify-public.py
python3 scripts/test-firmware.py
bash carplay/scripts/test-native.sh
bash androidauto/scripts/test-native.sh
bash experimental/metadata/test.sh
mkdir -p out/tools
cc -O2 -std=c99 -Wall -Wextra -Werror -o out/tools/sha256-host tools/sha256.c
python3 tools/test_sha256.py
for script in scripts/*.sh toolbox/*.sh touchpad/*.sh experimental/*/*.sh; do bash -n "$script";done
echo 'PASS: host tests and installer shell syntax; no vehicle contacted'
