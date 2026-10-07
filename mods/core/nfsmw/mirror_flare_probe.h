/* TEST ONLY (Test131, NFSMW_TEST_MIRROR_FLARES=1): why do the police light-bar flares come and go in the rear-view
 * mirror (an RC4 test session)? Observes the game's car-flare path; writes no game state.
 * Read on exe c0516b48:
 *  - 00742950 (thiscall, RET 0x18; ecx = car render info; args: view, position, matrix, flags, p6, p7) draws one
 *    car's light flares for one view: frame-arena matrix (fails silently when the arena is full: 00915f94/98), LOD
 *    gate ComputeLodIndex(view, pos, car+0x17a8) >= view+0x24 (4 for both views; projected size), car bbox frustum
 *    test against the view's planes ([view]+0x144), then 00505380 per flare.
 *  - 00505380 (cdecl; args: view, flare, matrix, intensity, ...) draws one flare; returns 0 when culled (point
 *    frustum test, direction fade, distance) and non-zero when drawn.
 * Every 60 game frames one line: per view (main 00919650 / mirror 00919730) the car-flare calls, how many of them
 * reached the flare stage (at least one 00505380 call inside), the flare calls and how many drew, and the frame-arena
 * failures in the window. Slots 98 and 99 (Test122 check_hook_slots). */
#ifndef MIRROR_FLARE_PROBE_H
#define MIRROR_FLARE_PROBE_H

#define MFP_FRAME 0x009885b8u
#define MFP_LINE_LIMIT 4000u
static int g_mfp_on, g_mfp_in = -1;
static uint32_t g_mfp_inner, g_mfp_frame0 = 0xffffffffu, g_mfp_lines, g_mfp_fail0;
static uint32_t g_mfp_w[2][4];          /* window: cars, reached, flares, drawn */
static uint64_t g_mfp_t[2][4];          /* totals */
static uint64_t g_mfp_windows, g_mfp_mirror_dark_windows;
/* per car and view: the outcome of the last frames (0 not reached: LOD gate or bbox frustum or arena; 1 reached, no
 * flare drawn; 2 drawn). A flicker is drawn -> not drawn -> drawn within 3 frames of the same car in the same view;
 * counted by the middle outcome (and, for "not reached", whether the probe's own LOD estimate failed). */
#define MFP_CARS 64
typedef struct { uint32_t car, frame; uint8_t hist[3], lodfail; } MfpCar;
static MfpCar g_mfp_cars[2][MFP_CARS];
static uint32_t g_mfp_fw[2][3];          /* window flickers: not reached (LOD), not reached (other), reached-not-drawn */
static uint64_t g_mfp_ft[2][3];
static float mfp_f(uint32_t a) { uint32_t u = get_u32(a); float f; memcpy(&f, &u, 4); return f; }
static int mfp_lod_fails(uint32_t view, uint32_t pos, uint32_t car) {
    const uint32_t cam = get_u32(view + 0x40u);
    if (!cam || !pos || !car) return 0;
    const float dx = mfp_f(pos) - mfp_f(cam + 0x40u), dy = mfp_f(pos + 4u) - mfp_f(cam + 0x44u), dz = mfp_f(pos + 8u) - mfp_f(cam + 0x48u);
    const float r = mfp_f(car + 0x17a8u), k = mfp_f(view + 0xcu);
    const float along = dx * mfp_f(cam + 0x50u) + dy * mfp_f(cam + 0x54u) + dz * mfp_f(cam + 0x58u);
    if (along < -r) return 1;
    const float d = sqrtf(dx * dx + dy * dy + dz * dz) - r;
    const float size = d > r * 0.5f ? k * r / d : k * 2.0f;   /* the near branch is not decoded: assume large */
    return (int)size < (int)get_u32(view + 0x24u);
}
static void mfp_car_outcome(int cls, uint32_t car, uint32_t frame, uint8_t outcome, int lodfail) {
    MfpCar *slot = NULL, *oldest = &g_mfp_cars[cls][0];
    for (int i = 0; i < MFP_CARS; ++i) {
        MfpCar *c = &g_mfp_cars[cls][i];
        if (c->car == car) { slot = c; break; }
        if (c->frame < oldest->frame) oldest = c;
    }
    if (!slot) { slot = oldest; memset(slot, 0, sizeof *slot); slot->car = car; slot->hist[0] = slot->hist[1] = 9; }
    else if (frame == slot->frame) { if (outcome > slot->hist[2]) slot->hist[2] = outcome; return; }
    else if (frame - slot->frame > 1) { slot->hist[0] = slot->hist[1] = 9; slot->hist[2] = 9; }
    slot->hist[0] = slot->hist[1]; slot->hist[1] = slot->hist[2]; slot->hist[2] = outcome;
    if (slot->hist[0] == 2 && slot->hist[1] < 2 && slot->hist[2] == 2) {
        const int k = slot->hist[1] == 1 ? 2 : slot->lodfail ? 0 : 1;
        ++g_mfp_fw[cls][k]; ++g_mfp_ft[cls][k];
    }
    slot->lodfail = (uint8_t)lodfail;
    slot->frame = frame;
}
/* per flare (car, flare descriptor) and view: drawn/culled toggles (police bars strobe by design, so compare the
 * mirror's toggle rate with the main view's for the same kind of traffic) */
