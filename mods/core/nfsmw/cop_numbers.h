/* Police numbers at heat 6-10 (Test161). Included by nfsmw.c.
 *
 * Heat 6-10 exist only through Heat Level Override, and their retail "cops" tables are leftovers: 10, 6, 8, 4 and 1
 * cars (heat 10 sends one car). With heat_level_override on, those tables are scaled to 8 cars, the game's own limit
 * and what heat 5 reaches, keeping their car types and proportions. Heat 1-5 and everything else are retail: the
 * limit of 8 police cars at once, waves, backup, roadblocks, joining patrols, the helicopter.
 *
 * Retail PC 1.3 (speed.exe 80774c2e): 00424110 (called only from CopRequest at 0042bb92 and from 0042b820 at
 * 0042b8f2; this = the AIPursuit) takes the player's pursuit attributes from 00419910 (its call at 00424121), sums
 * the counts of the "cops" table (the array at the layout's start: u16 count at +2, 0x18-byte entries from +8,
 * vehicle key at +8, count at +0x10; the helicopter, key [0090d8c8], is not counted), limits the sum to the wave's
 * remaining room and shares it over the car types. Here the counts are scaled after 00419910 returns and put back
 * when 00424110 returns, so the attribute data is never left changed. [this+0xe0] is the pursuit's heat level.
 * Test161 also tried per-heat sliders and more than 8 cars; see verification/test161-police-numbers. */
#ifndef COP_NUMBERS_H
#define COP_NUMBERS_H

#define COP_MANAGER 0x0090d5f4u
#define COP_HELI_KEY 0x0090d8c8u
#define COP_MAX 8
#define COP_PATCH_MAX 16

static uint32_t g_cop_this;   /* the AIPursuit of the 00424110 call in progress, else 0 */
static struct { uint32_t addr, value; } g_cop_patch[COP_PATCH_MAX];
static int g_cop_patches;
static uint32_t g_cop_logged; /* heat, table and target of the last log line */
static int g_cop_test_log, g_cop_test_retail;   /* TEST ONLY: NFSMW_TEST_COP_LOG, NFSMW_TEST_COP_RETAIL (A/B) */

static int cop_numbers_on(void) {
    return !g_cop_test_retail && setting("heat_level_override", 0);
}
static int cop_target(int heat) {   /* 0: the retail table */
    return heat >= 6 && heat <= 10 ? COP_MAX : 0;
}

