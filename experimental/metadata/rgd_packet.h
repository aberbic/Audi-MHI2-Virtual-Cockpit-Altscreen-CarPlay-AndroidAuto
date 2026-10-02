#ifndef RGD_PACKET_H
#define RGD_PACKET_H
#include <stddef.h>
#include <stdint.h>
/* Exact ARM32 layout inferred from stock packet builders, checked in adapter. */
struct rgd_native_packet {
    uint8_t opaque[24];
    uint8_t *storage;
    uint32_t total,capacity;
    uint8_t *control;
    uint16_t payload_length;
};
int rgd_packet_identify(struct rgd_native_packet *,uint16_t component);
#endif
