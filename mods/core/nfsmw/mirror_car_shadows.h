/* Car shadows in the rear-view mirror (post-final enhancement, Test134).
 *
 * The PC car renderer 0074e160 draws a car's ground shadow (00743ea0: four ground-height probes, then a dark
 * quad submitted to the frame's render list with 006cf930) only when
 *     shadows are on (00903328) && this is not the road-reflection pass (arg 9 == 0) && view id == 1 (main view).
 * The mirror view (00919730) has id 3, so cars in the mirror have no shadow and look as if they float.
 * This hook sits on the call just before that test (0074e7fa -> 007422d0, return 0074e7ff). For the mirror view
 * only, it makes the same 00743ea0 call the main view makes, with the same arguments, read from 0074e160's frame:
 *     this = esi, args = view, esp+0xc0 (car position), [ebp+0x2c], [esp+0x54], esp+0x490, [esp+0x38]
 * (esp = the frame's esp at 0074e7ff; 007422d0 is thiscall and removes its 2 arguments).
 * The mirror pass flushes the render list after its cars (006de300 at 006deb01), so the shadow lands in the
 * mirror picture. Switch: mirror_car_shadows. */
#define MIRROR_SHADOW_FN    0x00743ea0u
#define CAR_SHADOWS_ON      0x00903328u
static uint32_t g_mshadow_calls, g_mshadow_fail, g_mshadow_own;
/* TEST ONLY (Test141, NFSMW_TEST_MIRROR_COST=1): time spent in the extra mirror calls (car shadows + props). */
#include <time.h>
static int g_mcost = -1;
static uint64_t g_mcost_shadow_ns, g_mcost_props_ns;
static uint64_t mcost_now(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec; }
static void mirror_car_shadow(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (!setting("mirror_car_shadows", 1) || !api->guest_call) return;
    const uint32_t ebp = cpu->ebp, esp = cpu->esp;
    const uint32_t view = get_u32(ebp + 8);
    if (view != MIRROR_VIEW || get_u32(view + 4) != 3) return;
    if (!get_u32(CAR_SHADOWS_ON) || get_u32(ebp + 0x28)) return;
    /* Guard: never the car the mirror looks out of (its body is not drawn in the mirror, so its shadow alone would
     * be a dark patch). The mirror camera ([view+0x40], position at +0x40) sits in that car; another car's centre is
     * more than 2 m away. In Test134 this skipped 4 calls in a 6-minute drive: the own car normally never gets here.
     * (The car-shaped dark shape at the bottom of the mirror is the own car's sun shadow from the mirror world-shadow
     * pass, mirror_shadows; it is there with this switch off too.) */
    const uint32_t cam = get_u32(view + 0x40);
    if (!cam) return;
    const float dx = get_float(esp + 0xc0u) - get_float(cam + 0x40u);
    const float dy = get_float(esp + 0xc4u) - get_float(cam + 0x44u);
    if (dx * dx + dy * dy < 4.0f) { ++g_mshadow_own; return; }
    static int alt = -1; /* TEST ONLY (Test134): shadows on odd frames only, for an A/B in adjacent frame dumps */
    if (alt < 0) alt = getenv("NFSMW_TEST_MIRROR_SHADOW_ALT") != NULL;
    if (alt && !(get_u32(0x00982b78u) & 1u)) return;
    const uint32_t args[6] = { view, esp + 0xc0u, get_u32(ebp + 0x2c), get_u32(esp + 0x54),
                               esp + 0x490u, get_u32(esp + 0x38) };
    if (g_mcost < 0) g_mcost = getenv("NFSMW_TEST_MIRROR_COST") != NULL;
    const uint64_t t0 = g_mcost ? mcost_now() : 0;
    if (api->guest_call(api, MIRROR_SHADOW_FN, cpu->esi, args, 6, NULL) != POP_OK) ++g_mshadow_fail;
    if (g_mcost) g_mcost_shadow_ns += mcost_now() - t0;
    if (++g_mshadow_calls == 1 || g_mshadow_calls % 36000 == 0) {
        char line[160];
        snprintf(line, sizeof line, "core.nfsmw: mirror car shadows: %u drawn, %u failed", g_mshadow_calls, g_mshadow_fail);
        api->log(api, line);
    }
}
static void mirror_car_shadows_exit(const PopModApi *api) {
    if (!g_mshadow_calls) return;
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: mirror car shadows: %u drawn, %u failed, %u own-car skipped (exit)",
             g_mshadow_calls, g_mshadow_fail, g_mshadow_own);
    api->log(api, line);
}
