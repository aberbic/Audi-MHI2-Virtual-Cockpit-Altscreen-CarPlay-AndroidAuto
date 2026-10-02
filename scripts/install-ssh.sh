#!/usr/bin/env bash
# Upload flat RAM payload; invoke the same guarded installer used by M.I.B.
set -euo pipefail
cd "$(dirname "$0")/.."
case "${1:-}" in --check|--install) action=$1;; *) echo 'Usage: install-ssh.sh --check|--install [SSH-alias]';exit 2;; esac
host=${2:-mib}
test -s out/bundle/altscreen-manifest || { echo 'Prepare the private bundle first.';exit 1; }
snapshot=$(mktemp -d "${TMPDIR:-/tmp}/altscreen-upload.XXXXXX")
for f in out/bundle/altscreen-*; do
 name=${f##*/}
 ssh -o ConnectTimeout=15 "$host" "cat > /tmp/$name" < "$f"
 ssh -o ConnectTimeout=15 "$host" "cat /tmp/$name" > "$snapshot/$name"
 cmp "$f" "$snapshot/$name"
done
ssh -o ConnectTimeout=15 "$host" "/bin/sh /tmp/altscreen-install.sh $action --parked"
