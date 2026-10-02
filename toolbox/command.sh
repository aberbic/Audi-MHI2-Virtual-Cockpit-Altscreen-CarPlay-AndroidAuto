#!/bin/sh
# Older M.I.B. menus invoke command.sh; sourcing it during M.I.B. setup is a no-op.
case ${0##*/} in command.sh) exec /bin/sh "${0%/*}/custom.sh" "$@";; esac
