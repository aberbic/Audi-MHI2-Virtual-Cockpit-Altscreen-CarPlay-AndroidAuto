#!/usr/bin/env python3
"""Apply the hook-first loader delta only to the exact owned MU1438 stock gal."""
from pathlib import Path
import hashlib
import json
import sys

root = Path(__file__).resolve().parents[1]
patch = json.loads((root / 'patches/gal.json').read_text())
if len(sys.argv) != 3:
    sys.exit('Usage: patch-loader.py stock-gal output-file')
data = bytearray(Path(sys.argv[1]).read_bytes())
if hashlib.sha256(data).hexdigest() != patch['stock_sha256']:
    sys.exit('Refusing a non-matching firmware executable.')
for change in patch['changes']:
    start = change['offset']
    old, new = bytes.fromhex(change['before']), bytes.fromhex(change['after'])
    if len(old) != len(new) or data[start:start+len(old)] != old:
        sys.exit('Delta precondition failed.')
    data[start:start+len(old)] = new
if hashlib.sha256(data).hexdigest() != patch['patched_sha256']:
    sys.exit('Patched hash mismatch.')
output = Path(sys.argv[2])
output.parent.mkdir(parents=True, exist_ok=True)
with output.open('xb') as stream:
    stream.write(data)
output.chmod(0o755)
print('Verified hook-first loader:', output)
