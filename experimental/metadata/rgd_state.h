#ifndef RGD_STATE_H
#define RGD_STATE_H
#include <stddef.h>
#include <stdint.h>

/* Offline prototype, NOT connected to driver, IPC, HMI or map ownership.
 * Caller serializes access and supplies explicit session/route epochs.
 * Epochs cannot be inferred reliably from isolated maneuver messages.
 * Caller must also expire stale data: this pure reducer has no clock/timeout. */
#define RGD_STATE_SLOTS 16
#define RGD_STATE_TEXT 128
#define RGD_STATE_EXIT 64
#define RGD_M_TYPE (1u << 0)
#define RGD_M_DESCRIPTION (1u << 1)
#define RGD_M_ROAD (1u << 2)
#define RGD_M_EXIT (1u << 3)
#define RGD_M_ANGLE (1u << 4)

struct rgd_maneuver {
    int used;
    uint16_t index;
    unsigned present;
    uint8_t type;
    int exit_angle;
    char description[RGD_STATE_TEXT], road[RGD_STATE_TEXT], exit[RGD_STATE_EXIT];
};
struct rgd_state {
    uint64_t session, epoch;
    uint16_t component;
    int route_known, route_state, source_support; /* -1 = source support unknown */
    int order_known;
    size_t order_count;
    uint16_t order[RGD_STATE_SLOTS];
    int distance_known, eta_known, remaining_known, destination_distance_known;
    uint32_t distance_m, destination_distance_m;
    uint64_t eta_seconds, remaining_seconds;
    char current_road[RGD_STATE_TEXT], destination[RGD_STATE_TEXT];
    struct rgd_maneuver slots[RGD_STATE_SLOTS];
};
struct rgd_current {
    struct rgd_maneuver maneuver;
    int distance_known;
    uint32_t distance_m;
};
void rgd_state_reset(struct rgd_state *, uint16_t component, uint64_t session, uint64_t epoch);
/* 1 = accepted; 0 = other component/session/epoch/message or inactive detail;
 * -1 = invalid/over capacity. State and input unchanged on 0/-1.
 * Complete messages only. Does not interpret lanes, publish or subscribe. */
int rgd_state_apply(struct rgd_state *, uint64_t session, uint64_t epoch,
                    const uint8_t *, size_t);
/* Copies a presentation candidate only if route, ordered slot and type exist.
 * Missing first slot never falls through to a later prefetched maneuver.
 * Output unchanged on 0. Conservative prototype: only states 1/6 eligible. */
int rgd_state_current(const struct rgd_state *, struct rgd_current *);
#endif
