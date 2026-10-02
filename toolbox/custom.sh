#!/bin/sh
# M.I.B. Advanced Settings -> Run Custom Script / Run individual script.
# This module never restarts HMI. Preparing the SD payload is a separate local step.
set -eu
case $0 in */*) D=${0%/*};; *) D=.;; esac
D=$(cd "$D" && pwd)
if test ! -d /mnt/app/eso/hmi/lsd && test -d /net/mmx/mnt/app/eso/hmi/lsd; then
 exec on -f mmx /bin/sh "$D/custom.sh" "$@"
fi
echo 'AltScreen MU1438/Tegra: park safely, disconnect all projection phones, retain recovery SD.'
exec /bin/sh "$D/altscreen/altscreen-install.sh" "${1:---install}" --parked
