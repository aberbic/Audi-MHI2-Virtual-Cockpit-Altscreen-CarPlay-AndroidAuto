#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p out/tools
cc -O2 -std=c99 -Wall -Wextra -Werror -o out/tools/sha256-host tools/sha256.c
python3 tools/test_sha256.py
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -O2 -std=gnu99 -Wall -Wextra -Werror -o out/tools/altscreen-sha256 tools/sha256.c
