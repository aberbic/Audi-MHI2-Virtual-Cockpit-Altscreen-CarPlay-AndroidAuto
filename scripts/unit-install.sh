#!/bin/sh
# Shared M.I.B./SSH installer. QNX /tmp is flat shared memory, not a directory tree.
# No reboot, process termination, network changes, or metadata/touchpad opt-in.
set -eu
PATH=/proc/boot:/bin:/usr/bin:/sbin:/usr/sbin:/mnt/app/armle/bin:/mnt/app/armle/usr/bin
export PATH
unset LD_PRELOAD
case $0 in */*) D=${0%/*};; *) D=.;; esac
D=$(cd "$D" && pwd)
case "${1:-}" in --install|--restore|--check) action=$1 ;; *) echo 'Usage: altscreen-install.sh --check|--install|--restore --parked';exit 2;; esac
test "${2:-}" = --parked || { echo 'Park safely and explicitly supply --parked.';exit 2; }
test -d /mnt/app/eso/hmi/lsd && test -f /ifs/lsd.jxe || { echo 'Run on the MMX/Tegra head unit, not RCC.';exit 1; }
tool=/tmp/altscreen-sha256.$$
cp "$D/altscreen-sha256" "$tool"
chmod 755 "$tool"
pidin ar > /tmp/altscreen-processes.$$
while IFS= read -r line; do
 case "$line" in *'/dio_manager'*|*' mm-ipod '*|*'/mm-ipod '*|*'io-usb-dcd'*|*'/gal'|*'/gal '*|*'/aa_renderer '*|*'/cluster_runtime '*)
  echo 'Disconnect projection and wait for its processes to exit. Restart MMI manually if stuck.';exit 1;; esac
done < /tmp/altscreen-processes.$$
dest_for() {
 case $1 in
  altscreen-dio_manager) echo /mnt/app/eso/bin/apps/dio_manager;;
  altscreen-gal) echo /mnt/app/eso/bin/apps/gal;;
  altscreen-gal.json) echo /mnt/system/etc/eso/production/gal.json;;
  altscreen-libaltscreen111.so) echo /mnt/app/root/lib-target/libaltscreen111.so;;
  altscreen-cluster_runtime) echo /mnt/app/root/mu1438-cluster/cluster_runtime;;
  altscreen-cluster_supervisor.sh) echo /mnt/app/root/mu1438-cluster/cluster_supervisor.sh;;
  altscreen-libaa_observe.so) echo /mnt/app/root/lib-target/libaa_observe.so;;
  altscreen-aa_renderer) echo /mnt/app/root/mu1438-aa/aa_renderer;;
  altscreen-aa_supervisor.sh) echo /mnt/app/root/mu1438-aa/aa_supervisor.sh;;
  altscreen-mu1438-cluster.jar) echo /mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar;;
  *) return 1;;
 esac
}
count=0
seen='|'
while IFS='|' read -r name dest mode digest; do
 test "$(dest_for "$name")" = "$dest" || exit 1
 case "$seen" in *"|$name|"*) echo 'Duplicate payload record';exit 1;; esac
 seen="$seen$name|"
 case "$name:$mode" in
  altscreen-mu1438-cluster.jar:644|altscreen-gal.json:644) ;;
  altscreen-mu1438-cluster.jar:*|altscreen-gal.json:*) exit 1;;
  *:755) ;; *) exit 1;;
 esac
 count=$((count+1))
done < "$D/altscreen-manifest"
test "$count" = 10 || { echo 'Incomplete payload manifest';exit 1; }
rw=0
system_rw=0
backup=
cleanup(){
 rc=$?
 if test "$system_rw" = 1; then sync || :;mount -ur /mnt/system || echo 'WARNING: failed to restore /mnt/system read-only';fi
 if test "$rw" = 1; then sync || :;mount -ur /mnt/app || echo 'WARNING: failed to restore /mnt/app read-only';fi
 if test "$rc" != 0 && test -n "$backup"; then echo "Interrupted. Retain recovery snapshot: $backup";fi
}
trap cleanup 0
trap 'exit 1' 1 2 15
if test "$action" = --restore; then
 case "$D" in /mnt/app/root/altscreen-backups/*) ;; *) echo 'Run the saved installer from its backup directory.';exit 1;; esac
 backup=$D
 while IFS='|' read -r name dest mode digest; do
  old=$(cat "$D/$name.old-sha256")
  if test "$old" != absent; then "$tool" "$old" "$D/$name.before";fi
  if test -e "$dest"; then
   current=$("$tool" "$dest")
   test "$current" = "$digest" || test "$current" = "$old" || { echo "Unrecognized changed target: $dest";exit 1; }
  fi
 done < "$D/altscreen-manifest"
 mount -uw /mnt/app;rw=1
 mount -uw /mnt/system;system_rw=1
 while IFS='|' read -r name dest mode digest; do
  old=$(cat "$D/$name.old-sha256")
  if test "$old" = absent; then
   if test -e "$dest"; then test ! -e "$D/$name.disabled";mv "$dest" "$D/$name.disabled";fi
  else
   cp "$D/$name.before" "$dest.altscreen-restore";chmod "$mode" "$dest.altscreen-restore"
   mv "$dest.altscreen-restore" "$dest";"$tool" "$old" "$dest"
  fi
 done < "$D/altscreen-manifest"
 echo 'Prior files restored; newly added files retained in backup. Restart MMI manually.'
 exit 0
fi
# Original firmware fingerprints: no train-string override can bypass these checks.
"$tool" e43d80e7eeb803d6a7db29908562b9545e7b17138d4660ce3a6efafa974b99ce /ifs/lsd.jxe
"$tool" 34503a9f18799420005d7bd40c3cbf84634c8c9030bb0dcba79d519f0227e949 /mnt/app/eso/lib/libairplay.so
"$tool" c4b06d695649dd80dea5033e0ad02c39dbec512826a9d725e606b84219d14c30 /mnt/app/eso/lib/libautoreceiver_qnx.so.1
"$tool" 0ac13f43d3fb259eee328c272fa1211bb558b6e2cd97c6e38ef38e4e71469d51 /mnt/app/armle/lib/dll/ipod-drvr-iap2.so
case $("$tool" /mnt/app/eso/bin/apps/dio_manager) in bd42c9ee43021eca4db1dbd42550d86f7c7df60d350063564e985667a63bfe70|d6edbae1f239733adeab057e9d5ef3345bc1100af1a56b6559783a47eb13a493) ;; *) echo 'Unsupported CarPlay loader';exit 1;; esac
case $("$tool" /mnt/app/eso/bin/apps/gal) in 5b56421a3a08976ae5a35344ae71f8baa5a33adeb3d171de75692b24137b590f|a90dedaa0c3fe06b1e0f4322cc80c91f99932ab630d13454df097e70a5468aeb) ;; *) echo 'Unsupported AA loader';exit 1;; esac
case $("$tool" /mnt/system/etc/eso/production/gal.json) in bd1babf5395e59f1ff2dbde7b68e822f59b381ee665dc1fc53d7caa6e2a98f13|1bddd7e893850c1087eb36bd19ba9a5b270e8fb879720513cf5ad8b3e860ab9a) ;; *) echo 'Unsupported AA configuration';exit 1;; esac
test ! -e /mnt/app/root/mu1438-rgd/metadata.enabled || { echo 'Disable/restore experimental metadata before baseline installation.';exit 1; }
while IFS='|' read -r name dest mode digest; do "$tool" "$digest" "$D/$name";done < "$D/altscreen-manifest"
test "$action" != --check || { echo 'Preflight passed; no flash files changed.';exit 0; }
backup=/mnt/app/root/altscreen-backups/$(date +%Y%m%d-%H%M%S)-$$
test ! -e "$backup"
mount -uw /mnt/app;rw=1
mount -uw /mnt/system;system_rw=1
mkdir -p "$backup"
cp "$0" "$backup/altscreen-install.sh";cp "$D/altscreen-manifest" "$backup/altscreen-manifest";cp "$tool" "$backup/altscreen-sha256"
while IFS='|' read -r name dest mode digest; do
 if test -e "$dest"; then
  old=$("$tool" "$dest");cp "$dest" "$backup/$name.before";"$tool" "$old" "$backup/$name.before"
  printf '%s\n' "$old" > "$backup/$name.old-sha256"
 else printf 'absent\n' > "$backup/$name.old-sha256";fi
 mkdir -p "${dest%/*}"
 cp "$D/$name" "$dest.altscreen-new";chmod "$mode" "$dest.altscreen-new";"$tool" "$digest" "$dest.altscreen-new"
done < "$D/altscreen-manifest"
while IFS='|' read -r name dest mode digest; do mv "$dest.altscreen-new" "$dest";"$tool" "$digest" "$dest";done < "$D/altscreen-manifest"
echo "Installed and verified. SAVE THIS BACKUP: $backup"
echo 'Restart MMI manually while parked. Test both main-screen controls and cluster projection.'
