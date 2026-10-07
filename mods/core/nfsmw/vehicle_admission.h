/* Opt-in observation of retail-PC 0074e160, not a replacement renderer.
 * The two result taps are restricted to calls made by this admission routine.
 * No CPU/guest state is changed. Reasons follow its eight actual false-return
 * branches; successful return does not promise that any mesh was submitted. */
#include <time.h>

typedef struct VAContext {
    struct VAContext *previous;
    int frustum_seen, frustum, inside_seen, inside;
} VAContext;
static VAContext *va_active;
static uint32_t va_hooks[3];
static int va_summary;
static unsigned long long va_alloc_failures;
static unsigned long long va_calls, va_bytes, va_dropped, va_read_errors;
#define VA_LOG_LIMIT (512ull * 1024 * 1024)
static uint32_t va_read(const PopModApi *api, uint32_t addr) {
    uint32_t value = 0;
    if (!addr || api->guest_read_u32(api, addr, &value) != POP_OK) ++va_read_errors;
    return value;
}
static unsigned long long va_wall_us(void) {
    struct timespec t;
    if (timespec_get(&t, TIME_UTC) != TIME_UTC) return 0;
    return (unsigned long long)t.tv_sec * 1000000 + (unsigned long long)t.tv_nsec / 1000;
}
static void va_result(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)api; (void)inv;
    if (!va_active) return;
    if (user) { va_active->inside_seen = 1; va_active->inside = (int)cpu->eax; }
    else { va_active->frustum_seen = 1; va_active->frustum = (int)(cpu->eax & 255); }
}
/* Classification is valid only for the pinned function: all false returns
 * occur before 0074e69c. Do not reuse this classifier for another binary. */
