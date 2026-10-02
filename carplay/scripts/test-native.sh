#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/tests
libs=()
if test "$(uname -s)" = Linux; then libs=(-ldl); fi
cc -std=gnu99 -O1 -g -fsanitize=address,undefined -pthread -Isrc/native \
  -o out/tests/test_au_wire tests/test_au_wire.c "${libs[@]}"
out/tests/test_au_wire
