/* TEST ONLY (Test115, NFSMW_TEST_STREAM_TIMING=<csv path>): wrap hooks that time the PC v1.3 streaming functions the
 * public reverse engineering names (MWSDK retail-v1.3 database; external review research 2 Oct 2026), so a main-thread stall
 * can be split into "slept" (RECOMP_SLEEP_TRACE, kit) and "worked inside the streamer". Every call is counted (count,
 * total, maximum per function, logged at exit); a call of 2 ms or more is written to the CSV with its start on the
 * same clock as frames.csv render_monotonic_ns and RECOMP_SLEEP_TRACE (CLOCK_UPTIME_RAW), its caller (the return
 * address) and its duration. Not installed otherwise; no game state is read or written. */
#ifndef STREAM_TIMING_H
#define STREAM_TIMING_H
#include <time.h>

typedef struct { uint32_t addr; const char *name; uint64_t n, total_ns, max_ns; } StreamFn;
static StreamFn g_stream_fns[] = {
    {0x00503380u, "Stream_BlockUntilLoaded", 0, 0, 0},
    {0x005010a0u, "StreamMgr_FindResidentSection", 0, 0, 0},
    {0x006626b0u, "RunDeferredCallbacks", 0, 0, 0},
    {0x00666aa0u, "RegionLoader_LoadHandler", 0, 0, 0},
    {0x00665390u, "ProcessStreamingFileLoad", 0, 0, 0},
    {0x00664cc0u, "DispatchAssetLoadCompletion", 0, 0, 0},
    {0x0064cf00u, "bThreadYield", 0, 0, 0},
};
#define STREAM_FN_COUNT (sizeof g_stream_fns / sizeof g_stream_fns[0])
static FILE *g_stream_csv;
static int g_stream_on;

static uint64_t stream_now_ns(void) {
#ifdef __APPLE__
    return clock_gettime_nsec_np(CLOCK_UPTIME_RAW);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
#endif
}
static void stream_wrap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    StreamFn *f = (StreamFn *)user;
    uint32_t caller = 0;
    api->guest_read_u32(api, cpu->esp, &caller);
    const uint64_t t0 = stream_now_ns();
    api->call_next(api, inv, cpu);
    const uint64_t d = stream_now_ns() - t0;
    ++f->n;
    f->total_ns += d;
    if (d > f->max_ns) f->max_ns = d;
    if (d >= 2000000ull && g_stream_csv)
        fprintf(g_stream_csv, "%llu,%s,%08x,%.3f\n", (unsigned long long)t0, f->name, caller, (double)d / 1e6);
}
/* Slots 73, 74, 82..86 (free in g_hooks). */
static const int k_stream_slots[7] = {73, 74, 82, 83, 84, 85, 86};
static void stream_timing_init(const PopModApi *api) {
    const char *path = getenv("NFSMW_TEST_STREAM_TIMING");
    if (!path || !*path) return;
    g_stream_csv = fopen(path, "w");
    if (!g_stream_csv) { api->log(api, "core.nfsmw: TEST stream timing: cannot open the CSV"); return; }
    setvbuf(g_stream_csv, NULL, _IOLBF, 0);
    fprintf(g_stream_csv, "start_monotonic_ns,function,caller,ms\n");
    unsigned ok = 0;
    for (unsigned i = 0; i < STREAM_FN_COUNT; ++i) {
        const int slot = k_stream_slots[i];
        if (!hook_slot_free(g_stream_fns[i].addr, slot) ||
            api->hook_install(api, g_stream_fns[i].addr, stream_wrap, POP_HOOK_WRAP, &g_stream_fns[i], &g_hooks[slot]) != POP_OK) {
            char line[128];
            snprintf(line, sizeof line, "core.nfsmw: TEST stream timing: hook %08x (%s) failed", g_stream_fns[i].addr, g_stream_fns[i].name);
            api->log(api, line);
            continue;
        }
        ++ok;
    }
    g_stream_on = 1;
    char line[96];
    snprintf(line, sizeof line, "core.nfsmw: TEST stream timing on: %u of %u functions wrapped", ok, (unsigned)STREAM_FN_COUNT);
    api->log(api, line);
}
static void stream_timing_exit(const PopModApi *api) {
    if (!g_stream_on) return;
    for (unsigned i = 0; i < STREAM_FN_COUNT; ++i) {
        const StreamFn *f = &g_stream_fns[i];
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: TEST stream timing: %s %llu calls, total %.1f ms, max %.2f ms", f->name,
                 (unsigned long long)f->n, (double)f->total_ns / 1e6, (double)f->max_ns / 1e6);
        api->log(api, line);
    }
    if (g_stream_csv) { fclose(g_stream_csv); g_stream_csv = NULL; }
    g_stream_on = 0;
}
#endif
