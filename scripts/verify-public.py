#!/usr/bin/env python3
"""Offline public-tree and baseline artifact checks. Never contacts a vehicle."""
from pathlib import Path
import hashlib, json, re, subprocess
root=Path(__file__).resolve().parents[1]
manifest=json.loads((root/'artifacts.json').read_text())
for name,expected in manifest['sha256'].items():
    assert hashlib.sha256((root/name).read_bytes()).hexdigest()==expected, name
profile=json.loads((root/'profiles/mu1438.json').read_text())
assert all(re.fullmatch('[0-9a-f]{64}',v) for v in profile['stock'].values())
assert profile['android_auto']['AA_HD']==1
assert manifest['android_auto_build_defines']['AA_HD']==1
assert profile['android_auto']['AA_PACKED_1080']==1
assert manifest['android_auto_build_defines']['AA_PACKED_1080']==1
assert profile['android_auto']['coded']==[1920,1080]
assert profile['android_auto']['crop_xywh']==[0,180,1920,720]
assert profile['android_auto']['main_supported_fps']==[30]
firmware=json.loads((root/'firmware/mu1438-inputs.json').read_text())
assert firmware['train']==profile['train'] and firmware['main_unit']==profile['main_unit']
assert set(firmware['files'])=={'dio_manager.stock','gal.stock','gal.json.stock','lsd.jxe','mu1438-stock.jar'}
for original in ('dio_manager','gal','gal.json','lsd.jxe'):
    name=original if original=='lsd.jxe' else original+'.stock'
    assert firmware['files'][name]['sha256']==profile['stock'][original],name
assert re.fullmatch('[0-9a-f]{64}',firmware['archive_sha256'])
assert all(re.fullmatch('[0-9a-f]{64}',entry['sha256']) and entry['bytes']>0 for entry in firmware['files'].values())
skip={'.git','out','inputs','backups','logs','__pycache__'}
files=[]
for path in root.rglob('*'):
    rel=path.relative_to(root)
    if not path.is_file() or any(p in skip for p in rel.parts): continue
    assert path.suffix not in {'.jar','.jxe','.class','.pem','.key','.fec','.pcap','.log'}, str(rel)
    assert not any(p in {'stock','refs'} for p in rel.parts),str(rel)
    data=path.read_bytes()
    assert not re.search(rb'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----',data),str(rel)
    assert not re.search(rb'gh[pousr]_[A-Za-z0-9]{30,}',data),str(rel)
    assert (bytes([47])+b'Users/') not in data,str(rel)
    if data.startswith(b'\x7fELF'): assert str(rel) in manifest['sha256'],str(rel)
    files.append(str(rel))
print(f'PASS: {len(files)} public files checked; baseline hashes verified; private/generated inputs excluded')
