/* Narrow draw trace for watched scenery (setting scenery_trace, or NFSMW_TEST_SCENERY_LOD): for one model only - by
 * default the Rosewood College sign (XS_RW_RosewoodCollege_1); NFSMW_SIGN_WATCH=<text> picks another SceneryInfo name -
 * its render entries (solid, effect type, the entry's texture/material words) and whether every entry it submitted was
 * actually drawn, and with the light-pool pass or not. The scenery probe adds an instance to the watch when it first
 * sees one whose name contains the text. Hooks used: world_lights.h's render-item and draw-entry hooks (installed with
 * the light pools or with scenery_trace on). Lines are written only on a change:
 *   SIGN items: the instance's parts changed (count, solid, effect type, +0x24; +0x1c printed) - with the time;
 *   SIGN drawn: the previous submission drew fewer entries than it submitted, or the extra-pass flags changed.
 * Read only. Up to 4000 lines. */
#ifndef SIGN_TRACE_H
#define SIGN_TRACE_H
#include <string.h>
#include <sys/time.h>
#include <time.h>
#define ST_WATCH 16
static int g_st_on = -1;
static uint32_t g_st_inst[ST_WATCH], g_st_n, g_st_lines;
static uint32_t g_st_sig[ST_WATCH], g_st_parts[ST_WATCH];
static uint32_t g_st_lo[ST_WATCH], g_st_hi[ST_WATCH], g_st_drawn[ST_WATCH], g_st_flags[ST_WATCH], g_st_last_flags[ST_WATCH];
static uint32_t g_st_solid[ST_WATCH][8], g_st_mask[ST_WATCH];   /* entries drawn once each, matched by solid */
static const char *st_watch_text(void) {
    const char *w = getenv("NFSMW_SIGN_WATCH");
    return (w && *w) ? w : "RosewoodCollege";
}
static int st_enabled(void) {
    if (g_st_on < 0) {
        const char *e = getenv("NFSMW_TEST_SCENERY_LOD");
        g_st_on = (e && *e && *e != '0') || setting("scenery_trace", 0) != 0;
    }
    return g_st_on;
}
static int st_find(uint32_t inst) {
    for (uint32_t i = 0; i < g_st_n; ++i) if (g_st_inst[i] == inst) return (int)i;
    return -1;
}
/* From the scenery probe, once per newly seen instance: watch it if its name matches. */
static void sign_trace_consider(const PopModApi *api, uint32_t inst, const char *name, float x, float y, float z) {
    if (!st_enabled() || !strstr(name, st_watch_text()) || st_find(inst) >= 0) return;
    const uint32_t k = g_st_n < ST_WATCH ? g_st_n++ : (inst >> 6) % ST_WATCH;   /* full: reuse a slot */
    g_st_inst[k] = inst; g_st_sig[k] = 0; g_st_parts[k] = 0; g_st_lo[k] = g_st_hi[k] = 0;
    g_st_drawn[k] = 0; g_st_flags[k] = 0; g_st_last_flags[k] = 0xffffffffu;
    if (g_st_lines < 4000) {
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: SIGN watch %08x '%s' at %.1f %.1f %.1f (slot %u)", inst, name, x, y, z, k);
        api->log(api, line);
        ++g_st_lines;
    }
}
static void st_time(char *out, size_t n) {
    struct timeval tv; gettimeofday(&tv, NULL);
    struct tm tm; const time_t s = tv.tv_sec; localtime_r(&s, &tm);
    snprintf(out, n, "%02d:%02d:%02d.%03d", tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000));
}
/* From x3sl_render_item after the original ran: the instance produced render entries [before, after). */
static void sign_trace_item(const PopModApi *api, uint32_t inst, uint32_t before, uint32_t after) {
    if (g_st_on <= 0 || !g_st_n) return;
    const int k = st_find(inst);
    if (k < 0) return;
    char t[16];
    /* The previous submission of this instance: all entries drawn? which extra passes? */
    if (g_st_hi[k] > g_st_lo[k] && g_st_lines < 4000 &&
        (g_st_drawn[k] < g_st_hi[k] - g_st_lo[k] || g_st_flags[k] != g_st_last_flags[k])) {
        st_time(t, sizeof t);
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: SIGN drawn %s frame %u inst %08x: %u of %u entries drawn, extra-pass flags %x (was %x)",
                 t, get_u32(0x009885b8u), inst, g_st_drawn[k], g_st_hi[k] - g_st_lo[k], g_st_flags[k], g_st_last_flags[k]);
        api->log(api, line);
        ++g_st_lines;
    }
    if (g_st_hi[k] > g_st_lo[k]) g_st_last_flags[k] = g_st_flags[k];
    if (after > before + 8) after = before + 8;
    g_st_lo[k] = before; g_st_hi[k] = after; g_st_drawn[k] = 0; g_st_flags[k] = 0; g_st_mask[k] = 0;
    for (uint32_t i = before; i < after; ++i) g_st_solid[k][i - before] = get_u32(0x0093e878u + i * 0x44u);
    /* What it submitted: a signature over the parts. */
    uint32_t sig = 2166136261u, n = after > before ? after - before : 0;
    char parts[160] = ""; int len = 0;
    for (uint32_t i = before; i < after && i < before + 8; ++i) {
        const uint32_t e = 0x0093e878u + i * 0x44u, eff = get_u32(e + 0x10);
        const uint32_t w[4] = {get_u32(e), eff ? get_u32(eff + 4) : 0xff, get_u32(e + 0x1c), get_u32(e + 0x24)};
        /* +0x1c changes every frame (a per-frame pointer, Test145 first run), so it is printed, not compared. */
        sig = (sig ^ w[0]) * 16777619u; sig = (sig ^ w[1]) * 16777619u; sig = (sig ^ w[3]) * 16777619u;
        if (len < (int)sizeof parts - 40)
            len += snprintf(parts + len, sizeof parts - (size_t)len, " [%08x t%u %08x %08x]", w[0], w[1], w[2], w[3]);
    }
    if ((sig != g_st_sig[k] || n != g_st_parts[k]) && g_st_lines < 4000) {
        st_time(t, sizeof t);
        char line[320];
        snprintf(line, sizeof line, "core.nfsmw: SIGN items %s frame %u inst %08x: %u entries%s", t, get_u32(0x009885b8u), inst, n, parts);
        api->log(api, line);
        ++g_st_lines;
    }
    g_st_sig[k] = sig; g_st_parts[k] = n;
}
/* From wl_draw_entry: render entry `index` is being drawn; extra = bit0 light-pool pass. */
/* Counted once per entry, and only while the entry still holds the solid this instance submitted: the render list
 * is reused by every view's flush, so an index alone would also count other objects (Test145 first run). */
static void sign_trace_draw(uint32_t index, uint32_t solid, uint32_t extra) {
    if (g_st_on <= 0 || !g_st_n) return;
    for (uint32_t k = 0; k < g_st_n; ++k)
        if (index >= g_st_lo[k] && index < g_st_hi[k]) {
            const uint32_t bit = 1u << (index - g_st_lo[k]);
            if ((g_st_mask[k] & bit) || g_st_solid[k][index - g_st_lo[k]] != solid) continue;
            g_st_mask[k] |= bit; ++g_st_drawn[k]; g_st_flags[k] |= extra;
        }
}
#endif
