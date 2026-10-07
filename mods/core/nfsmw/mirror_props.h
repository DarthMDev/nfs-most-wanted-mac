/* Knocked-over props in the rear-view mirror (post-final, Test137; the same restoration HD Reflections' RestoreDetails
 * makes with its destroyable-object pass, external review research 3 Oct).
 * The main view draws the world's movable props (cones, signs, barriers, debris: the list 009b0f90) with
 * 007538d0(view, 0) at 006ded79; the mirror pass in 006de300 never calls it, so props knocked loose vanish from the
 * mirror. 007538d0 culls against the view it is given (its camera frustum and LOD) and submits each prop to the frame
 * render list (00502860 -> 00502520, as cars are), so the call is made for the mirror right after the mirror's own
 * car submission (00750b10 at 006de98b, return 006de990): the mirror pass flushes that list at 006deb01.
 * Switch: mirror_props. */
#define DRAW_WORLD_PROPS 0x007538d0u
static uint32_t g_mprops_calls, g_mprops_fail, g_mprops_busy, g_mprops_max;
static void mirror_props(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    if (!setting("mirror_props", 1) || !api->guest_call) return;
    if (get_u32(MIRROR_VIEW + 4) != 3 || !get_u32(MIRROR_VIEW + 0x40)) return;
    const uint32_t args[2] = { MIRROR_VIEW, 0 };
    /* Evidence counter: each prop that passes the mirror's cull takes frame-arena space (00915f84, in 0073b9c0). */
    const uint32_t arena = get_u32(0x00915f84u);
    if (g_mcost < 0) g_mcost = getenv("NFSMW_TEST_MIRROR_COST") != NULL;
    const uint64_t t0 = g_mcost ? mcost_now() : 0;
    if (api->guest_call(api, DRAW_WORLD_PROPS, 0, args, 2, NULL) != POP_OK) ++g_mprops_fail;
    if (g_mcost) g_mcost_props_ns += mcost_now() - t0;
    const uint32_t used = get_u32(0x00915f84u) - arena;
    if (used && used < 0x100000u) { ++g_mprops_busy; if (used > g_mprops_max) g_mprops_max = used; }
    if (++g_mprops_calls == 1) api->log(api, "core.nfsmw: props in the rear-view mirror active");
    if (g_mcost > 0 && g_mprops_calls % 600 == 0) {   /* TEST ONLY (Test141) */
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: TEST mirror cost: last 600 frames: car shadows %.1f us/frame, props %.1f us/frame",
                 g_mcost_shadow_ns / 600.0 / 1000.0, g_mcost_props_ns / 600.0 / 1000.0);
        api->log(api, line);
        g_mcost_shadow_ns = g_mcost_props_ns = 0;
    }
    if (g_mprops_calls % 3600 == 0) {
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: props in the mirror: %u frames, %u failed, %u with props drawn (max %u arena bytes)",
                 g_mprops_calls, g_mprops_fail, g_mprops_busy, g_mprops_max);
        api->log(api, line);
    }
}
static void mirror_props_exit(const PopModApi *api) {
    if (!g_mprops_calls) return;
    char line[200];
    snprintf(line, sizeof line, "core.nfsmw: props in the mirror: %u frames, %u failed, %u with props drawn (max %u arena bytes) (exit)",
             g_mprops_calls, g_mprops_fail, g_mprops_busy, g_mprops_max);
    api->log(api, line);
}
