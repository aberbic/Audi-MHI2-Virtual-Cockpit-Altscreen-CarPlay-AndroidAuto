#define _GNU_SOURCE
#define MU1438_HOST_TEST 1
#define main renderer_program_main
#include "aa_renderer.c"
#undef main
#include <assert.h>
static unsigned char saved_attrs[64];
static int starts,configurations;
static int mock_init(void){starts++;return 0;}
static int mock_open(void **handle,const struct open_params *p){
    assert(p->output==1 && p->layer==60 && p->priority==100 && p->use_layer==1);
    *handle=(void*)1;return 0;
}
static int mock_config(void *handle,const struct config *c){
    assert(handle==(void*)1 && c->width==AA_VIDEO_WIDTH && c->height==AA_VIDEO_HEIGHT && c->fps==30.0f);
    configurations++;return 0;
}
static int mock_get(void *handle,void *p){assert(handle==(void*)1);memcpy(p,saved_attrs,64);return 0;}
static unsigned get32(const unsigned char *p,unsigned at){uint32_t value;memcpy(&value,p+at,4);return value;}
static int mock_set(void *handle,const void *raw){
    const unsigned char *p=raw;assert(handle==(void*)1);
    assert(get32(p,12)==AA_CROP_X && get32(p,16)==AA_CROP_Y);
    assert(get32(p,20)==AA_CROP_WIDTH && get32(p,24)==AA_CROP_HEIGHT);
    assert(get32(p,28)==0 && get32(p,32)==0 && get32(p,36)==1440 && get32(p,40)==540);
    assert(p[8]==1 && p[60]==1 && p[61]==0);memcpy(saved_attrs,p,64);return 0;
}
int main(void){
    init_kd=mock_init;open_video=mock_open;configure=mock_config;get_attrs=mock_get;set_attrs=mock_set;
    assert(prepare_renderer()==0 && starts==1 && configurations==1);
    assert(prepare_renderer()==0 && starts==1 && configurations==1);
    puts("PASS: actual renderer uses matching coded size/crop, unchanged output, visibility and 30fps configuration");return 0;
}
