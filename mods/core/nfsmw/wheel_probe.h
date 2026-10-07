/* TEST ONLY (Test70, NFSMW_TEST_WHEEL_PROBE=1): the per-car, per-view visibility test 007422d0 that
 * ExOpts' DisappearingWheelsFix patches (0074251d: the "outside every plane" result 0 becomes 1). Each
 * call fills 11 entries [this + i*0x20 + 0x18] (one per view table entry 0090504c + i*0x50; [entry-4]
 * = mode 0/1, [entry] = plane count). Every 600 calls the probe logs, per entry, how many calls left
 * it culled and the entry's mode/plane count, so runs before/after focus loss can be compared. */
#ifndef WHEEL_PROBE_H
#define WHEEL_PROBE_H
static int g_wheel_probe;
static uint32_t g_wheel_calls, g_wheel_culled[11];
static void wheel_probe(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    const uint32_t self = cpu->ecx;
    api->call_original(api, cpu->target, cpu);
    if (!self) return;
    for (unsigned i = 0; i < 11; ++i) {
        uint8_t v = 1;
        api->guest_read_u8(api, self + i * 0x20u + 0x18u, &v);
        if (!v) ++g_wheel_culled[i];
    }
    if (++g_wheel_calls % 600 == 0) {
        char line[512]; int n = snprintf(line, sizeof line, "core.nfsmw: TEST wheel probe %u calls; culled per view:", g_wheel_calls);
        for (unsigned i = 0; i < 11 && n < (int)sizeof line - 40; ++i) {
            const uint32_t e = 0x0090504cu + i * 0x50u;
            n += snprintf(line + n, sizeof line - n, " %u:%u(m%u,p%u)", i, g_wheel_culled[i], get_u32(e - 4u), get_u32(e));
            g_wheel_culled[i] = 0;
        }
        api->log(api, line);
    }
}
#endif
