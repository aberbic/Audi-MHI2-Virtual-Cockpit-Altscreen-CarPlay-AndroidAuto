#!/usr/bin/env bash
# Owner-authorized parked experiment. Changes AA pair + main AA fps list only.
# --restore returns to the slightly improved packed-720 profile and old config.
set -euo pipefail
cd "$(dirname "$0")/.."
case "${1:-}" in --install|--restore) action=$1;; *) echo 'Usage: deploy-packed-1080.sh --install|--restore [SSH-alias]';exit 2;; esac
host=${2:-mib}
sshcar(){ ssh -o ConnectTimeout=15 "$host" "$@"; }
hash(){ shasum -a 256 "$1" | awk '{print $1}'; }
mkdir -p out/vehicle-tests
backup=$(mktemp -d "$PWD/out/vehicle-tests/packed-1080.XXXXXX")
sshcar 'pidin ar' > "$backup/processes.before"
if rg -q '(^|[ /])gal([[:space:]]|$)|/mnt/app/root/mu1438-aa/aa_renderer' "$backup/processes.before"; then
 echo 'AA is running; disconnect Pixel before the paired update.' >&2;exit 1
fi
names=(libaa_observe.so aa_renderer gal.json)
targets=(/mnt/app/root/lib-target/libaa_observe.so /mnt/app/root/mu1438-aa/aa_renderer /mnt/system/etc/eso/production/gal.json)
oldhash=(09a3f93b286e7ddb90cabd0b05f5bbea9a6d922235469959bc7ede949c9c5564 a532f62970192efffa2c8dc95455bea12cb807013b2039bde81c0f0ef4eb7c11 bd1babf5395e59f1ff2dbde7b68e822f59b381ee665dc1fc53d7caa6e2a98f13)
newhash=(d4cbdc201940fda58e17d7e51c59036dba1440a97625b224ffccff1dec54e961 f1a9c72e6f7451c3c07ff876da5187f06579a72559e0be369d9918f879b3452c 1bddd7e893850c1087eb36bd19ba9a5b270e8fb879720513cf5ad8b3e860ab9a)
for i in 0 1 2; do
 sshcar "cat ${targets[$i]}" > "$backup/${names[$i]}.before"
 actual=$(hash "$backup/${names[$i]}.before")
 if test "$action" = --install; then test "$actual" = "${oldhash[$i]}";
 else test "$actual" = "${newhash[$i]}" || test "$actual" = "${oldhash[$i]}";fi
 shasum -a 256 "$backup/${names[$i]}.before" >> "$backup/SHA256SUMS"
done
sshcar 'cat /mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar' > "$backup/hmi.before"
sshcar 'test ! -e /mnt/app/root/mu1438-rgd/metadata.enabled'
if test "$action" = --install; then
 for i in 0 1 2; do test "$(hash "out/packed-1080/${names[$i]}")" = "${newhash[$i]}";done
 sshcar 'set -e
 mount -uw /mnt/app
 trap "sync; mount -ur /mnt/app" 0
 if test ! -e /mnt/app/root/mu1438-aa/libaa.before-packed1080.so; then cp /mnt/app/root/lib-target/libaa_observe.so /mnt/app/root/mu1438-aa/libaa.before-packed1080.so;fi
 if test ! -e /mnt/app/root/mu1438-aa/aa_renderer.before-packed1080; then cp /mnt/app/root/mu1438-aa/aa_renderer /mnt/app/root/mu1438-aa/aa_renderer.before-packed1080;fi
 if test ! -e /mnt/app/root/mu1438-aa/gal.before-packed1080.json; then cp /mnt/system/etc/eso/production/gal.json /mnt/app/root/mu1438-aa/gal.before-packed1080.json;fi'
fi
oldfiles=(/mnt/app/root/mu1438-aa/libaa.before-packed1080.so /mnt/app/root/mu1438-aa/aa_renderer.before-packed1080 /mnt/app/root/mu1438-aa/gal.before-packed1080.json)
for i in 0 1 2; do
 sshcar "cat ${oldfiles[$i]}" > "$backup/${names[$i]}.baseline"
 test "$(hash "$backup/${names[$i]}.baseline")" = "${oldhash[$i]}"
 source="out/packed-1080/${names[$i]}"
 test "$action" != --restore || source="$backup/${names[$i]}.baseline"
 sshcar "cat > /tmp/aa-hdtest-${names[$i]}" < "$source"
 sshcar "cat /tmp/aa-hdtest-${names[$i]}" > "$backup/${names[$i]}.staged"
 cmp "$source" "$backup/${names[$i]}.staged"
done
sshcar 'set -e
mount -uw /mnt/app
trap "sync; mount -ur /mnt/app" 0
mount -uw /mnt/system
trap "sync; mount -ur /mnt/system; mount -ur /mnt/app" 0
cp /tmp/aa-hdtest-libaa_observe.so /mnt/app/root/lib-target/libaa_observe.so.hd-new
cp /tmp/aa-hdtest-aa_renderer /mnt/app/root/mu1438-aa/aa_renderer.hd-new
cp /tmp/aa-hdtest-gal.json /mnt/system/etc/eso/production/gal.json.hd-new
chmod 755 /mnt/app/root/lib-target/libaa_observe.so.hd-new /mnt/app/root/mu1438-aa/aa_renderer.hd-new
chmod 644 /mnt/system/etc/eso/production/gal.json.hd-new
mv /mnt/app/root/mu1438-aa/aa_renderer.hd-new /mnt/app/root/mu1438-aa/aa_renderer
mv /mnt/app/root/lib-target/libaa_observe.so.hd-new /mnt/app/root/lib-target/libaa_observe.so
mv /mnt/system/etc/eso/production/gal.json.hd-new /mnt/system/etc/eso/production/gal.json'
for i in 0 1 2; do
 sshcar "cat ${targets[$i]}" > "$backup/${names[$i]}.published"
 cmp "$backup/${names[$i]}.staged" "$backup/${names[$i]}.published"
done
sshcar 'cat /mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar' > "$backup/hmi.after"
cmp "$backup/hmi.before" "$backup/hmi.after"
if test "$action" = --install; then
 for item in aa_endpoint.h264 aa_renderer.log aa_endpoint.log mu1438-touchpad-input.log; do
  if sshcar "if test -f /tmp/$item; then cat /tmp/$item; else exit 44;fi" > "$backup/previous-$item"; then :; else rc=$?;test "$rc" = 44 || exit "$rc";fi
 done
 sshcar ': > /tmp/aa_capture.enabled; : > /tmp/mu1438-touchpad-trace.enabled'
 echo 'Fuller 1080p test installed. Main AA advertises 30fps only. Capture/input diagnostics armed.'
else
 sshcar 'rm -f /tmp/aa_capture.enabled /tmp/mu1438-touchpad-trace.enabled'
 echo 'Packed-720 pair and original frame-rate configuration restored.'
fi
printf 'All three files verified, HMI unchanged. Private backup: %s\n' "$backup"
