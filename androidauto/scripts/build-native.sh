#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w \
  mhi2-qnx65-armv7-public:4.9 bash scripts/compile-native.sh
