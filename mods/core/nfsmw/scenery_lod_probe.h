/* TEST ONLY (Test127, NFSMW_TEST_SCENERY_LOD=1): why does a piece of scenery appear late (report #127)?
 * Observes the game's own scenery selector; writes no game state. Read on exe c0516b48 (2 Oct 2026):
 *  - 00729120 (thiscall, RET 0xc; ecx = scenery section, args: instance index, cull info, frustum-test flag) decides
 *    one instance. Order: preculler bit (cull+0xc4 cell, per-section mask at section+0x30) -> instance flags ->
 *    optional frustum test -> projected size from 00729060 -> thresholds -> model slot -> 12-byte draw item
 *    {model | flag, matrix, instance} appended at cull+0x8c.
 *  - Projected size (00729060): r = SceneryInfo+0x38 + 6.0; d = |instance+0x20 - cull+0xa0|;
 *    size = (d - r > r) ? K * r / (d - r) : K, K = cull+0xc0 = View+0xc = (screen width / 2) / tan(hfov / 2)
 *    (006be9f0, width DAT_00982be4). Behind the camera (dot with cull+0xb0 < -r): 0.
 *  - Normal single view: size < 2 or < 18 culled; world detail >= 2 picks model slot +0x28, else +0x30 (also +0x30
 *    after the density test: slot +0x28's mesh > 39, d >= 25, size / max(density, 6) < 8.7). Split screen: < 23.
 *    Reflection / special views (flags 0x1800 or 0x20): < 32. Instance flag 0x2000000 adds 10.
 * Only the player-1 main view (cull+0x80 == 00919650) is logged. Per instance the last outcome is kept; a line is
 * written when a visible-size instance (>= 24 px) changes outcome:
 *   stream  - first seen while its section's run of evaluations is at most 1 frame old (newly resident or newly in
 *             view at section level);
 *   cull>draw / draw>cull - reason: precull, flags/frustum (refused at >= 18 px), small (below the threshold);
 *   lod     - drawn both frames, model slot changed.
 * The names come from SceneryInfo+0 (24 chars; HYPOTHESIS: the debug name). Slot 96 (Test122 check_hook_slots). */
#ifndef SCENERY_LOD_PROBE_H
#define SCENERY_LOD_PROBE_H
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#define SLP_VIEW_PLAYER1 0x00919650u /* cull+0x80 for the main view (00731b80 / 006e44c0) */
#define SLP_FRAME 0x009885b8u        /* game frame counter (as audio_env_probe.h) */
#define SLP_TABLE 65536u
#define SLP_LOG_LIMIT 20000u
static uint32_t g_slp_log_limit = SLP_LOG_LIMIT; /* 200000 when the player turns it on (scenery_trace, Test139) */
enum { SLP_NONE, SLP_DRAWN, SLP_PRECULL, SLP_FLAGS, SLP_SMALL, SLP_FULL };
static const char *const k_slp_name[] = {"none", "drawn", "precull", "flags/frustum", "small", "list-full"};
typedef struct { uint32_t section, index, frame; uint8_t outcome, slot; } SlpEntry;
typedef struct { uint32_t section, last, start; } SlpSection;
static SlpEntry *g_slp;
static SlpSection g_slp_sections[8192];
static int g_slp_on;
static uint64_t g_slp_calls, g_slp_lines, g_slp_count[6];
/* The shared scenery draw list (00731b80): 0x00994368..0x009a2dc8 = 5000 items of 12 bytes for ALL views of a frame;
 * 00729120 silently skips an instance when the list is full (cursor >= cull+0x90). HYPOTHESIS (#127): at 3840 wide
 * the doubled projection scale admits far more instances, the list fills, and the instances visited last flicker. */
#define SLP_LIST_BEGIN 0x00994368u
#define SLP_LIST_ITEMS 5000u
static uint32_t g_slp_frame = 0xffffffffu, g_slp_frame_peak, g_slp_frame_drops, g_slp_frame_main_drops;
/* per view class (006bfdd0's view order; cull+0x80 = the view): inserts and skips this frame and in total, and the
 * class that first met a full list this frame (external review S2: the hypothesis predicts that later views starve first) */
