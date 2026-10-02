"""Resolution/layout-only comparison: no change to input services or codec policy."""
from pathlib import Path
import subprocess
from compare_discovery import fields, services
root=Path(__file__).resolve().parents[1]
def get(data,field):
    result=[v for n,w,v in fields(data) if n==field]
    assert len(result)==1,(field,result)
    return result[0]
def messages(name):
    return [bytes.fromhex(x) for x in subprocess.check_output([root/'out/tests'/name],text=True).splitlines()]
a,b=messages('descriptor_baseline'),messages('descriptor_packed')
sa,sb=services(a[0]),services(b[0])
assert set(sa)==set(sb)=={19,20}
assert sa[20]==sb[20] and a[1:]==b[1:]
ma,mb=get(sa[19],3),get(sb[19],3)
assert [(n,w,v) for n,w,v in fields(ma) if n!=4]==[(n,w,v) for n,w,v in fields(mb) if n!=4]
va,vb=get(ma,4),get(mb,4)
for field in (1,2,6,7,8,10):assert get(va,field)==get(vb,field)
assert [get(vb,i) for i in (1,2,3,4,5,9)]==[2,2,0,240,128,113]
ui=get(vb,11)
assert [get(get(ui,1),i) for i in (1,2,3,4)]==[120,120,0,0]
assert [get(get(ui,2),i) for i in (1,2,3,4)]==[68,129,453,453]
assert get(ui,2)==get(ui,3)
assert get(ui,4)==get(get(va,11),4)
for old,new in zip((51,97,340,340),(68,129,453,453)):
    assert abs(old*1.5-new*1.125)<0.5
assert 960/96==1280/128 and 360/96==480/128
print('PASS: packed 720p geometry; same logical UI size, input bytes, services, codec, cadence, decoder depth and theme')
