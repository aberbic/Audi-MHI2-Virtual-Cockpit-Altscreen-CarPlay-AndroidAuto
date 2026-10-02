#!/usr/bin/env bash
# Parked paired-profile experiment. No HMI, CarPlay or input-policy changes.
set -euo pipefail
cd "$(dirname "$0")/.."
case "${1:-}" in --install|--restore) action=$1;; *) echo 'Usage: deploy-packed-720.sh --install|--restore [SSH-alias]';exit 2;; esac
host=${2:-mib}
sshcar(){ ssh -o ConnectTimeout=15 "$host" "$@"; }
hash(){ shasum -a 256 "$1" | awk '{print $1}'; }
mkdir -p out/vehicle-tests
backup=$(mktemp -d "$PWD/out/vehicle-tests/packed-720.XXXXXX")
sshcar 'pidin ar' > "$backup/processes.before"
if rg -q '(^|[ /])gal([[:space:]]|$)|/mnt/app/root/mu1438-aa/aa_renderer' "$backup/processes.before"; then
 echo 'AA is running; paired update requires the Pixel disconnected.' >&2;exit 1
fi
sshcar 'cat /mnt/app/root/lib-target/libaa_observe.so' > "$backup/hook.before"
sshcar 'cat /mnt/app/root/mu1438-aa/aa_renderer' > "$backup/renderer.before"
sshcar 'cat /mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar' > "$backup/hmi.before"
shasum -a 256 "$backup/hook.before" "$backup/renderer.before" "$backup/hmi.before" > "$backup/SHA256SUMS"
if test "$action" = --install; then
 test "$(hash "$backup/hook.before")" = d5561cf8a34a8435e6789ec316d197ec6a385e6a4acc54150a49f50e2eace84f
 test "$(hash "$backup/renderer.before")" = f38c54ef96ba33a47e6d6b9f9dd698a6d97fb92bdf1c3879045b4da8e0457781
 test "$(hash out/packed-720/libaa_observe.so)" = 09a3f93b286e7ddb90cabd0b05f5bbea9a6d922235469959bc7ede949c9c5564
 test "$(hash out/packed-720/aa_renderer)" = a532f62970192efffa2c8dc95455bea12cb807013b2039bde81c0f0ef4eb7c11
 sshcar 'set -e
 mount -uw /mnt/app
 trap "sync; mount -ur /mnt/app" 0
 if test ! -e /mnt/app/root/mu1438-aa/libaa.before-packed720.so; then cp /mnt/app/root/lib-target/libaa_observe.so /mnt/app/root/mu1438-aa/libaa.before-packed720.so; fi
 if test ! -e /mnt/app/root/mu1438-aa/aa_renderer.before-packed720; then cp /mnt/app/root/mu1438-aa/aa_renderer /mnt/app/root/mu1438-aa/aa_renderer.before-packed720; fi'
 for name in libaa_observe.so aa_renderer; do
  sshcar "cat > /tmp/aa-packed720-$name" < "out/packed-720/$name"
  sshcar "cat /tmp/aa-packed720-$name" > "$backup/$name.staged"
  cmp "out/packed-720/$name" "$backup/$name.staged"
 done
else
 test "$(hash "$backup/hook.before")" = 09a3f93b286e7ddb90cabd0b05f5bbea9a6d922235469959bc7ede949c9c5564
 test "$(hash "$backup/renderer.before")" = a532f62970192efffa2c8dc95455bea12cb807013b2039bde81c0f0ef4eb7c11
fi
sshcar 'cat /mnt/app/root/mu1438-aa/libaa.before-packed720.so' > "$backup/hook.baseline"
sshcar 'cat /mnt/app/root/mu1438-aa/aa_renderer.before-packed720' > "$backup/renderer.baseline"
test "$(hash "$backup/hook.baseline")" = d5561cf8a34a8435e6789ec316d197ec6a385e6a4acc54150a49f50e2eace84f
test "$(hash "$backup/renderer.baseline")" = f38c54ef96ba33a47e6d6b9f9dd698a6d97fb92bdf1c3879045b4da8e0457781
if test "$action" = --restore; then
 sshcar 'cat > /tmp/aa-packed720-libaa_observe.so' < "$backup/hook.baseline"
 sshcar 'cat > /tmp/aa-packed720-aa_renderer' < "$backup/renderer.baseline"
fi
sshcar 'set -e
mount -uw /mnt/app
trap "sync; mount -ur /mnt/app" 0
cp /tmp/aa-packed720-aa_renderer /mnt/app/root/mu1438-aa/aa_renderer.packed-new
cp /tmp/aa-packed720-libaa_observe.so /mnt/app/root/lib-target/libaa_observe.so.packed-new
chmod 755 /mnt/app/root/mu1438-aa/aa_renderer.packed-new /mnt/app/root/lib-target/libaa_observe.so.packed-new
mv /mnt/app/root/mu1438-aa/aa_renderer.packed-new /mnt/app/root/mu1438-aa/aa_renderer
mv /mnt/app/root/lib-target/libaa_observe.so.packed-new /mnt/app/root/lib-target/libaa_observe.so'
sshcar 'cat /mnt/app/root/lib-target/libaa_observe.so' > "$backup/hook.published"
sshcar 'cat /mnt/app/root/mu1438-aa/aa_renderer' > "$backup/renderer.published"
sshcar 'cat /mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar' > "$backup/hmi.after"
cmp "$backup/hmi.before" "$backup/hmi.after"
if test "$action" = --install; then
 cmp out/packed-720/libaa_observe.so "$backup/hook.published"
 cmp out/packed-720/aa_renderer "$backup/renderer.published"
 if sshcar 'if test -f /tmp/aa_endpoint.h264; then cat /tmp/aa_endpoint.h264; else exit 44; fi' > "$backup/previous-capture.h264"; then :; else rc=$?;test "$rc" = 44 || exit "$rc";fi
 sshcar ': > /tmp/aa_capture.enabled'
 echo 'Packed 720p installed; next AA session captures at most 8 MiB for analysis.'
else
 cmp "$backup/hook.baseline" "$backup/hook.published"
 cmp "$backup/renderer.baseline" "$backup/renderer.published"
 sshcar 'rm -f /tmp/aa_capture.enabled'
 echo 'Original 720p baseline pair restored and verified.'
fi
printf 'HMI unchanged. Private backup: %s\n' "$backup"