#define MFP_FLARES 512
typedef struct { uint32_t car, flare, frame; uint8_t drawn; } MfpFlare;
static MfpFlare g_mfp_fl[2][MFP_FLARES];
static uint32_t g_mfp_tw[2][2];          /* window: flare-frames, toggles */
static uint64_t g_mfp_tt[2][2];
static uint32_t g_mfp_cur_car;
/* per view and flare kind (flare+0x2d bit 0: 1 = omni, e.g. light bars; 0 = directional, e.g. head/tail lights):
 * attempts and drawn, window and total */
static uint32_t g_mfp_kw[2][2][2];
static uint64_t g_mfp_kt[2][2][2];
static void mfp_flare_state(int cls, uint32_t car, uint32_t flare, uint32_t frame, int drawn) {
    uint32_t h = (car * 2654435761u ^ flare * 40503u) % MFP_FLARES;
    MfpFlare *f = NULL;
    for (int i = 0; i < 8; ++i) {
        MfpFlare *c = &g_mfp_fl[cls][(h + i) % MFP_FLARES];
        if ((c->car == car && c->flare == flare) || !c->car || frame - c->frame > 120) { f = c; break; }
    }
    if (!f) return;
    ++g_mfp_tw[cls][0]; ++g_mfp_tt[cls][0];
    if (f->car == car && f->flare == flare && frame - f->frame == 1 && f->drawn != (uint8_t)drawn) { ++g_mfp_tw[cls][1]; ++g_mfp_tt[cls][1]; }
    f->car = car; f->flare = flare; f->frame = frame; f->drawn = (uint8_t)drawn;
}
static int mfp_class(uint32_t view) {
    return view == 0x00919650u ? 0 : view == 0x00919730u ? 1 : -1;
}
static void mfp_tick(const PopModApi *api) {
    const uint32_t frame = get_u32(MFP_FRAME), fails = get_u32(0x00915f98u);
    if (g_mfp_frame0 == 0xffffffffu) { g_mfp_frame0 = frame; g_mfp_fail0 = fails; return; }
    if (frame - g_mfp_frame0 < 60u) return;
    ++g_mfp_windows;
    if (g_mfp_w[1][0] && !g_mfp_w[1][3]) ++g_mfp_mirror_dark_windows;
    if ((g_mfp_w[0][0] || g_mfp_w[1][0]) && g_mfp_lines < MFP_LINE_LIMIT) {
        char line[560];
        snprintf(line, sizeof line,
                 "core.nfsmw: TEST mirror flares: frame %u main cars %u reached %u flares %u drawn %u | mirror cars %u "
                 "reached %u flares %u drawn %u | arena fails +%u | flickers main %u/%u/%u mirror %u/%u/%u (lod/unreached/culled) | flare toggles main %u of %u, mirror %u of %u | directional drawn main %u/%u mirror %u/%u, omni drawn main %u/%u mirror %u/%u",
                 frame, g_mfp_w[0][0], g_mfp_w[0][1], g_mfp_w[0][2], g_mfp_w[0][3], g_mfp_w[1][0], g_mfp_w[1][1],
                 g_mfp_w[1][2], g_mfp_w[1][3], fails - g_mfp_fail0, g_mfp_fw[0][0], g_mfp_fw[0][1], g_mfp_fw[0][2],
                 g_mfp_fw[1][0], g_mfp_fw[1][1], g_mfp_fw[1][2], g_mfp_tw[0][1], g_mfp_tw[0][0], g_mfp_tw[1][1],
                 g_mfp_tw[1][0], g_mfp_kw[0][0][1], g_mfp_kw[0][0][0], g_mfp_kw[1][0][1], g_mfp_kw[1][0][0],
                 g_mfp_kw[0][1][1], g_mfp_kw[0][1][0], g_mfp_kw[1][1][1], g_mfp_kw[1][1][0]);
        api->log(api, line);
        ++g_mfp_lines;
    }
    memset(g_mfp_w, 0, sizeof g_mfp_w);
    memset(g_mfp_fw, 0, sizeof g_mfp_fw);
    memset(g_mfp_tw, 0, sizeof g_mfp_tw);
    memset(g_mfp_kw, 0, sizeof g_mfp_kw);
    g_mfp_frame0 = frame;
    g_mfp_fail0 = fails;
}
static void mfp_car_wrap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t view = get_u32(cpu->esp + 4u), car = cpu->ecx, pos = get_u32(cpu->esp + 8u);
    const int cls = mfp_class(view);
    const int lodfail = cls >= 0 ? mfp_lod_fails(view, pos, car) : 0;
    const uint32_t drawn_before = cls >= 0 ? g_mfp_w[cls][3] : 0;
    const int prev = g_mfp_in;
    const uint32_t prev_inner = g_mfp_inner;
    const uint32_t prev_car = g_mfp_cur_car;
    g_mfp_in = cls;
    g_mfp_inner = 0;
    g_mfp_cur_car = car;
    api->call_next(api, inv, cpu);
    if (cls >= 0) {
        ++g_mfp_w[cls][0]; ++g_mfp_t[cls][0];
        if (g_mfp_inner) { ++g_mfp_w[cls][1]; ++g_mfp_t[cls][1]; }
        mfp_car_outcome(cls, car, get_u32(MFP_FRAME), !g_mfp_inner ? 0 : g_mfp_w[cls][3] > drawn_before ? 2 : 1, lodfail);
    }
    g_mfp_in = prev;
    g_mfp_inner = prev_inner;
    g_mfp_cur_car = prev_car;
    mfp_tick(api);
}
static void mfp_flare_wrap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const int cls = mfp_class(get_u32(cpu->esp + 4u));
    const uint32_t flare = get_u32(cpu->esp + 8u);
    api->call_next(api, inv, cpu);
    if (g_mfp_in < 0 || cls != g_mfp_in) return;
    ++g_mfp_inner;
    ++g_mfp_w[cls][2]; ++g_mfp_t[cls][2];
    if (cpu->eax) { ++g_mfp_w[cls][3]; ++g_mfp_t[cls][3]; }
    {
        const int omni = flare ? (int)(get_u32(flare + 0x2cu) >> 8 & 1u) : 0;
        ++g_mfp_kw[cls][omni][0]; ++g_mfp_kt[cls][omni][0];
        if (cpu->eax) { ++g_mfp_kw[cls][omni][1]; ++g_mfp_kt[cls][omni][1]; }
    }
    mfp_flare_state(cls, g_mfp_cur_car, flare, get_u32(MFP_FRAME), cpu->eax != 0);
}
static void mirror_flare_probe_init(const PopModApi *api) {
    const char *e = getenv("NFSMW_TEST_MIRROR_FLARES");
    if (!e || strcmp(e, "1")) return;
    int ok = hook_slot_free(0x00742950u, 98) &&
             api->hook_install(api, 0x00742950u, mfp_car_wrap, POP_HOOK_WRAP, NULL, &g_hooks[98]) == POP_OK;
    ok += hook_slot_free(0x00505380u, 99) &&
          api->hook_install(api, 0x00505380u, mfp_flare_wrap, POP_HOOK_WRAP, NULL, &g_hooks[99]) == POP_OK;
    g_mfp_on = ok == 2;
    char line[120];
    snprintf(line, sizeof line, "core.nfsmw: TEST mirror flare probe on: %d of 2 hooks", ok);
    api->log(api, line);
}
static void mirror_flare_probe_exit(const PopModApi *api) {
    if (!g_mfp_on) return;
    char line[420];
    snprintf(line, sizeof line,
             "core.nfsmw: TEST mirror flares total: main cars %llu reached %llu flares %llu drawn %llu | mirror cars %llu "
             "reached %llu flares %llu drawn %llu | %llu windows, %llu with mirror cars but no mirror flare drawn | flickers main "
             "%llu/%llu/%llu mirror %llu/%llu/%llu (lod/unreached/culled)",
             (unsigned long long)g_mfp_t[0][0], (unsigned long long)g_mfp_t[0][1], (unsigned long long)g_mfp_t[0][2],
             (unsigned long long)g_mfp_t[0][3], (unsigned long long)g_mfp_t[1][0], (unsigned long long)g_mfp_t[1][1],
             (unsigned long long)g_mfp_t[1][2], (unsigned long long)g_mfp_t[1][3], (unsigned long long)g_mfp_windows,
             (unsigned long long)g_mfp_mirror_dark_windows, (unsigned long long)g_mfp_ft[0][0],
             (unsigned long long)g_mfp_ft[0][1], (unsigned long long)g_mfp_ft[0][2], (unsigned long long)g_mfp_ft[1][0],
             (unsigned long long)g_mfp_ft[1][1], (unsigned long long)g_mfp_ft[1][2]);
    api->log(api, line);
}
#endif