/* 00419910 returned the pursuit attributes to 00424110 (call site 00424121). */
static void cop_table_read(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    api->call_original(api, cpu->target, cpu);
    const uint32_t self = g_cop_this, inst = cpu->eax;
    if (!self || !inst || g_cop_patches) return;
    const int on = cop_numbers_on();
    if (!on && !g_cop_test_log) return;
    const uint32_t layout = get_u32(inst + 8u);
    if (!layout) return;
    const unsigned n = (get_u32(layout) >> 16) & 0xffffu;
    if (n == 0 || n > COP_PATCH_MAX) return;
    const uint32_t heli = get_u32(COP_HELI_KEY);
    int32_t count[COP_PATCH_MAX];
    int64_t sum = 0;
    for (unsigned i = 0; i < n; ++i) {
        const uint32_t e = layout + 8u + 0x18u * i;
        count[i] = (get_u32(e + 8u) == heli) ? 0 : (int32_t)get_u32(e + 0x10u);
        if (count[i] < 0) count[i] = 0;
        sum += count[i];
    }
    if (sum <= 0 || sum > 1000) return;
    const int heat = (int)get_u32(self + 0xe0u), target = on ? cop_target(heat) : 0;
    if (target && target != sum) {
        /* largest-remainder split of the 8 cars over the car types, in the table's proportions */
        int32_t share[COP_PATCH_MAX];
        int64_t rem[COP_PATCH_MAX], given = 0;
        for (unsigned i = 0; i < n; ++i) {
            share[i] = (int32_t)(count[i] * (int64_t)target / sum);
            rem[i] = count[i] ? (count[i] * (int64_t)target) % sum : -1;
            given += share[i];
        }
        while (given < target) {   /* at most n - 1 rounds: the remainders add up to less than n */
            unsigned best = 0;
            for (unsigned i = 1; i < n; ++i)
                if (rem[i] > rem[best]) best = i;
            if (rem[best] < 0) break;
            ++share[best];
            rem[best] = -1;
            ++given;
        }
        for (unsigned i = 0; i < n; ++i) {
            if (!count[i] || share[i] == count[i]) continue;
            const uint32_t a = layout + 8u + 0x18u * i + 0x10u;
            g_cop_patch[g_cop_patches].addr = a;
            g_cop_patch[g_cop_patches].value = get_u32(a);
            ++g_cop_patches;
            api->guest_write_u32(api, a, (uint32_t)share[i]);
        }
    }
    if (g_cop_test_log) {   /* TEST ONLY: chasers by type (P+0x8c..+0x90, key/count pairs) and roadblock cars */
        static uint32_t last;
        const uint32_t p = self + 0x48u, b = get_u32(p + 0x8cu), e = get_u32(p + 0x90u), rb = get_u32(p + 0x84u);
        uint32_t chasers = 0, blockers = 0;
        if (b && e >= b && e - b <= 8u * 32u)
            for (uint32_t a = b; a < e; a += 8u) chasers += get_u32(a + 4u);
        if (rb) {
            const uint32_t f = get_u32(rb + 0xcu), l = get_u32(rb + 0x10u);
            if (f && l >= f && l - f <= 4u * 32u) blockers = (l - f) / 4u;
        }
        if (((chasers << 16) | blockers) != last) {
            last = (chasers << 16) | blockers;
            char line[96];
            snprintf(line, sizeof line, "core.nfsmw: TEST chasers %u, roadblock cars %u", chasers, blockers);
            api->log(api, line);
        }
    }
    const uint32_t state = ((uint32_t)heat << 24) | ((uint32_t)sum << 12) | (uint32_t)target;
    if (target && state != g_cop_logged) {
        g_cop_logged = state;
        char line[128];
        snprintf(line, sizeof line, "core.nfsmw: police numbers heat %d: table %d -> %d cars", heat, (int)sum, target);
        api->log(api, line);
    }
}

/* 00424110 (this = AIPursuit): the table is scaled only during the call. */
static void cop_table_call(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    const uint32_t outer = g_cop_this;
    g_cop_this = outer ? 0 : cpu->ecx;   /* not re-entrant: a nested call keeps the retail table */
    api->call_original(api, cpu->target, cpu);
    if (!outer) {
        while (g_cop_patches > 0) {
            --g_cop_patches;
            api->guest_write_u32(api, g_cop_patch[g_cop_patches].addr, g_cop_patch[g_cop_patches].value);
        }
        g_cop_this = 0;
    } else {
        g_cop_this = outer;
    }
}

/* Every frame, TEST ONLY (NFSMW_TEST_COP_LOG): the cop manager's police-car count when it changes. */
static void cop_numbers_poll(const PopModApi *api) {
    if (!g_cop_test_log) return;
    const uint32_t mgr = get_u32(COP_MANAGER);
    if (!mgr) return;
    static uint32_t last = 0xffffffffu, frames;
    const uint32_t loaded = get_u32(mgr + 0x94u);
    if (++frames >= 60 && loaded != last) {
        frames = 0;
        last = loaded;
        char line[96];
        snprintf(line, sizeof line, "core.nfsmw: TEST police cars %u (limit %u)", loaded, get_u32(mgr + 0x98u));
        api->log(api, line);
    }
}

static void cop_numbers_install(void) {
    g_cop_test_log = getenv("NFSMW_TEST_COP_LOG") != NULL;
    g_cop_test_retail = getenv("NFSMW_TEST_COP_RETAIL") != NULL;
    /* One plain REPLACE per address (a second one, even filtered by call site, is refused): 00424110 has only the
     * two pursuit callers, so its hook is unfiltered; 00419910 has three, so the one inside 00424110 is selected. */
    if (install(0x00424110u, cop_table_call, POP_HOOK_REPLACE, 110) != POP_OK ||
        install_at(0x00419910u, 0x00424126u, cop_table_read, POP_HOOK_REPLACE, 112) != POP_OK)
        g_api->log(g_api, "core.nfsmw: heat 6-10 police numbers unavailable (hooks 00424110 / 00419910)");
}
#endif
