#include <stdint.h>
#ifndef FIXTURE_ABI
#define FIXTURE_ABI 0x200
#endif
static int native_callback(int x){return x+7;}
uintptr_t fixture_interface[14]={(uintptr_t)"ipod_drvr",0,0,0,0,(uintptr_t)native_callback};
uintptr_t ipod_module[9]={(uintptr_t)"fixture",FIXTURE_ABI,0,0,0,0,0,0,(uintptr_t)fixture_interface};
uintptr_t ident_info_funcs[48];
uintptr_t iap2_ctrl_msg_table[768];
