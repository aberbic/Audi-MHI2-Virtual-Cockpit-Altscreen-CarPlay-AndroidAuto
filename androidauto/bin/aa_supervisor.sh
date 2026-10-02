#!/bin/sh
# One renderer per gal lifetime. No boot-menu edits or phone/network actions.
export LD_LIBRARY_PATH=/eso/lib:/mnt/app/root/lib-target:/mnt/app/usr/lib:/mnt/app/armle/lib:/mnt/app/armle/lib/dll:/mnt/app/armle/usr/lib
export IPL_CONFIG_DIR=/etc/eso/production
parent_pid=$1
case "$parent_pid" in ''|*[!0-9]*) exit 2;; esac
test "$parent_pid" -gt 1 || exit 2
child_pid=
trap 'if test -n "$child_pid"; then kill -TERM "$child_pid" 2>/dev/null; wait "$child_pid"; fi' 0
trap '' 1
trap 'exit 0' 2 15
attempt=0
while kill -0 "$parent_pid" 2>/dev/null && test ! -e /tmp/mu1438-cluster-disable; do
    /mnt/app/root/mu1438-aa/aa_renderer "$parent_pid" &
    child_pid=$!
    wait "$child_pid"
    result=$?
    child_pid=
    test "$result" -ne 0 || exit 0
    attempt=$((attempt+1))
    test "$attempt" -lt 3 || exit 1
    sleep 2
done
