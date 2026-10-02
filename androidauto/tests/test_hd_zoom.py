from pathlib import Path
import subprocess
import os
from compare_discovery import fields,services,varint
root=Path(__file__).resolve().parents[1]
lines=subprocess.check_output([str(root/'out/tests/test_hd_zoom')],text=True).splitlines()
messages=[bytes.fromhex(line) for line in lines]
def get(data,field):
    values=[v for n,w,v in fields(data) if n==field]
    assert len(values)==1,(field,values)
    return values[0]
svc=services(messages[0]);media=get(svc[19],3);vc=get(media,4);ui=get(vc,11)
hd=os.environ.get('AA_TEST_HD','1')=='1'
assert [get(vc,i) for i in (1,2,3,4,5,8,9,10)]==([3,2,480,540,144,10000,128,3] if hd else [2,2,320,360,96,10000,85,3])
assert [get(get(ui,1),i) for i in (1,2,3,4)]==([270,270,240,240] if hd else [180,180,160,160])
assert [get(get(ui,2),i) for i in (1,2,3,4)]==([77,146,510,510] if hd else [51,97,340,340])
assert get(ui,2)==get(ui,3)
packed=get(get(svc[20],4),1);keys=[];pos=0
while pos<len(packed):value,pos=varint(packed,pos);keys.append(value)
assert keys==[168,169,65536]
for report,key,down in zip(messages[1:4],[168,168,169],[1,0,1]):
    item=get(get(report,4),1)
    assert [get(item,i) for i in (1,2,3,4)]==[key,down,0,0]
relative=get(get(messages[4],6),1)
assert get(relative,1)==65536 and get(relative,2)==2**64-2
assert 1920-480==1440 and 1080-540==540
print('PASS:', '1080p research profile' if hd else '720p working baseline', 'geometry, classic insets and input descriptors')
