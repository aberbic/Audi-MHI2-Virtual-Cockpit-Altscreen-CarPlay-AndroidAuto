#!/usr/bin/env python3
"""Change only the stock main-video frame-rate list; preserve commented JSON."""
from pathlib import Path
import re,sys
if len(sys.argv)!=3:sys.exit('Usage: prepare-main30.py original-gal.json output-gal.json')
p=Path(sys.argv[1]).read_bytes()
pattern=rb'("supportedFrameRates"\s*:\s*)\[\s*30\s*,\s*60\s*\]'
if len(re.findall(pattern,p))!=1:sys.exit('Expected exactly one stock [30,60] frame-rate declaration')
out,count=re.subn(pattern,lambda m:m[1]+b'[30]',p)
assert count==1 and out!=p
with Path(sys.argv[2]).open('xb') as f:f.write(out)
print('Prepared main-screen 30fps-only advertisement; all other configuration bytes preserved')
