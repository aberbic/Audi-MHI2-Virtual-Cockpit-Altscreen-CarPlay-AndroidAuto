/* QNX-only callback unit tests. No real driver or USB handles are used. */
#include "rgd_adapter.c"
#include <assert.h>
static int within_stock,send_count,packet_calls;
static void *expected_context=(void*)0x1234;
static int fake_packet(void *context){
    static const uint8_t ack[]={0x40,0x40,0,6,0x1d,2};
    assert(context==expected_context);within_stock=1;packet_calls++;
    received(NULL,ack,sizeof(ack));within_stock=0;errno=EAGAIN;return 0;
}
static int fake_send(void *context,const void *p,unsigned n){
    struct rgd_event event;assert(!within_stock && context==expected_context);
    assert(rgd_parse(p,n,&event)==1 && event.message==0x5200 && event.length[0]==2);
    assert(n==24 && ((const uint8_t*)p)[11]==16);send_count++;
    /* Simulate a competing receive callback while the request is in flight. */
    assert(sending);assert(receive_packet(context)==0 && send_count==1);
    errno=EIO;return 0;
}
static int fake_read(void *device,void *buffer,unsigned capacity,int timeout,unsigned flags){
    assert(device==expected_context && capacity==16 && timeout==5 && flags==7);
    memset(buffer,0xaa,3);errno=EINTR;return 3;
}
static int fake_ident(void *context,struct rgd_native_packet *p){(void)p;assert(context==expected_context);errno=EAGAIN;return 0;}
int main(void){
    uint8_t buffer[16];events=4000;original_packet=fake_packet;send_raw=fake_send;original_read=fake_read;
    rgd_monitor_init(&monitor,received,NULL);
    transport_ready=1;advertised=0;assert(receive_packet(expected_context)==0 && !send_count && errno==EAGAIN);
    advertised=1;assert(receive_packet(expected_context)==0 && !send_count && errno==EAGAIN);
    {
        static const uint8_t auth_ok[]={0x40,0x40,0,6,0xaa,5};
        received(NULL,auth_ok,sizeof(auth_ok));
    }
    assert(receive_packet(expected_context)==0 && send_count==1 && errno==EAGAIN);
    assert(receive_packet(expected_context)==0 && send_count==1 && packet_calls==5);
    memset(buffer,0x55,sizeof(buffer));assert(capture_read(expected_context,buffer,sizeof(buffer),5,7)==3 && errno==EINTR);
    assert(buffer[0]==0xaa && buffer[2]==0xaa && buffer[3]==0x55);
    {
        /* Exact rejected parameter from the real phone, without private data. */
        static const uint8_t rejection[]={0x40,0x40,0,14,0x1d,3,0,8,0,7,0x52,0,0x52,3};
        struct rgd_native_packet p,before;uint8_t bytes[100],original[100];
        memset(bytes,0x5a,sizeof(bytes));memcpy(original,bytes,sizeof(bytes));memset(&p,0,sizeof(p));
        p.storage=bytes;p.control=bytes+9;p.capacity=sizeof(bytes);before=p;
        original_ident=fake_ident;sent=0;
        received(NULL,rejection,sizeof(rejection));assert(rejected && !advertised && !accepted);
        assert(identify(expected_context,&p)==0 && errno==EAGAIN);
        assert(!memcmp(&p,&before,sizeof(p)) && !memcmp(bytes,original,sizeof(bytes)));
        assert(receive_packet(expected_context)==0 && send_count==1);
    }
    puts("PASS: request waits for stock authentication, concurrent dispatch cannot duplicate it, errno/bytes preserved, rejection uses unchanged-stock fallback");return 0;
}
