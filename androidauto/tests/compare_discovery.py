"""Compare discovery structure without printing route/device identifying strings."""
from pathlib import Path

def varint(data,pos):
    value=0
    for shift in range(0,70,7):
        if pos>=len(data):raise ValueError('truncated varint')
        b=data[pos];pos+=1;value|=(b&127)<<shift
        if not b&128:return value,pos
    raise ValueError('oversized varint')

def fields(data):
    pos=0;out=[]
    while pos<len(data):
        tag,pos=varint(data,pos);field,wire=tag>>3,tag&7
        if wire==0:value,pos=varint(data,pos)
        elif wire==2:
            size,pos=varint(data,pos);value=data[pos:pos+size];pos+=size
            if len(value)!=size:raise ValueError('truncated bytes')
        elif wire in (1,5):
            size=8 if wire==1 else 4;value=data[pos:pos+size];pos+=size
            if len(value)!=size:raise ValueError('truncated fixed field')
        else:raise ValueError('unsupported wire type')
        out.append((field,wire,value))
    return out

def services(data):
    result={}
    for number,wire,value in fields(data):
        if number!=1 or wire!=2:continue
        item=fields(value)
        sid=next(v for n,w,v in item if n==1 and w==0)
        result[sid]=value
    return result

def display(data,prefix=''):
    for n,w,v in fields(data):
        name=f'{prefix}.{n}'
        if w==0:print(name,'=',v)
        elif w==2:
            print(name,'bytes=',len(v))
            if prefix=='' and n in (3,4):display(v,name)
            elif prefix=='.3' and n==4:display(v,name)
