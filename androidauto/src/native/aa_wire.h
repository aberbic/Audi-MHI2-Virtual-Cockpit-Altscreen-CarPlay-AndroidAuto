#ifndef AA_WIRE_H
#define AA_WIRE_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "aa_layout.h"
#ifndef AA_CLASSIC_LAYOUT
#define AA_CLASSIC_LAYOUT 0
#endif
#ifndef AA_HD
#define AA_HD 0
#endif
#ifndef AA_ZOOM
#define AA_ZOOM 0
#endif
struct aa_pb { unsigned char *data;size_t used,capacity;int bad; };
static void aa_byte(struct aa_pb *b,unsigned v) { if(b->used>=b->capacity)b->bad=1;else b->data[b->used++]=(unsigned char)v; }
static void aa_var(struct aa_pb *b,uint64_t v) { do{unsigned c=(unsigned)(v&127);v>>=7;aa_byte(b,c|(v?128:0));}while(v); }
static void aa_int(struct aa_pb *b,unsigned field,uint64_t value) { aa_var(b,(uint64_t)field<<3);aa_var(b,value); }
static void aa_bytes(struct aa_pb *b,unsigned field,const void *data,size_t length)
{
    aa_var(b,((uint64_t)field<<3)|2);aa_var(b,length);
    if(b->bad || length>b->capacity-b->used){b->bad=1;return;}
    memcpy(b->data+b->used,data,length);b->used+=length;
}
static int aa_takevar(const unsigned char *p,size_t n,size_t *at,uint64_t *value)
{
    unsigned shift;*value=0;
    for(shift=0;shift<64;shift+=7){unsigned c;if(*at>=n)return -1;c=p[(*at)++];if(shift==63&&(c&254))return -1;*value|=(uint64_t)(c&127)<<shift;if(!(c&128))return 0;}
    return -1;
}
static int aa_getint(const unsigned char *p,size_t n,unsigned field,uint32_t *value)
{
    size_t at=0;int found=0;
    while(at<n){uint64_t tag,v;if(aa_takevar(p,n,&at,&tag)||!(tag>>3))return -1;
        switch(tag&7){
        case 0:if(aa_takevar(p,n,&at,&v))return -1;if(tag>>3==field){if(v>UINT32_MAX)return -1;*value=(uint32_t)v;found=1;}break;
        case 2:if(aa_takevar(p,n,&at,&v)||v>n-at)return -1;at+=(size_t)v;break;
        case 1:if(n-at<8)return -1;at+=8;break;
        case 5:if(n-at<4)return -1;at+=4;break;
        default:return -1;
        }
    }
    return found;
}
/* Append two complete ServiceDiscoveryResponse.services entries. */
static void aa_insets(struct aa_pb *dst,unsigned field,unsigned top,unsigned bottom,unsigned left,unsigned right)
{
    unsigned char data[32];struct aa_pb p={data,0,sizeof(data),0};
    aa_int(&p,1,top);aa_int(&p,2,bottom);aa_int(&p,3,left);aa_int(&p,4,right);
    if(p.bad)dst->bad=1;else aa_bytes(dst,field,data,p.used);
}
static size_t aa_cluster_services(unsigned char *out,size_t capacity)
{
    unsigned char vc_data[128],ms_data[160],svc_data[192],input_data[40];
    struct aa_pb vc={vc_data,0,sizeof(vc_data),0},ms={ms_data,0,sizeof(ms_data),0};
    struct aa_pb svc={svc_data,0,sizeof(svc_data),0},input={input_data,0,sizeof(input_data),0};
    struct aa_pb result={out,0,capacity,0};
    aa_int(&vc,1,AA_HD?3:2);aa_int(&vc,2,2);
    if(AA_CLASSIC_LAYOUT){
        unsigned char ui_data[80];struct aa_pb ui={ui_data,0,sizeof(ui_data),0};
        /* Vendor native 1920x1080: margins 480x540, DPI144; classic content
         * insets (77,146,510,510). Scale all pixel lengths by 2/3 for 720p.
         * Crop (160,180,960,360) then scale 1.5 to physical 1440x540. */
        aa_int(&vc,3,AA_VIDEO_WIDTH-AA_CROP_WIDTH);aa_int(&vc,4,AA_VIDEO_HEIGHT-AA_CROP_HEIGHT);aa_int(&vc,5,AA_DPI);
        aa_int(&vc,6,0);aa_int(&vc,7,500);aa_int(&vc,8,10000);aa_int(&vc,9,AA_REAL_DPI);aa_int(&vc,10,3);
        aa_insets(&ui,1,AA_CROP_Y,AA_CROP_Y,AA_CROP_X,AA_CROP_X);
        aa_insets(&ui,2,AA_INSET_TOP,AA_INSET_BOTTOM,AA_INSET_SIDE,AA_INSET_SIDE);
        aa_insets(&ui,3,AA_INSET_TOP,AA_INSET_BOTTOM,AA_INSET_SIDE,AA_INSET_SIDE);
        aa_int(&ui,4,0);
        if(ui.bad)vc.bad=1;else aa_bytes(&vc,11,ui.data,ui.used);
    }else{
        aa_int(&vc,3,0);aa_int(&vc,4,0);aa_int(&vc,5,160);aa_int(&vc,8,10000);aa_int(&vc,10,3);
    }
    aa_int(&ms,1,3);aa_bytes(&ms,4,vc.data,vc.used);aa_int(&ms,5,1);aa_int(&ms,6,1);aa_int(&ms,7,1);
    aa_int(&svc,1,19);aa_bytes(&svc,3,ms.data,ms.used);aa_bytes(&result,1,svc.data,svc.used);
    svc.used=0;aa_int(&svc,1,20);
    if(AA_ZOOM){
        unsigned char packed[16];struct aa_pb keys={packed,0,sizeof(packed),0};
        aa_var(&keys,168);aa_var(&keys,169);aa_var(&keys,65536);
        aa_bytes(&input,1,keys.data,keys.used);
    }
    aa_int(&input,5,1);aa_bytes(&svc,4,input.data,input.used);aa_bytes(&result,1,svc.data,svc.used);
    return result.bad||vc.bad||ms.bad||svc.bad||input.bad?0:result.used;
}
static size_t aa_zoom_report(unsigned char *out,size_t capacity,uint64_t micros,int steps,int rotary,int down)
{
    unsigned char item_data[40],event_data[48];
    struct aa_pb item={item_data,0,sizeof(item_data),0},event={event_data,0,sizeof(event_data),0},report={out,0,capacity,0};
    aa_int(&report,1,micros);
    if(rotary){aa_int(&item,1,65536);aa_int(&item,2,(uint64_t)(int64_t)steps);}
    else {aa_int(&item,1,steps<0?168:169);aa_int(&item,2,down?1:0);aa_int(&item,3,0);aa_int(&item,4,0);}
    aa_bytes(&event,1,item.data,item.used);aa_bytes(&report,rotary?6:4,event.data,event.used);
    return item.bad||event.bad||report.bad?0:report.used;
}
#endif
