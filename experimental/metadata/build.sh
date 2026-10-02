#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p out
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -shared -fPIC -O2 -g -std=gnu99 -Wall -Wextra -Werror -DRGD_ADAPTER \
 -Wl,-Bsymbolic -Wl,-soname,libmu1438_rgd_capture.so -o out/ipod-drvr-iap2.capture.so \
 driver_probe.c rgd_adapter.c rgd_wire.c rgd_monitor.c rgd_packet.c
docker run --rm --platform=linux/amd64 -v "$PWD:/w" -w /w mhi2-qnx65-armv7-public:4.9 \
 qcc -O2 -std=gnu99 -Wall -Wextra -Werror -o out/test_adapter.qnx test_adapter.c rgd_wire.c rgd_monitor.c rgd_packet.c
echo 'Built UNVALIDATED metadata research artifacts only. No vehicle installation performed.'
