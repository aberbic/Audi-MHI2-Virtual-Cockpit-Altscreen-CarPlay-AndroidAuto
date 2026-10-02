#!/usr/bin/env python3
"""Generate a PRIVATE owner-install bundle. Never commit/share out/ or inputs/."""
from pathlib import Path
import argparse, hashlib, json, shutil, subprocess, sys, zipfile
root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--touchpad', action='store_true', help='include UNVALIDATED touchpad port, not enabled by default')
args = parser.parse_args()
profile = json.loads((root/'profiles/mu1438.json').read_text())
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
subprocess.run([sys.executable, root/'scripts/verify-public.py'], check=True)
for name in ('dio_manager', 'gal', 'lsd.jxe', 'gal.json'):
    path = root/'inputs'/ (name if name == 'lsd.jxe' else name+'.stock')
    if not path.is_file() or digest(path) != profile['stock'][name]:
        sys.exit('Missing or unsupported original firmware input: '+str(path))
hmi = root/('out/touchpad/mu1438-cluster-touchpad.jar' if args.touchpad else 'out/hmi/mu1438-cluster.jar')
if not hmi.is_file(): sys.exit('Build the selected HMI add-on locally first: '+str(hmi))
with zipfile.ZipFile(hmi) as z:
    if 'local/mu1438/ClusterGate.class' not in z.namelist(): sys.exit('Wrong HMI build')
helper = root/'out/tools/altscreen-sha256'
if not helper.is_file() or helper.read_bytes()[:4] != b'\x7fELF': sys.exit('Run scripts/build-tools.sh first')
dest = root/'out/bundle'
if dest.exists(): sys.exit('out/bundle already exists; move it aside before creating another bundle')
dest.mkdir(parents=True)
items = [
 ('carplay/bin/libaltscreen111.so', '/mnt/app/root/lib-target/libaltscreen111.so', 755),
 ('carplay/bin/cluster_runtime', '/mnt/app/root/mu1438-cluster/cluster_runtime', 755),
 ('carplay/bin/cluster_supervisor.sh', '/mnt/app/root/mu1438-cluster/cluster_supervisor.sh', 755),
 ('androidauto/bin/libaa_observe.so', '/mnt/app/root/lib-target/libaa_observe.so', 755),
 ('androidauto/bin/aa_renderer', '/mnt/app/root/mu1438-aa/aa_renderer', 755),
 ('androidauto/bin/aa_supervisor.sh', '/mnt/app/root/mu1438-aa/aa_supervisor.sh', 755),
 (str(hmi.relative_to(root)), '/mnt/app/eso/hmi/lsd/jars/mu1438-cluster.jar', 644)]
rows = []
for source, target, mode in items:
    name = 'altscreen-'+Path(target).name
    shutil.copy2(root/source, dest/name)
    rows.append(f'{name}|{target}|{mode}|{digest(dest/name)}')
for component, name in [('carplay','dio_manager'),('androidauto','gal')]:
    target = dest/('altscreen-'+name)
    subprocess.run([sys.executable,root/component/'scripts/patch-loader.py',root/'inputs'/(name+'.stock'),target],check=True)
    rows.append(f'{target.name}|/mnt/app/eso/bin/apps/{name}|755|{digest(target)}')
target = dest/'altscreen-gal.json'
subprocess.run([sys.executable,root/'androidauto/scripts/prepare-main30.py',root/'inputs/gal.json.stock',target],check=True)
if digest(target) != profile['android_auto']['gal_json_sha256']:
    sys.exit('Unexpected main-screen configuration output hash')
rows.append(f'{target.name}|/mnt/system/etc/eso/production/gal.json|644|{digest(target)}')
(dest/'altscreen-manifest').write_text('\n'.join(rows)+'\n')
shutil.copy2(helper,dest/'altscreen-sha256')
shutil.copy2(root/'scripts/unit-install.sh',dest/'altscreen-install.sh')
mod = root/'out/mib-sd/mod'
(mod/'altscreen').mkdir(parents=True)
for f in dest.iterdir(): shutil.copy2(f,mod/'altscreen'/f.name)
for name in ('custom.sh','command.sh'): shutil.copy2(root/'toolbox'/name,mod/name)
print('Private SSH payload:',dest)
print('Private M.I.B. SD layout:',mod.parent)
print('Contains reconstructed proprietary firmware files: DO NOT publish these bundles.')
