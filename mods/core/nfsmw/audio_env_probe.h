/* TEST ONLY (Test118, NFSMW_TEST_AUDIO_ENV=1): is the PC game's own tunnel reverb running in the port? Two hooks on
 * the audio code the strings name (speed.exe 1.3, read 2 Oct 2026):
 *  - 004c5c00, SFXCTL_Tunnel's update (fastcall, this in ecx): it asks the track's audio-zone lookup (004b5620) for
 *    zone types 3, 4, 5, 7 and 10 in turn, keeps the type found at +0x38 and "in a tunnel" at +0x28, and on a change
 *    posts the "TunnelUpdate" message (1 entering, 0 leaving) and starts a reverb transition;
 *  - 004b57e0, the reverb transition (thiscall: preset index, time): it sets the FX object's target preset +0xa0.
 * Every change of (+0x28, +0x38) and every transition is logged with the game frame (009885b8). Wrap/before hooks
 * only; no game state is written. Slots 88 and 89 (free in g_hooks; Test122 check_hook_slots). */
#ifndef AUDIO_ENV_PROBE_H
#define AUDIO_ENV_PROBE_H

static int g_aenv_on, g_aenv_last_in = -1, g_aenv_last_zone = -1;
static uint64_t g_aenv_updates, g_aenv_changes, g_aenv_transitions;

static void aenv_tunnel_wrap(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t self = cpu->ecx;
    api->call_next(api, inv, cpu);
    ++g_aenv_updates;
    if (!self) return;
    const int in = (int)(get_u32(self + 0x28u) & 0xffu), zone = (int)get_u32(self + 0x38u);
    if (in == g_aenv_last_in && zone == g_aenv_last_zone) return;
    ++g_aenv_changes;
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: TEST audio environment: frame %u, tunnel %d -> %d, zone type %d -> %d (preset slot %u)",
             get_u32(0x009885b8u), g_aenv_last_in, in, g_aenv_last_zone, zone, get_u32(self + 0x74u));
    api->log(api, line);
    g_aenv_last_in = in;
    g_aenv_last_zone = zone;
}
static void aenv_transition(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t preset = 0, time = 0;
    api->guest_read_u32(api, cpu->esp + 4u, &preset);
    api->guest_read_u32(api, cpu->esp + 8u, &time);
    ++g_aenv_transitions;
    char line[128];
    snprintf(line, sizeof line, "core.nfsmw: TEST audio environment: frame %u, reverb transition to preset %d over %d (FX object %08x)",
             get_u32(0x009885b8u), (int)preset, (int)time, cpu->ecx);
    api->log(api, line);
}
static void audio_env_probe_init(const PopModApi *api) {
    const char *on = getenv("NFSMW_TEST_AUDIO_ENV");
    if (!on || !*on || *on == '0') return;
    int ok = 0;
    if (hook_slot_free(0x004c5c00u, 88) &&
        api->hook_install(api, 0x004c5c00u, aenv_tunnel_wrap, POP_HOOK_WRAP, NULL, &g_hooks[88]) == POP_OK) ++ok;
    if (hook_slot_free(0x004b57e0u, 89) &&
        api->hook_install(api, 0x004b57e0u, aenv_transition, POP_HOOK_BEFORE, NULL, &g_hooks[89]) == POP_OK) ++ok;
    g_aenv_on = 1;
    char line[96];
    snprintf(line, sizeof line, "core.nfsmw: TEST audio environment probe on: %d of 2 hooks", ok);
    api->log(api, line);
}
static void audio_env_probe_exit(const PopModApi *api) {
    if (!g_aenv_on) return;
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: TEST audio environment: %llu tunnel updates, %llu changes, %llu reverb transitions",
             (unsigned long long)g_aenv_updates, (unsigned long long)g_aenv_changes, (unsigned long long)g_aenv_transitions);
    api->log(api, line);
    g_aenv_on = 0;
}
#endif