enum { SLV_MAIN, SLV_MIRROR, SLV_ENV, SLV_ROAD, SLV_AUX, SLV_SHADOW, SLV_OTHER, SLV_N };
static const char *const k_slv_name[SLV_N] = {"main", "view3", "env", "road", "aux", "shadow", "other"};
static uint32_t g_slv_frame_ins[SLV_N], g_slv_frame_skip[SLV_N];
static uint64_t g_slv_ins[SLV_N], g_slv_skip[SLV_N], g_slv_first_full[SLV_N];
static int g_slv_frame_first = -1;
static int slp_view_class(uint32_t view) {
    if (view == 0x00919650u) return SLV_MAIN;
    if (view == 0x00919730u) return SLV_MIRROR;          /* the third player-view slot (flags 0x1000 in 006bfdd0) */
    if (view >= 0x00919dc0u && view < 0x00919ff0u) return SLV_ENV;   /* six env-map faces, 0x70 apart */
    if (view == 0x009197a0u) return SLV_ROAD;            /* road reflection (flags 0x800) */
    if (view == 0x00919960u) return SLV_AUX;
    if (view == 0x00919c70u) return SLV_SHADOW;          /* flags 0x4100 */
    return SLV_OTHER;
}
static uint32_t g_slp_peak;
static uint64_t g_slp_drops, g_slp_full_frames, g_slp_frames;

