#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <dlfcn.h>
#include <stdio.h>
int main(int argc,char **argv){
    void *stock,*probe;uintptr_t *a,*b,*table;uintptr_t before[48];
    assert(argc==3);
    stock=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL);assert(stock);
    a=dlsym(stock,"ipod_module");table=dlsym(stock,"ident_info_funcs");assert(a && table);
    memcpy(before,table,sizeof(before));
    probe=dlopen(argv[2],RTLD_NOW|RTLD_LOCAL);if(!probe){puts(dlerror());return 1;}
    b=dlsym(probe,"ipod_module");assert(b && !memcmp(a,b,9*sizeof(uintptr_t)));
    assert(!memcmp(before,table,sizeof(before)));
    assert(((int(*)(int))((uintptr_t*)b[8])[5])(10)==17);
    puts("PASS: all module fields and callback pointers preserved; original callback executes; no identification mutation");
    return 0;
}
