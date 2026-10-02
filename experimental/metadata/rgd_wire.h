#ifndef MU1438_RGD_WIRE_H
#define MU1438_RGD_WIRE_H
#include <stddef.h>
#include <stdint.h>

/* Complete iAP2 control messages only; no raw-USB magic-byte scanning.
 * Phase 1 records numeric values and text lengths, never private text. */
struct rgd_event {
    uint16_t message;
    uint32_t present, numeric;
    uint64_t value[32];
    uint16_t length[32];
    unsigned unknown_fields;
};
/* 1=guidance event, 0=other message, -1=malformed/ambiguous message.
 * out is unchanged on 0/-1. */
int rgd_parse(const uint8_t *frame, size_t size, struct rgd_event *out);
/* Pure offline builder. Preserve existing identification fields; add required
 * message declarations and one component. Caller supplies separate storage.
 * Never accepts overlapping input/output or rewrites a pre-existing component.
 * 0=failure, otherwise output length. Output unchanged on failure. */
size_t rgd_identify(const uint8_t *frame, size_t size, uint16_t component,
                   uint8_t *out, size_t capacity);
#endif