static const char *va_reason(int ok, const VAContext *c, uint32_t failed_delta,
                             int32_t projected, int32_t minimum, uint32_t view_id) {
    if (ok) return "returned_true";
    if (!c->frustum_seen) {
        if (projected < minimum && view_id != 15 && view_id != 16) return "projected_size";
        if (failed_delta) return "initial_scratch_allocation";
        return "unexplained_pre_frustum";
    }
    if (!c->frustum) return "frustum";
    if (c->inside_seen && c->inside) return "main_camera_inside_bounds";
    if (!c->inside_seen) return "both_lods_unavailable";
    return "unexplained_after_frustum";
}
static void va_observe(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    VAContext context = {va_active, 0, 0, 0, 0};
    uint32_t car = cpu->ecx, entry_sp = cpu->esp;
    unsigned long long read_before = va_read_errors, begin = va_wall_us();
    uint32_t caller = va_read(api, entry_sp), view = va_read(api, entry_sp + 4);
    uint32_t vid = va_read(api, view + 4), minimum = va_read(api, view + 0x24);
    uint32_t frame = va_read(api, 0x00982b78), batch = va_read(api, 0x00982cdc);
    uint32_t cursor = va_read(api, 0x00915f84), end = va_read(api, 0x00915f88);
    uint32_t failed = va_read(api, 0x00915f98), draws = va_read(api, 0x00987b00);
    uint32_t lod_min = va_read(api, car + 0x1628), lod_max = va_read(api, car + 0x162c);
    uint32_t scale = va_read(api, view + 0xc), width = va_read(api, 0x00982be4);
    uint32_t height = va_read(api, 0x00982be8), level = va_read(api, 0x00982c00);
    /* Render-connection identity is only labelled on verified caller paths.
     * 00750e20's EDI is repurposed before its call; recover `this` from its
     * saved stack slot only on the verified special call. Raw registers remain
     * available for other callers; `car` is render-info, not a resource ID. */
    uint32_t caller_esi = cpu->esi, caller_edi = cpu->edi;
    uint32_t connection = caller == 0x007511ba ? cpu->edi :
        caller == 0x00751376 ? cpu->esi :
        caller == 0x0075114d ? va_read(api, entry_sp + 0x48) : 0;
    va_active = &context;
    PopModStatus status = api->call_original(api, cpu->target, cpu);
    va_active = context.previous;
    ++va_calls;
    int ok = (cpu->eax & 255) != 0;
    uint32_t after = va_read(api, 0x00982cdc), failed_after = va_read(api, 0x00915f98);
    uint32_t cursor_after = va_read(api, 0x00915f84);
    /* The aligned local frame remains below the restored stack. The projected
     * value is initialized on every path, and overwritten only after the last
     * false-return branch. Never interpret it after a successful return. */
    uint32_t locals = ((entry_sp - 4) & ~15u) - 0x600u;
    int32_t projected = ok ? 0 : (int32_t)va_read(api, locals + 0x10);
    const char *reason = va_reason(ok, &context, failed_after - failed, projected, (int32_t)minimum, vid);
    if (status != POP_OK || va_read_errors != read_before) reason = "observer_incomplete";
    if (!strcmp(reason, "initial_scratch_allocation")) ++va_alloc_failures;
    if (va_summary) return;
    /* Existing run log is the only sink. Hard byte cap is explicit and fatal
     * to completeness, never silent sampling; no new capture history buffer. */
    if (va_bytes >= VA_LOG_LIMIT) {
        if (!va_dropped) api->log(api, "VA_OVERFLOW: guest observer byte cap reached; later absence is UNKNOWN");
        ++va_dropped; return;
    }
    char line[10000];
    int len = snprintf(line, sizeof line,
        "VA {\"seq\":%llu,\"begin_us\":%llu,\"end_us\":%llu,\"frame\":%u,"
        "\"car\":%u,\"render_connection\":%u,\"caller\":%u,\"caller_regs\":[%u,%u],\"view\":%u,\"view_id\":%u,"
        "\"result\":%d,\"reason\":\"%s\",\"projected_valid\":%d,\"projected\":%d,\"minimum\":%d,"
        "\"projection_scale_bits\":%u,\"resolution\":[%u,%u],\"video_level_raw\":%u,\"lod_bounds\":[%d,%d],"
        "\"frustum\":[%d,%d],\"inside\":[%d,%d],\"scratch\":[%u,%u,%u],\"failed_bytes\":[%u,%u],"
        "\"batch\":[%u,%u],\"guest_draw_counter\":[%u,%u],\"read_errors\":%llu,\"meshes\":[",
        va_calls, begin, va_wall_us(), frame, car, connection, caller, caller_esi, caller_edi, view, vid,
        ok, reason, !ok, projected, (int32_t)minimum, scale, width, height, level, (int32_t)lod_min, (int32_t)lod_max,
        context.frustum_seen, context.frustum, context.inside_seen, context.inside, cursor, end, cursor_after,
        failed, failed_after, batch, after, draws, va_read(api, 0x00987b00), va_read_errors - read_before);
    /* Correlate admitted part/solid/VB/IB to existing D3D provenance. Only this
     * invocation's new batch entries are inspected; metadata cap is explicit.
     * Failed invocations correlate by the same render-info identity seen in
     * another view/earlier frame, not by invented persistent resource IDs. */
    unsigned count = 0;
    if (after >= batch && after <= 4096) {
        for (uint32_t i = batch; i < after && count < 64; ++i) {
            uint32_t b = 0x0093e878u + i * 0x44u;
            if (va_read(api, b + 4)) continue; /* indexed mesh path only */
            uint32_t mesh = va_read(api, b), model = va_read(api, b + 8);
            uint32_t solid = model ? va_read(api, model) : 0;
            uint32_t vb = mesh ? va_read(api, mesh + 0x54) : 0;
            uint32_t ib = solid ? va_read(api, solid + 0x28) : 0;
            len += snprintf(line + len, sizeof line - (size_t)len, "%s[%u,%u,%u,%u,%u]",
                            count ? "," : "", mesh, model, solid, vb, ib);
            ++count;
        }
    }
    snprintf(line + len, sizeof line - (size_t)len,
             "],\"mesh_metadata_limited\":%d,\"read_errors_final\":%llu}",
             after < batch || after > 4096 || after - batch > 64, va_read_errors - read_before);
    va_bytes += strlen(line);
    if (api->log(api, line) != POP_OK) ++va_dropped;
}
static PopModStatus va_install(const PopModApi *api) {
    const char *enabled = getenv("NFSMW_VEHICLE_ADMISSION");
    if (!enabled || (strcmp(enabled, "1") && strcmp(enabled, "summary"))) return POP_OK;
    va_summary = !strcmp(enabled, "summary");
    if (!api->hook_install_ex) return POP_E_STATE;
    PopModStatus s = api->hook_install_ex(api, 0x0074e160, 0, va_observe, POP_HOOK_REPLACE,
                                        POP_HOOK_NO_GAME_VIEW, NULL, &va_hooks[0]);
    if (s == POP_OK) s = api->hook_install_ex(api, 0x004fca70, 0x0074e34b, va_result, POP_HOOK_AFTER,
                                             POP_HOOK_NO_GAME_VIEW, NULL, &va_hooks[1]);
    if (s == POP_OK) s = api->hook_install_ex(api, 0x0045fde0, 0x0074e691, va_result, POP_HOOK_AFTER,
                                             POP_HOOK_NO_GAME_VIEW, (void *)1, &va_hooks[2]);
    if (s != POP_OK) {
        for (unsigned i = 0; i < 3; ++i) if (va_hooks[i]) { api->hook_remove(api, va_hooks[i]); va_hooks[i] = 0; }
        return s;
    }
    api->log(api, "VA_START schema=1 entry=0074e160 taps=004fca70@0074e34b,0045fde0@0074e691 limit_bytes=536870912");
    return POP_OK;
}
static void va_stop(const PopModApi *api) {
    if (!va_hooks[0]) return;
    char line[192];
    snprintf(line, sizeof line, "VA_END calls=%llu bytes=%llu dropped=%llu read_errors=%llu frame_alloc_rejections=%llu",
             va_calls, va_bytes, va_dropped, va_read_errors, va_alloc_failures);
    api->log(api, line);
    for (unsigned i = 0; i < 3; ++i) if (va_hooks[i]) { api->hook_remove(api, va_hooks[i]); va_hooks[i] = 0; }
}
