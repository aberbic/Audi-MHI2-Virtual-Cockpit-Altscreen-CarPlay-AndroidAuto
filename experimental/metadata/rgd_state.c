#include "rgd_state.h"
#include "rgd_wire.h"
#include <string.h>

static unsigned be16(const uint8_t *p) { return ((unsigned)p[0] << 8) | p[1]; }
static int present(const struct rgd_event *e, unsigned id) { return !!(e->present & (1u << id)); }
/* Called only after complete structural validation by rgd_parse. */
static const uint8_t *field(const uint8_t *p, size_t n, unsigned id, size_t *size) {
    size_t at;
    for (at = 6; at < n; at += be16(p + at)) {
        if (be16(p + at + 2) == id) {
            *size = be16(p + at) - 4;
            return p + at + 4;
        }
    }
    *size = 0;
    return NULL;
}
/* Byte-bounded UTF-8. Accept an optional final NUL, reject embedded NUL,
 * controls, overlong encodings, surrogates and partial code points. No clipping. */
static int copy_text(char *out, size_t cap, const uint8_t *p, size_t n) {
    size_t i = 0, j;
    if (n && p[n - 1] == 0) --n;
    if (n >= cap) return -1;
    while (i < n) {
        unsigned c = p[i++], count, cp, minimum;
        if (c < 0x80) {
            if (c < 0x20 || c == 0x7f) return -1;
            continue;
        }
        if (c >= 0xc2 && c <= 0xdf) { count = 1; cp = c & 31; minimum = 0x80; }
        else if (c >= 0xe0 && c <= 0xef) { count = 2; cp = c & 15; minimum = 0x800; }
        else if (c >= 0xf0 && c <= 0xf4) { count = 3; cp = c & 7; minimum = 0x10000; }
        else return -1;
        if (count > n - i) return -1;
        for (j = 0; j < count; ++j) {
            c = p[i++];
            if ((c & 0xc0) != 0x80) return -1;
            cp = (cp << 6) | (c & 63);
        }
        if (cp < minimum || cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff) ||
            (cp >= 0x80 && cp <= 0x9f)) return -1;
    }
    memcpy(out, p, n); out[n] = 0;
    return 0;
}
static int text_field(char *out, size_t cap, const uint8_t *p, size_t n, unsigned id) {
    size_t size;
    const uint8_t *value = field(p, n, id, &size);
    return value ? copy_text(out, cap, value, size) : 0;
}
/* Validate supported fields even in a route-end update that will clear them.
 * A bad field must not allow an otherwise transactional message to clear state. */
