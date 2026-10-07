// Test70: new-career cash ledger (career_cash.h) and the helicopter takedown fix (heli_bounty.h)
// against a fixture guest. Pure: no game.
#include "../../kit/mods/pop_mod_api.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <string>

static std::map<uint32_t, uint8_t> bytes;
static std::map<std::string, int64_t> settings;
static uint32_t get_u32(uint32_t a) { uint32_t v = 0; for (int i = 3; i >= 0; --i) v = v << 8 | bytes[a + (uint32_t)i]; return v; }
static void put_u32(uint32_t a, uint32_t v) { for (int i = 0; i < 4; ++i) bytes[a + (uint32_t)i] = (uint8_t)(v >> (8 * i)); }
static int64_t setting(const char *k, int64_t fb) { auto it = settings.find(k); return it == settings.end() ? fb : it->second; }
static uint32_t g_exo_starting_cash;
static uint32_t g_hooks[80];
#include "../../mods/core/nfsmw/career_cash.h"
#include "../../mods/core/nfsmw/heli_bounty.h"

static unsigned checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; printf("FAIL %d: %s\n", __LINE__, #x); } } while (0)
static std::string last_log;
static const uint32_t HELI = 0x1234abcdu, SUV = 0x0badf00du, REC = 0x100000u, ESP = 0x200000u;
static int hash_calls;

int main() {
    PopModApi api{};
    api.size = sizeof api;
    api.log = [](const PopModApi *, const char *m) { last_log = m; return POP_OK; };
    api.guest_read_u8 = [](const PopModApi *, uint32_t a, uint8_t *v) { *v = bytes[a]; return POP_OK; };
    api.guest_write_u8 = [](const PopModApi *, uint32_t a, uint8_t v) { bytes[a] = v; return POP_OK; };
    api.guest_write_u32 = [](const PopModApi *, uint32_t a, uint32_t v) { put_u32(a, v); return POP_OK; };
    api.guest_call = [](const PopModApi *, uint32_t addr, uint32_t, const uint32_t *args, uint32_t n, uint32_t *out) {
        if (addr != 0x005cc240u || n != 1 || args[0] != HELI_NAME_COPHELI) return POP_E_RANGE;
        ++hash_calls; *out = HELI; return POP_OK; };
    // The original 0056d7c0: flag 2, + 10,000 when its byte argument is set.
    api.call_original = [](const PopModApi *, uint32_t, pop_cpu_v1 *c) {
        put_u32(c->ecx + 4, get_u32(c->ecx + 4) | 2);
        if (bytes[c->esp + 4]) put_u32(c->ecx + 0x0c, get_u32(c->ecx + 0x0c) + 10000);
        return POP_OK; };
    auto career = [&](uint8_t ug2_found) {
        put_u32(REC + 0x0c, 0); bytes[ESP + 4] = ug2_found;
        pop_cpu_v1 c{}; c.ecx = REC; c.esp = ESP; c.target = 0x0056d7c0u;
        exo_starting_cash(&api, &c, nullptr, nullptr);
        return get_u32(REC + 0x0c);
    };
    // 1. Cash ledger: each control separately, never twice.
    settings.clear(); g_exo_starting_cash = 0;
    CHECK(career(0) == 10000 && last_log.find("Underground 2 bonus 10000 (ug2_save_bonus)") != std::string::npos);
    CHECK(career(1) == 10000 && last_log.find("save found by the game") != std::string::npos);  // not 20,000
    settings["ug2_save_bonus"] = 0;
    CHECK(career(0) == 0);                                   // retail without a UG2 save
    CHECK(career(1) == 10000);                               // retail with one
    g_exo_starting_cash = 10000000;
    CHECK(career(0) == 10000000);                            // StartingCash alone
    settings.erase("ug2_save_bonus");
    CHECK(career(0) == 10010000);                            // both, each once
    CHECK(last_log.find("starting cash 10000000, total 10010000") != std::string::npos);
    CHECK((get_u32(REC + 4) & 2) != 0);                      // the original still ran
    // 2. Helicopter bounty: only copheli, only <= 10, idempotent.
    auto bounty = [&](uint32_t hash, int32_t value) {
        put_u32(REC + 0xf4, (uint32_t)value);
        pop_cpu_v1 c{}; c.eax = hash; c.esi = REC;
        heli_bounty_after(&api, &c, nullptr, nullptr);
        return (int32_t)get_u32(REC + 0xf4);
    };
    g_heli_enabled = 1;
    CHECK(bounty(HELI, 0) == 100000);
    CHECK(bounty(HELI, 10) == 100000);
    CHECK(bounty(HELI, 11) == 11);
    CHECK(bounty(HELI, 100000) == 100000);                   // a repeat leaves it
    CHECK(bounty(SUV, 0) == 0);                              // non-helicopter low bounty untouched (not ExOpts' broad rule)
    CHECK(bounty(SUV, 5000) == 5000);
    g_heli_enabled = 0;
    CHECK(bounty(HELI, 0) == 0);                             // off = retail
    CHECK(hash_calls == 1);                                  // hash looked up once
    // 3. Announcement: helicopter matches the copsporthench comparison; others untouched.
    auto announce = [&](uint32_t vehicle) {
        pop_cpu_v1 c{}; c.esi = vehicle; c.eax = 0xfeedbeefu;
        heli_announce_after(&api, &c, nullptr, nullptr);
        return c.eax;
    };
    g_heli_enabled = 1;
    CHECK(announce(HELI) == HELI);
    CHECK(announce(SUV) == 0xfeedbeefu);
    g_heli_enabled = 0;
    CHECK(announce(HELI) == 0xfeedbeefu);
    // 4. Scoped call-site hooks: the hash function is hooked only while the caller runs.
    static std::map<uint32_t, std::pair<uint32_t, uint32_t>> live;   // id -> (addr, return pc)
    static uint32_t next_id = 1, removed = 0;
    api.hook_install_at_callsite = [](const PopModApi *, uint32_t addr, uint32_t ret, PopHookFn, int32_t, void *, uint32_t *out) {
        live[next_id] = {addr, ret}; *out = next_id++; return POP_OK; };
    api.hook_remove = [](const PopModApi *, uint32_t id) { removed += live.erase(id) ? 1u : 0u; return POP_OK; };
    g_heli_enabled = 1;
    heli_bounty_enter(&api, nullptr, nullptr, nullptr);
    CHECK(live.size() == 1 && live.begin()->second.first == 0x005cc240u && live.begin()->second.second == 0x00418f99u);
    heli_bounty_enter(&api, nullptr, nullptr, nullptr);       // nested: still one
    CHECK(live.size() == 1);
    pop_cpu_v1 ac{}; ac.esp = ESP; put_u32(ESP + 8, SUV);
    heli_announce_enter(&api, &ac, nullptr, nullptr);         // another vehicle: no hook
    CHECK(live.size() == 1);
    put_u32(ESP + 8, HELI);
    heli_announce_enter(&api, &ac, nullptr, nullptr);         // the helicopter: hooked
    CHECK(live.size() == 2);
    heli_bounty_leave(&api, nullptr, nullptr, nullptr);
    heli_announce_leave(&api, nullptr, nullptr, nullptr);
    CHECK(live.empty() && removed == 2 && !g_hooks[73] && !g_hooks[74]);
    heli_bounty_leave(&api, nullptr, nullptr, nullptr);       // leave without enter: nothing
    CHECK(removed == 2);
    g_heli_enabled = 0;
    heli_bounty_enter(&api, nullptr, nullptr, nullptr);       // off: never hooked
    CHECK(live.empty());
    printf("career fixes: %u checks, %u failures\n", checks, failures);
    return failures ? 1 : 0;
}
