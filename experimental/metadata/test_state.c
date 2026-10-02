#include "rgd_state.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* All messages below are synthetic; no captures, driver calls or sockets. */
static uint8_t frame[4096];
static size_t used;
static void put16(uint8_t *p, unsigned v) { p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v; }
static void bytes(unsigned id, const void *p, size_t n) {
    assert(used + n + 4 <= sizeof(frame));
    put16(frame + used, (unsigned)n + 4); put16(frame + used + 2, id);
    if (n) memcpy(frame + used + 4, p, n);
    used += 4 + n;
}
static void num(unsigned id, uint64_t value, unsigned width) {
    uint8_t p[8]; unsigned i;
    assert(width <= sizeof(p));
    for (i = 0; i < width; ++i) p[i] = (uint8_t)(value >> (8 * (width - 1 - i)));
    bytes(id, p, width);
}
static void start(unsigned message) {
    used = 6; frame[0] = frame[1] = 0x40; put16(frame + 4, message); num(0, 16, 2);
}
static void order(const unsigned *indexes, size_t n) {
    uint8_t p[64]; size_t i;
    assert(n <= sizeof(p) / 2);
    for (i = 0; i < n; ++i) put16(p + i * 2, indexes[i]);
    bytes(13, p, n * 2);
}
static void text(unsigned id, const char *s) { bytes(id, s, strlen(s) + 1); }
static int apply(struct rgd_state *s) {
    uint8_t before[4096]; int rc;
    put16(frame + 2, (unsigned)used); memcpy(before, frame, used);
    rc = rgd_state_apply(s, s->session, s->epoch, frame, used);
    assert(!memcmp(before, frame, used)); return rc;
}
static void detail(struct rgd_state *s, unsigned index, unsigned type, const char *road) {
    start(0x5202); num(1, index, 2); num(3, type, 1); text(4, road);
    assert(apply(s) == 1);
}
static void active(struct rgd_state *s, unsigned index) {
    start(0x5201); num(1, 1, 1); order(&index, 1); assert(apply(s) == 1);
}
static void absent(const struct rgd_state *s) {
    struct rgd_current out, before;
    memset(&out, 0xa5, sizeof(out)); before = out;
    assert(!rgd_state_current(s, &out)); assert(!memcmp(&before, &out, sizeof(out)));
}
static void rejected(struct rgd_state *s, int expected) {
    struct rgd_state before = *s;
    assert(apply(s) == expected); assert(!memcmp(&before, s, sizeof(before)));
}
static void selection_and_deltas(void) {
    struct rgd_state s; struct rgd_current c;
    unsigned indexes[] = {700, 4};
    rgd_state_reset(&s, 16, 100, 1);
    detail(&s, 4, 2, "Later Road"); absent(&s);
    start(0x5201); num(1, 1, 1); num(10, 120, 4); order(indexes, 2);
    text(3, "Example Avenue"); num(5, 1790000000, 8); num(6, 300, 8); num(7, 2400, 4);
    assert(apply(&s) == 1); absent(&s); /* no fallback to cached second turn */
    detail(&s, 700, 1, "Current Road");
    assert(rgd_state_current(&s, &c) && c.maneuver.index == 700 && c.distance_m == 120);
    start(0x5202); num(1, 700, 2); num(11, 0xffa6, 2); text(2, "Turn left"); text(13, "Exit A");
    assert(apply(&s) == 1); assert(rgd_state_current(&s, &c));
    assert(c.maneuver.exit_angle == -90 && !strcmp(c.maneuver.road, "Current Road"));
    assert(!strcmp(c.maneuver.description, "Turn left") && !strcmp(c.maneuver.exit, "Exit A"));
    start(0x5201); num(10, 80, 4); num(15, 0, 1); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && c.distance_m == 80); /* foreground flag is not route end */
    assert(s.eta_known && s.eta_seconds == 1790000000 && s.remaining_seconds == 300);
    assert(s.destination_distance_m == 2400 && !strcmp(s.current_road, "Example Avenue"));
    indexes[0] = 4;
    start(0x5201); order(indexes, 1); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && c.maneuver.index == 4 && !c.distance_known);
    start(0x5201); num(10, 0, 4); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && c.distance_known && c.distance_m == 0);
    start(0x5202); num(1, 4, 2); text(4, ""); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && !c.maneuver.road[0]);
    start(0x5201); order(NULL, 0); assert(apply(&s) == 1); absent(&s);
    assert(s.order_known && !s.order_count && !s.distance_known);
}
static void lifecycle(void) {
    struct rgd_state s, before; struct rgd_current c;
    rgd_state_reset(&s, 16, 10, 20); active(&s, 1); detail(&s, 1, 2, "Old Road");
    start(0x5201); num(1, 5, 1); assert(apply(&s) == 1); absent(&s);
    start(0x5201); num(1, 1, 1); { unsigned index = 1; order(&index, 1); }
    assert(apply(&s) == 1); absent(&s); /* reroute cannot reuse old slot */
    detail(&s, 1, 1, "New Road"); assert(rgd_state_current(&s, &c));
    start(0x5201); num(1, 0, 1); assert(apply(&s) == 1); absent(&s);
    assert(!s.slots[0].used && !s.eta_known && !s.current_road[0]);
    start(0x5202); num(1, 1, 2); num(3, 2, 1); rejected(&s, 0);
    active(&s, 1); absent(&s); detail(&s, 1, 1, "Fresh Road");
    start(0x5201); num(20, 0, 1); assert(apply(&s) == 1); absent(&s);
    start(0x5201); num(20, 1, 1); assert(apply(&s) == 1); absent(&s);
    active(&s, 1); absent(&s); detail(&s, 1, 2, "Supported Road");
    start(0x5201); num(1, 2, 1); assert(apply(&s) == 1); absent(&s);
    rgd_state_reset(&s, 16, 11, 21); absent(&s); before = s;
    assert(rgd_state_apply(&s, 10, 21, frame, used) == 0 && !memcmp(&s, &before, sizeof(s)));
    assert(rgd_state_apply(&s, 11, 20, frame, used) == 0 && !memcmp(&s, &before, sizeof(s)));
    detail(&s, 65535, 11, "Approach Road");
    start(0x5201); num(1, 6, 1); { unsigned index = 65535; order(&index, 1); }
    assert(apply(&s) == 1 && rgd_state_current(&s, &c));
    start(0x5201); num(1, 99, 1); assert(apply(&s) == 1); absent(&s);
}
static void bounds_and_transaction(void) {
    struct rgd_state s, before; struct rgd_current c;
    unsigned indexes[17], i; size_t n;
    char too_long[RGD_STATE_TEXT + 1];
    static const uint8_t bad_utf8[][4] = {{0xc0,0xaf,0,0}, {0xed,0xa0,0x80,0}, {0xf4,0x90,0x80,0x80}};
    rgd_state_reset(&s, 16, 1, 1); active(&s, 5); detail(&s, 5, 1, "Unchanged");
    start(0x5201); num(10, 5, 4); num(10, 6, 4); rejected(&s, -1);
    start(0x5202); num(3, 1, 1); rejected(&s, -1); /* missing maneuver index */
    start(0x5201); indexes[0] = indexes[1] = 5; order(indexes, 2); rejected(&s, -1);
    for (i = 0; i < 17; ++i) indexes[i] = i;
    start(0x5201); order(indexes, 17); rejected(&s, -1);
    memset(too_long, 'X', sizeof(too_long));
    start(0x5201); num(1, 0, 1); bytes(3, too_long, sizeof(too_long)); rejected(&s, -1);
    start(0x5202); num(1, 5, 2); num(3, 2, 1); bytes(4, too_long, sizeof(too_long)); rejected(&s, -1);
    for (i = 0; i < 3; ++i) {
        start(0x5202); num(1, 5, 2); bytes(4, bad_utf8[i], sizeof(bad_utf8[i])); rejected(&s, -1);
    }
    start(0x5202); num(1, 5, 2); bytes(4, "A\0B", 3); rejected(&s, -1);
    start(0x5202); num(1, 5, 2); text(4, "Bad\nRoad"); rejected(&s, -1);
    start(0x5202); num(1, 5, 2); bytes(13, too_long, RGD_STATE_EXIT); rejected(&s, -1);
    start(0x5202); num(1, 5, 2); bytes(4, too_long, RGD_STATE_TEXT - 1); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && strlen(c.maneuver.road) == RGD_STATE_TEXT - 1);
    start(0x5202); num(1, 5, 2); text(4, "Rue \xc3\x89" "cole"); assert(apply(&s) == 1);
    assert(rgd_state_current(&s, &c) && !strcmp(c.maneuver.road, "Rue \xc3\x89" "cole"));
    before = s;
    for (n = 0; n < used; ++n) {
        assert(rgd_state_apply(&s, 1, 1, frame, n) == -1); assert(!memcmp(&s, &before, sizeof(s)));
    }
    frame[11] = 17; rejected(&s, 0); /* other component */
    rgd_state_reset(&s, 16, 2, 1);
    for (i = 0; i < 16; ++i) detail(&s, i, 1, "Cached Road");
    start(0x5202); num(1, 16, 2); num(3, 1, 1); rejected(&s, -1);
    active(&s, 0); detail(&s, 16, 1, "Replaces unlisted slot");
    assert(rgd_state_current(&s, &c) && c.maneuver.index == 0); /* current not evicted */
    rgd_state_reset(&s, 16, 3, 1);
    start(0x5201); num(1, 1, 1); order(indexes, 16); assert(apply(&s) == 1);
    for (i = 0; i < 16; ++i) detail(&s, i, 1, "Listed Road");
    start(0x5202); num(1, 17, 2); num(3, 1, 1); rejected(&s, -1);
}
static void generated_inputs(void) {
    struct rgd_state s, before;
    uint32_t rng = 12345; unsigned i; size_t j;
    rgd_state_reset(&s, 16, 1, 1); active(&s, 3); detail(&s, 3, 1, "Synthetic Road");
    for (i = 0; i < 20000; ++i) {
        int rc;
        before = s; rng = rng * 1664525u + 1013904223u; used = rng % 256;
        for (j = 0; j < used; ++j) { rng = rng * 1664525u + 1013904223u; frame[j] = (uint8_t)(rng >> 24); }
        if (used >= 6 && (i & 1)) {
            frame[0] = frame[1] = 0x40; put16(frame + 2, (unsigned)used); put16(frame + 4, 0x5201 + i % 2);
        }
        rc = rgd_state_apply(&s, 1, 1, frame, used);
        if (rc != 1) assert(!memcmp(&before, &s, sizeof(s)));
        assert(s.order_count <= RGD_STATE_SLOTS);
    }
}
static void structured_sequences(void) {
    struct rgd_state s; struct rgd_current c;
    unsigned i;
    rgd_state_reset(&s, 16, 1, 1);
    for (i = 1; i <= 1000; ++i) {
        unsigned current = (i * 31) % 65536, future = (current + 1) % 65536;
        unsigned indexes[2] = {current, future};
        rgd_state_reset(&s, 16, 1, i);
        detail(&s, future, 2, "Future Road");
        start(0x5201); num(1, 1, 1); order(indexes, 2); num(10, i, 4); assert(apply(&s) == 1);
        absent(&s);
        detail(&s, current, 1, "Current Road");
        assert(rgd_state_current(&s, &c) && c.maneuver.index == current && c.distance_m == i);
        detail(&s, future, 2, "Updated Future Road");
        assert(rgd_state_current(&s, &c) && c.maneuver.index == current);
        start(0x5201); order(&future, 1); assert(apply(&s) == 1);
        assert(rgd_state_current(&s, &c) && c.maneuver.index == future && !c.distance_known);
        start(0x5201); num(1, 0, 1); assert(apply(&s) == 1); absent(&s);
    }
}
int main(void) {
    selection_and_deltas(); lifecycle(); bounds_and_transaction(); generated_inputs(); structured_sequences();
    puts("PASS: offline route reducer: current-list selection, deltas, signed angles, epochs, route end/reroute, bounds/UTF-8, 20000 generated inputs and 1000 structured sequences");
    return 0;
}