static int validate_fields(unsigned message, const uint8_t *p, size_t n) {
    char scratch[RGD_STATE_TEXT];
    size_t size, i, j;
    const uint8_t *v;
    if (message == 0x5202) {
        return text_field(scratch, sizeof(scratch), p, n, 2) ||
               text_field(scratch, sizeof(scratch), p, n, 4) ||
               text_field(scratch, RGD_STATE_EXIT, p, n, 13);
    }
    if (text_field(scratch, sizeof(scratch), p, n, 3) ||
        text_field(scratch, sizeof(scratch), p, n, 4)) return -1;
    v = field(p, n, 13, &size);
    if (size / 2 > RGD_STATE_SLOTS) return -1;
    for (i = 0; i < size; i += 2)
        for (j = 0; j < i; j += 2) if (be16(v + i) == be16(v + j)) return -1;
    return 0;
}
void rgd_state_reset(struct rgd_state *s, uint16_t component, uint64_t session, uint64_t epoch) {
    memset(s, 0, sizeof(*s));
    s->component = component; s->session = session; s->epoch = epoch; s->source_support = -1;
}
static int listed(const struct rgd_state *s, unsigned index) {
    size_t i;
    for (i = 0; i < s->order_count; ++i) if (s->order[i] == index) return 1;
    return 0;
}
static struct rgd_maneuver *slot(struct rgd_state *s, uint16_t index) {
    size_t i;
    struct rgd_maneuver *free_slot = NULL, *unlisted = NULL;
    for (i = 0; i < RGD_STATE_SLOTS; ++i) {
        struct rgd_maneuver *m = &s->slots[i];
        if (m->used && m->index == index) return m;
        if (!m->used && !free_slot) free_slot = m;
        if (m->used && s->order_known && !listed(s, m->index) && !unlisted) unlisted = m;
    }
    if (!free_slot) free_slot = unlisted;
    if (!free_slot) return NULL;
    memset(free_slot, 0, sizeof(*free_slot)); free_slot->used = 1; free_slot->index = index;
    return free_slot;
}
static int order_field(struct rgd_state *s, const uint8_t *p, size_t n) {
    size_t size, i, j, old_count = s->order_count;
    unsigned old_first = old_count ? s->order[0] : 0;
    const uint8_t *v = field(p, n, 13, &size);
    if (!v) return 0;
    if (size / 2 > RGD_STATE_SLOTS) return -1;
    s->order_count = size / 2; s->order_known = 1;
    for (i = 0; i < s->order_count; ++i) {
        s->order[i] = (uint16_t)be16(v + i * 2);
        for (j = 0; j < i; ++j) if (s->order[i] == s->order[j]) return -1;
    }
    if (!s->order_count || !old_count || old_first != s->order[0]) {
        s->distance_known = 0; s->distance_m = 0;
    }
    return 0;
}
static int update_route(struct rgd_state *s, const struct rgd_event *e, const uint8_t *p, size_t n) {
    int old_support = s->source_support;
    if (present(e, 1)) {
        int next = (int)e->value[1];
        /* Entering no-route/arrival/reroute (or an unsupported state) discards
         * old geometry. Repeated reroute deltas do not erase newly cached data. */
        if (next != 1 && next != 6 && (!s->route_known || s->route_state != next)) {
            rgd_state_reset(s, s->component, s->session, s->epoch);
            s->source_support = old_support;
        }
        s->route_known = 1; s->route_state = next;
    }
    if (present(e, 20)) s->source_support = e->value[20] != 0;
    if (s->source_support == 0 || (s->route_known && (s->route_state == 0 || s->route_state == 2))) {
        int known = s->route_known, state = s->route_state, support = s->source_support;
        rgd_state_reset(s, s->component, s->session, s->epoch);
        s->route_known = known; s->route_state = state; s->source_support = support;
        return 0;
    }
    if (order_field(s, p, n) || text_field(s->current_road, sizeof(s->current_road), p, n, 3) ||
        text_field(s->destination, sizeof(s->destination), p, n, 4)) return -1;
    if (present(e, 5)) { s->eta_known = 1; s->eta_seconds = e->value[5]; }
    if (present(e, 6)) { s->remaining_known = 1; s->remaining_seconds = e->value[6]; }
    if (present(e, 7)) { s->destination_distance_known = 1; s->destination_distance_m = (uint32_t)e->value[7]; }
    /* Without a current-list head there is no safe distance/turn association. */
    if (present(e, 10) && s->order_count) { s->distance_known = 1; s->distance_m = (uint32_t)e->value[10]; }
    return 0;
}
int rgd_state_apply(struct rgd_state *s, uint64_t session, uint64_t epoch, const uint8_t *p, size_t n) {
    struct rgd_event e;
    struct rgd_state next;
    size_t size, i;
    const uint8_t *components;
    int rc, matches = 0;
    if (!s) return -1;
    if (session != s->session || epoch != s->epoch) return 0;
    rc = rgd_parse(p, n, &e);
    if (rc != 1) return rc;
    if (e.message != 0x5201 && e.message != 0x5202) return 0;
    components = field(p, n, 0, &size);
    if (!components || !size) return -1;
    for (i = 0; i < size; i += 2) if (be16(components + i) == s->component) matches = 1;
    if (!matches) return 0;
    if (validate_fields(e.message, p, n)) return -1;
    next = *s;
    if (e.message == 0x5201) {
        if (update_route(&next, &e, p, n)) return -1;
    } else {
        struct rgd_maneuver *m;
        if (s->source_support == 0 || (s->route_known && (s->route_state == 0 || s->route_state == 2))) return 0;
        if (!present(&e, 1)) return -1;
        m = slot(&next, (uint16_t)e.value[1]);
        if (!m) return -1;
        if (text_field(m->description, sizeof(m->description), p, n, 2) ||
            text_field(m->road, sizeof(m->road), p, n, 4) ||
            text_field(m->exit, sizeof(m->exit), p, n, 13)) return -1;
        if (present(&e, 2)) m->present |= RGD_M_DESCRIPTION;
        if (present(&e, 4)) m->present |= RGD_M_ROAD;
        if (present(&e, 13)) m->present |= RGD_M_EXIT;
        if (present(&e, 3)) { m->present |= RGD_M_TYPE; m->type = (uint8_t)e.value[3]; }
        if (present(&e, 11)) {
            unsigned angle = (unsigned)e.value[11];
            m->present |= RGD_M_ANGLE;
            m->exit_angle = angle >= 32768 ? (int)angle - 65536 : (int)angle;
        }
    }
    *s = next;
    return 1;
}
int rgd_state_current(const struct rgd_state *s, struct rgd_current *out) {
    size_t i;
    if (!s || !out || !s->route_known || (s->route_state != 1 && s->route_state != 6) ||
        s->source_support == 0 || !s->order_known || !s->order_count) return 0;
    for (i = 0; i < RGD_STATE_SLOTS; ++i) {
        const struct rgd_maneuver *m = &s->slots[i];
        if (m->used && m->index == s->order[0] && (m->present & RGD_M_TYPE)) {
            memset(out, 0, sizeof(*out)); out->maneuver = *m;
            out->distance_known = s->distance_known; out->distance_m = s->distance_m;
            return 1;
        }
    }
    return 0;
}
