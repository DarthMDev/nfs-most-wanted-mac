/* New-career cash (Test52 StartingCash, Test70 Underground 2 bonus). Included by native_options.h;
 * needs get_u32, setting and g_exo_starting_cash from the includer. */
#ifndef CAREER_CASH_H
#define CAREER_CASH_H
/* The career record's default (0056d7c0, RET 4) is the retail new-career initializer: it sets flag 2
 * and, when its byte argument says an Underground 2 save was found (00518 5ea passes [ebx+0x1c]), adds
 * the retail $10,000 loyalty bonus (add [ecx+0xc], 0x2710 at 0056d7cc). Two separate controls:
 *   ug2_save_bonus (Test70): treat the Underground 2 save as present, so the ORIGINAL
 *       instruction pays its $10,000 once - an optional enhancement (a Mac has no Underground 2
 *       save to find); never added a second time when the game found a save itself.
 *   starting_cash (Test52, ExOpts StartingCash): the configured amount added after the original.
 * Black Edition (black_edition) is a third, unrelated setting. Every grant is logged as a ledger. */
static void exo_starting_cash(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t record = cpu->ecx;
    const uint32_t before = record ? get_u32(record + 0x0c) : 0;
    uint8_t found = 0;
    api->guest_read_u8(api, cpu->esp + 4, &found);
    const int forced = setting("ug2_save_bonus", 1) && !found;
    if (forced) api->guest_write_u8(api, cpu->esp + 4, 1);
    api->call_original(api, cpu->target, cpu);
    if (!record) return;
    const uint32_t after_original = get_u32(record + 0x0c);
    uint32_t cash = after_original;
    if (g_exo_starting_cash) {
        cash += g_exo_starting_cash;
        api->guest_write_u32(api, record + 0x0c, cash);
    }
    char line[256];
    snprintf(line, sizeof line,
             "core.nfsmw: new career record %08x cash ledger: before %u, Underground 2 bonus %u (%s), "
             "starting cash %u, total %u", record, before, after_original - before,
             found ? "save found by the game" : forced ? "ug2_save_bonus" : "none",
             g_exo_starting_cash, cash);
    api->log(api, line);
}

#endif
