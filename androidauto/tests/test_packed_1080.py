from pathlib import Path
import subprocess
from compare_discovery import fields,services
root=Path(__file__).resolve().parents[1]
def get(p,n):
    v=[v for k,w,v in fields(p) if k==n];assert len(v)==1;return v[0]
def read(name):return [bytes.fromhex(s) for s in subprocess.check_output([root/'out/tests'/name],text=True).splitlines()]
a,b=read('descriptor_packed'),read('descriptor_packed1080')
sa,sb=services(a[0]),services(b[0]);assert sa[20]==sb[20] and a[1:]==b[1:]
ma,mb=get(sa[19],3),get(sb[19],3)
assert [(n,w,v) for n,w,v in fields(ma) if n!=4]==[(n,w,v) for n,w,v in fields(mb) if n!=4]
va,vb=get(ma,4),get(mb,4)
assert [get(vb,i) for i in (1,2,3,4,5,9)]==[3,2,0,360,192,170]
for field in (2,6,7,8,10):assert get(va,field)==get(vb,field)
ui=get(vb,11)
assert [get(get(ui,1),i) for i in (1,2,3,4)]==[180,180,0,0]
assert [get(get(ui,2),i) for i in (1,2,3,4)]==[102,194,680,680]
assert get(ui,2)==get(ui,3) and get(ui,4)==get(get(va,11),4)
assert 1280/128==1920/192 and 480/128==720/192
print('PASS: full-width 1080p crop, same logical UI size and unchanged input/service/codec/depth/theme fields')