static float slp_f32(uint32_t a) { uint32_t v = get_u32(a); float f; memcpy(&f, &v, 4); return f; }
static int16_t slp_i16(uint32_t a) { /* unaligned-safe */
    const uint32_t lo = get_u32(a & ~3u), hi = get_u32((a & ~3u) + 4u);
    const uint64_t both = (uint64_t)lo | (uint64_t)hi << 32;
    return (int16_t)(uint16_t)(both >> ((a & 3u) * 8u));
}
/* frames since this section's current run of evaluations began (a gap of more than 2 frames starts a new run) */
static uint32_t slp_section_age(uint32_t section, uint32_t frame) {
    SlpSection *s = &g_slp_sections[(section >> 4) & 8191u];
    if (s->section != section || frame - s->last > 2u) { s->section = section; s->start = frame; }
    s->last = frame;
    return frame - s->start;
}
static void slp_wrap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t section = cpu->ecx, sp = cpu->esp;
    const uint32_t index = get_u32(sp + 4u), cull = get_u32(sp + 8u);
    const uint32_t cursor = cull ? get_u32(cull + 0x8cu) : 0;
    const int full = cull && cursor >= get_u32(cull + 0x90u);
    api->call_next(api, inv, cpu);
    ++g_slp_calls;
    if (!g_slp || !section || !cull) return;
    const uint32_t frame = get_u32(SLP_FRAME);
    /* list occupancy, every view */
    if (frame != g_slp_frame) {
        if (g_slp_frame != 0xffffffffu) {
            ++g_slp_frames;
            if (g_slp_frame_peak > g_slp_peak) g_slp_peak = g_slp_frame_peak;
            if (g_slp_frame_drops) {
                ++g_slp_full_frames;
                if (g_slv_frame_first >= 0) ++g_slv_first_full[g_slv_frame_first];
                if (g_slp_lines < SLP_LOG_LIMIT && (g_slp_full_frames <= 50u || g_slp_full_frames % 300u == 0u)) {
                    char line[448];
                    int n = snprintf(line, sizeof line, "core.nfsmw: TEST scenery: frame %u draw list FULL (%u of %u items), "
                             "%u instances skipped (%u in the main view); first full view: %s; inserted/skipped by view:",
                             g_slp_frame, g_slp_frame_peak, SLP_LIST_ITEMS, g_slp_frame_drops, g_slp_frame_main_drops,
                             g_slv_frame_first >= 0 ? k_slv_name[g_slv_frame_first] : "?");
                    for (int v = 0; v < SLV_N && n > 0 && n < (int)sizeof line; ++v)
                        if (g_slv_frame_ins[v] || g_slv_frame_skip[v])
                            n += snprintf(line + n, sizeof line - (size_t)n, " %s %u/%u", k_slv_name[v], g_slv_frame_ins[v], g_slv_frame_skip[v]);
                    api->log(api, line);
                    ++g_slp_lines;
                }
            }
        }
        g_slp_frame = frame;
        g_slp_frame_peak = g_slp_frame_drops = g_slp_frame_main_drops = 0;
        memset(g_slv_frame_ins, 0, sizeof g_slv_frame_ins);
        memset(g_slv_frame_skip, 0, sizeof g_slv_frame_skip);
        g_slv_frame_first = -1;
    }
    const uint32_t after_any = get_u32(cull + 0x8cu);
    if (after_any >= SLP_LIST_BEGIN && (after_any - SLP_LIST_BEGIN) / 12u > g_slp_frame_peak)
        g_slp_frame_peak = (after_any - SLP_LIST_BEGIN) / 12u;
    const uint32_t view_ptr = get_u32(cull + 0x80u);
    const int main_view = view_ptr == SLP_VIEW_PLAYER1, vclass = slp_view_class(view_ptr);
    if (after_any > cursor) { ++g_slv_frame_ins[vclass]; ++g_slv_ins[vclass]; }
    if (full && after_any == cursor) {
        ++g_slp_drops;
        ++g_slp_frame_drops;
        ++g_slv_frame_skip[vclass]; ++g_slv_skip[vclass];
        if (g_slv_frame_first < 0) g_slv_frame_first = vclass;
        if (main_view) ++g_slp_frame_main_drops;
    }
    if (!main_view) return;
    const uint32_t inst = get_u32(section + 0x20u) + index * 0x40u;
    const uint32_t info = get_u32(section + 0x18u) + (uint32_t)(int32_t)slp_i16(inst + 0x3eu) * 0x48u;
    /* outcome */
    uint8_t outcome = SLP_SMALL, slot = 0xff;
    const uint32_t after = get_u32(cull + 0x8cu);
    if (after > cursor) {
        outcome = SLP_DRAWN;
        const uint32_t model = get_u32(cursor) & ~3u;
        for (uint8_t k = 0; k < 4; ++k)
            if (get_u32(info + 0x28u + 4u * k) == model) slot = k;
    } else {
        const int32_t cell = (int32_t)get_u32(cull + 0xc4u);
        if (full) outcome = SLP_FULL; /* refined below only when not full */
        if (!full && cell >= 0) {
            const uint32_t mask = get_u32(section + 0x30u) + (uint32_t)(int32_t)slp_i16(inst + 0x1cu) * 0x80u + ((uint32_t)cell >> 3);
            if ((get_u32(mask & ~3u) >> ((mask & 3u) * 8u)) & (1u << (cell & 7))) outcome = SLP_PRECULL;
        }
    }
    /* projected size, as 00729060 (truncated like _ftol) */
    const float r = slp_f32(info + 0x38u) + 6.0f, k = slp_f32(cull + 0xc0u);
    const float dx = slp_f32(inst + 0x20u) - slp_f32(cull + 0xa0u), dy = slp_f32(inst + 0x24u) - slp_f32(cull + 0xa4u),
                dz = slp_f32(inst + 0x28u) - slp_f32(cull + 0xa8u);
    const float ahead = dx * slp_f32(cull + 0xb0u) + dy * slp_f32(cull + 0xb4u) + dz * slp_f32(cull + 0xb8u);
    const float d = sqrtf(dx * dx + dy * dy + dz * dz);
    int size = 0;
    if (!(ahead < -r)) size = (int)((d - r > r) ? k * r / (d - r) : k);
    if (outcome == SLP_SMALL && size >= 18) outcome = SLP_FLAGS; /* refused before the size test (flags/frustum) */
    ++g_slp_count[outcome];
    /* history */
    const uint32_t section_age = slp_section_age(section, frame);
    SlpEntry *e = &g_slp[((section >> 4) * 2654435761u ^ index * 40503u) & (SLP_TABLE - 1u)];
    const int known = e->section == section && e->index == index && frame - e->frame <= 2u;
    if (!known && g_st_on > 0) {   /* Test145: a newly seen instance - watch it if it is the sign */
        char wname[25] = {0};
        for (int i = 0; i < 24; ++i) {
            const char c = (char)(get_u32((info + (uint32_t)i) & ~3u) >> (((info + (uint32_t)i) & 3u) * 8u));
            if (!c) break;
            wname[i] = (c >= 32 && c < 127) ? c : '?';
        }
        sign_trace_consider(api, inst, wname, slp_f32(inst + 0x20u), slp_f32(inst + 0x24u), slp_f32(inst + 0x28u));
    }
    const uint8_t was = known ? e->outcome : SLP_NONE, was_slot = known ? e->slot : 0xff;
    e->section = section; e->index = index; e->frame = frame; e->outcome = outcome; e->slot = slot;
    if (size < 24 || g_slp_lines >= g_slp_log_limit) return;
    const char *kind = NULL;
    if (!known && outcome == SLP_DRAWN && section_age <= 1u) kind = "stream";
    else if (known && (was == SLP_DRAWN) != (outcome == SLP_DRAWN)) kind = outcome == SLP_DRAWN ? "cull>draw" : "draw>cull";
    else if (known && was == SLP_DRAWN && outcome == SLP_DRAWN && was_slot != slot) kind = "lod";
    if (!kind) return;
    char name[25] = {0};
    for (int i = 0; i < 24; ++i) {
        const char c = (char)(get_u32((info + (uint32_t)i) & ~3u) >> (((info + (uint32_t)i) & 3u) * 8u));
        if (!c) break;
        name[i] = (c >= 32 && c < 127) ? c : '?';
    }
    /* Test139b: which test refused a "flags/frustum" instance, as 00729120 orders them: instance flag 0x100 asks
     * 006c29a0 (a condition we do not evaluate: "cond?"); then the view-flag mask (flags, with 0x40 or 0x8000000
     * widened to 0x8000040, XOR 0xffffff60, AND cull+0x84, AND 0x80000ff); otherwise the frustum test 004fca70. */
    const uint32_t iflags = get_u32(inst + 0x18u), vflags = get_u32(cull + 0x84u);
    const char *why = "";
    if (outcome == SLP_FLAGS) {
        uint32_t f = iflags;
        if ((f & 0x8000000u) || (f & 0x40u)) f |= 0x8000040u;
        why = (iflags & 0x100u) ? " why cond?" : (((f ^ 0xffffff60u) & vflags & 0x80000ffu) ? " why viewflags" : " why frustum");
    }
    /* Test139: wall-clock time on every line, so an owner report ("it flickered at 9:42") finds its frame. */
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tm;
    const time_t sec = tv.tv_sec;
    localtime_r(&sec, &tm);
    char line[352];
    snprintf(line, sizeof line,
        "core.nfsmw: TEST scenery: %02d:%02d:%02d.%03d frame %u %s %s->%s slot %d->%d size %d dist %.1f radius %.1f K %.1f pos %.1f %.1f %.1f "
        "section %08x idx %u info %08x '%s'%s iflags %08x vflags %08x",
        tm.tm_hour, tm.tm_min, tm.tm_sec, (int)(tv.tv_usec / 1000), frame, kind, k_slp_name[was], k_slp_name[outcome], was_slot == 0xff ? -1 : was_slot, slot == 0xff ? -1 : slot,
        size, d, r - 6.0f, k, slp_f32(inst + 0x20u), slp_f32(inst + 0x24u), slp_f32(inst + 0x28u), section, index, info,
        name, why, iflags, vflags);
    api->log(api, line);
    ++g_slp_lines;
}
static void scenery_lod_probe_init(const PopModApi *api) {
    const char *on = getenv("NFSMW_TEST_SCENERY_LOD");
    const int owner = setting("scenery_trace", 0) != 0; /* Test139: the player's diagnostics switch (restart) */
    if ((!on || !*on || *on == '0') && !owner) return;
    if (owner) g_slp_log_limit = 200000u;
    g_slp = (SlpEntry *)calloc(SLP_TABLE, sizeof *g_slp);
    int ok = g_slp && hook_slot_free(0x00729120u, 96) &&
             api->hook_install(api, 0x00729120u, slp_wrap, POP_HOOK_WRAP, NULL, &g_hooks[96]) == POP_OK;
    g_slp_on = 1;
    st_enabled();   /* Test145: the sign trace follows this switch */
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: TEST scenery LOD probe on: %d of 1 hooks; screen %ux%u, world detail %u",
             ok, get_u32(0x00982be4u), get_u32(0x00982be8u), get_u32(0x009017f4u));
    api->log(api, line);
}
static void scenery_lod_probe_exit(const PopModApi *api) {
    if (!g_slp_on) return;
    char line[384];
    snprintf(line, sizeof line,
        "core.nfsmw: TEST scenery: %llu calls; main view drawn %llu precull %llu flags/frustum %llu small %llu "
        "list-full %llu; draw list peak %u of %u, full in %llu of %llu frames, %llu skips; %llu lines",
        (unsigned long long)g_slp_calls, (unsigned long long)g_slp_count[SLP_DRAWN],
        (unsigned long long)g_slp_count[SLP_PRECULL], (unsigned long long)g_slp_count[SLP_FLAGS],
        (unsigned long long)g_slp_count[SLP_SMALL], (unsigned long long)g_slp_count[SLP_FULL], g_slp_peak,
        SLP_LIST_ITEMS, (unsigned long long)g_slp_full_frames, (unsigned long long)g_slp_frames,
        (unsigned long long)g_slp_drops, (unsigned long long)g_slp_lines);
    api->log(api, line);
    int n = snprintf(line, sizeof line, "core.nfsmw: TEST scenery: by view (inserted/skipped/first-full frames):");
    for (int v = 0; v < SLV_N && n > 0 && n < (int)sizeof line; ++v)
        n += snprintf(line + n, sizeof line - (size_t)n, " %s %llu/%llu/%llu", k_slv_name[v], (unsigned long long)g_slv_ins[v],
                      (unsigned long long)g_slv_skip[v], (unsigned long long)g_slv_first_full[v]);
    api->log(api, line);
    free(g_slp);
    g_slp = NULL;
    g_slp_on = 0;
}
#endif
