#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p out
cc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined rgd_wire.c test_wire.c -o out/test_wire
out/test_wire
cc -std=c99 -Wall -Wextra -Werror -g -fsanitize=address,undefined rgd_wire.c rgd_monitor.c rgd_packet.c test_monitor.c -o out/test_monitor
out/test_monitor
