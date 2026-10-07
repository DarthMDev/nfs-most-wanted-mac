/* Helicopter takedown bounty and announcement (the Extra Options mod's
 * HelicopterBountyFix behaviour, rebuilt natively and narrowed). Included by nfsmw.c.
 *
 * Retail PC 1.3: when a pursuit vehicle is taken out, 00418f30 reads its
 * bounty from the pursuit attributes (00454810 key c07c8040 -> [this+0xf4]), hashes its name
 * (005cc240 at 00418f94 -> [this+0xf8]) and multiplies the bounty by the combo count at 00418fe4. For
 * "copheli" the attribute is <= 10, so a downed helicopter pays almost nothing; and the announcement
 * chain in 00595b00 (name hash -> text) has no "copheli" entry, so no message is shown. This is a
 * community enhancement (the Extra Options mod's HelicopterBountyFix behaviour, reimplemented).
 *
 * ExOpts' caves replace ANY bounty <= 10 with 100,000 and add copheli -> the copsporthench text
 * ("Federal Pursuit Vehicle"). Here only the helicopter is changed:
 *  - bounty: AFTER the name-hash call at 00418f94 (return 00418f99), before the multiply at 00418fe4,
 *    [this+0xf4] := 100,000 when the hash is copheli's and the attribute is <= 10;
 *  - announcement: AFTER the copsporthench hash call at 00595bc2 (return 00595bc7), when the vehicle
 *    is copheli the comparison is made to match, so the copsporthench text is announced.
 * Setting `helicopter_bounty` (default on); off = retail.
 *
 * 005cc240 is the game's string hash and runs constantly (front-end packages, attributes): a hook on
 * it, even one filtered by call site, puts every call through the hook dispatcher (Development 58
 * candidate: ~45 ms main-thread spikes every 1.5-3 s in the front end). So the two call-site hooks
 * exist only while their caller runs: BEFORE 00418f30 (the takedown bounty) / 00595af0 (the
 * announcement chain, and only for a helicopter's name hash) installs the one filtered hook, AFTER removes
 * it. The hash function is otherwise never hooked. */
#ifndef HELI_BOUNTY_H
#define HELI_BOUNTY_H

#define HELI_NAME_COPHELI 0x008915dcu   /* "copheli" in speed.exe .rdata */
#define HELI_BOUNTY 100000u
static uint32_t g_heli_hash;
static int g_heli_enabled;

static uint32_t heli_hash(const PopModApi *api) {
    if (!g_heli_hash) {
        const uint32_t a[] = {HELI_NAME_COPHELI};
        uint32_t h = 0;
        if (api->guest_call(api, 0x005cc240u, 0, a, 1, &h) == POP_OK) g_heli_hash = h;
    }
    return g_heli_hash;
}
static void heli_bounty_after(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (!g_heli_enabled || !cpu->esi) return;
    const uint32_t hash = cpu->eax;
    if (hash != heli_hash(api)) return;
    const int32_t bounty = (int32_t)get_u32(cpu->esi + 0xf4);
    if (bounty > 10) return;
    api->guest_write_u32(api, cpu->esi + 0xf4, HELI_BOUNTY);
    char line[128];
    snprintf(line, sizeof line, "core.nfsmw: helicopter takedown bounty %d -> %u (vehicle record %08x)",
             bounty, HELI_BOUNTY, cpu->esi);
    api->log(api, line);
}
static void heli_announce_after(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (!g_heli_enabled) return;
    if (cpu->esi && cpu->esi == heli_hash(api)) {
        cpu->eax = cpu->esi;   /* `cmp esi, eax` matches: the copsporthench text 4ee07213 is announced */
        api->log(api, "core.nfsmw: helicopter takedown announced");
    }
}

/* Scoped call-site hooks (slots 73/74 are dynamic; 76-79 hold the scope hooks). */
static uint32_t g_heli_scope_calls[2];
static void heli_scope(const PopModApi *api, int which, int enter) {
    uint32_t *slot = &g_hooks[73 + which];
    if (enter) {
        if (!g_heli_enabled || *slot || !api->hook_install_at_callsite) return;
        heli_hash(api);   /* computed here, outside the hashed call */
        if (++g_heli_scope_calls[which] == 1 || g_heli_scope_calls[which] % 1000 == 0) {
            char line[128];
            snprintf(line, sizeof line, "core.nfsmw: helicopter %s scope entered %u times",
                     which ? "announcement" : "bounty", g_heli_scope_calls[which]);
            api->log(api, line);
        }
        if (api->hook_install_at_callsite(api, 0x005cc240u, which ? 0x00595bc7u : 0x00418f99u,
                                          which ? heli_announce_after : heli_bounty_after,
                                          POP_HOOK_AFTER, 0, slot) != POP_OK)
            *slot = 0;
    } else if (*slot) {
        api->hook_remove(api, *slot);
        *slot = 0;
    }
}
static void heli_bounty_enter(const PopModApi *api, pop_cpu_v1 *c, PopHookInvocation *i, void *u) { (void)c; (void)i; (void)u; heli_scope(api, 0, 1); }
static void heli_bounty_leave(const PopModApi *api, pop_cpu_v1 *c, PopHookInvocation *i, void *u) { (void)c; (void)i; (void)u; heli_scope(api, 0, 0); }
/* 00595af0(count, vehicle name hash) runs about every frame; only a helicopter's call gets the scoped hook
 * (its second argument is the name hash that the chain compares). */
static void heli_announce_enter(const PopModApi *api, pop_cpu_v1 *c, PopHookInvocation *i, void *u) {
    (void)i; (void)u;
    if (g_heli_enabled && c && get_u32(c->esp + 8) == heli_hash(api)) heli_scope(api, 1, 1);
}
static void heli_announce_leave(const PopModApi *api, pop_cpu_v1 *c, PopHookInvocation *i, void *u) { (void)c; (void)i; (void)u; heli_scope(api, 1, 0); }

#endif
