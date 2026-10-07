/* eo_extra.h - more Extra Options, rebuilt natively as data writes only (.data/.rdata, verified outside .text), from
 * the addresses of the Extra Options mod (ExOptsTeam/NFSMWExOpts 8a094fe, GPL-3.0, used as a map; no code copied).
 * All off by default (the game as shipped).
 *   unlock_all                0x926124 (byte) = 1: unlocks all cars, parts, events and areas, hidden tracks too
 *                             (Extra Options UnlockAllThings)
 *   remove_neon_barriers      .rdata 0x8B282B/0x8B2838/0x8B2844 = 'J': the player-barrier scenery groups never match
 *                             (Extra Options RemoveNeonBarriers)
 *   remove_old_bridge_barrier .rdata 0x8B2817 = 0x20, 0x8B2810 = 0x54 (Extra Options RemoveOldBridgeBarrier)
 *   force_car_lod / force_tire_lod  0x903384 / 0x903388 (int; -1 = the game decides) (Extra Options ForceCarLOD/TireLOD) */
#ifndef EO_EXTRA_H
#define EO_EXTRA_H

static void eo_extra_init(const PopModApi *api) {
    char line[200];
    const int unlock = setting("unlock_all", 0) != 0, neon = setting("remove_neon_barriers", 0) != 0,
              bridge = setting("remove_old_bridge_barrier", 0) != 0;
    const int car_lod = (int)setting("force_car_lod", -1), tire_lod = (int)setting("force_tire_lod", -1);
    if (unlock) api->guest_write_u8(api, 0x00926124u, 1);
    if (neon) {
        api->guest_write_u8(api, 0x008b282bu, 0x4a);
        api->guest_write_u8(api, 0x008b2838u, 0x4a);
        api->guest_write_u8(api, 0x008b2844u, 0x4a);
    }
    if (bridge) {
        api->guest_write_u8(api, 0x008b2817u, 0x20);
        api->guest_write_u8(api, 0x008b2810u, 0x54);
    }
    if (car_lod >= 0) api->guest_write_u32(api, 0x00903384u, (uint32_t)car_lod);
    if (tire_lod >= 0) api->guest_write_u32(api, 0x00903388u, (uint32_t)tire_lod);
    snprintf(line, sizeof line,
             "core.nfsmw: Extra Options: unlock all %s, neon barriers %s, old bridge barrier %s, car LOD %d, tire LOD %d",
             unlock ? "on" : "off", neon ? "removed" : "kept", bridge ? "removed" : "kept", car_lod, tire_lod);
    api->log(api, line);
}


/* Extra Options keys (all off by default; keys act only in gameplay, game flow 0x925E90 == 6, as in Extra Options):
 *   freeze_camera_key  Pause/Break (F15 on a Mac keyboard) or F9 toggles 0x911020 (Camera_StopUpdating): the camera
 *                      stays where it is while the car drives on (FreezeCamera; F9 added because MacBooks have no Pause)
 *   hot_positions      Left Shift + 1-5 saves a position (0x9B0908 = slot), Left Ctrl + 1-5 jumps back to it
 *                      (0x9B090C = slot): the game's own hot-position feature (EnableSaveLoadHotPos)
 *   light_keys         H toggles the headlights and O the police lights of the player's car (PVehicle IsGlareOn /
 *                      GlareOn 0x6881C0 / 0x669360, ids 7 and 0x7000)
 *   debug_camera       Backspace switches the world camera to the game's free debug camera and back
 *                      (CameraAI_SetAction 0x479EB0 "CDActionDebug" / "CDActionDrive")
 * Keys match by virtual-key code or DirectInput scan code, whichever the host supplies.
 * Guest calls run in a BEFORE hook on the player's per-frame driver 0x006ee490 (slot 105). */
static int g_eo_freeze_key, g_eo_hotpos, g_eo_shift, g_eo_ctrl, g_eo_light_keys, g_eo_debug_cam;
static volatile uint32_t g_eo_pending; /* 1 headlights, 2 police lights, 4 debug camera */
static uint32_t g_eo_key_hook, g_eo_str_debug, g_eo_str_drive;
static int g_eo_cam_debug, g_eo_lights_head, g_eo_lights_cop;
static int eo_is(int32_t dik, int32_t vk, int32_t want_dik, int32_t want_vk) {
    return (vk && vk == want_vk) || (dik && dik == want_dik);
}
static int32_t eo_key(const PopModApi *api, int32_t dik, int32_t vk, int32_t down, void *user) {
    (void)user;
    if (vk == 0x10 || vk == 0xa0 || vk == 0xa1 || dik == 0x2a || dik == 0x36) { g_eo_shift = down != 0; return 0; }
    if (vk == 0x11 || vk == 0xa2 || vk == 0xa3 || dik == 0x1d || dik == 0x9d) { g_eo_ctrl = down != 0; return 0; }
    if (!down || get_u32(0x00925e90u) != 6u) return 0;
    char line[120];
    int digit = 0;
    for (int i = 1; i <= 5; ++i)
        if (eo_is(dik, vk, 0x01 + i, 0x30 + i)) digit = i;
    if (g_eo_freeze_key && (eo_is(dik, vk, 0xc5, 0x13) || eo_is(dik, vk, 0x43, 0x78))) {
        const uint32_t v = get_u32(0x00911020u) & 0xffu;
        api->guest_write_u8(api, 0x00911020u, v ? 0 : 1);
        snprintf(line, sizeof line, "core.nfsmw: Extra Options: camera %s", v ? "unfrozen" : "frozen");
        api->log(api, line);
        return 1;
    }
    if (g_eo_hotpos && digit && (g_eo_shift || g_eo_ctrl)) {
        const uint32_t slot = (uint32_t)digit;
        api->guest_write_u32(api, g_eo_shift ? 0x009b0908u : 0x009b090cu, slot);
        snprintf(line, sizeof line, "core.nfsmw: Extra Options: hot position %u %s", slot, g_eo_shift ? "saved" : "loaded");
        api->log(api, line);
        return 1;
    }
    if (g_eo_light_keys && eo_is(dik, vk, 0x23, 0x48)) { g_eo_pending |= 1u; return 1; }
    if (g_eo_light_keys && eo_is(dik, vk, 0x18, 0x4f)) { g_eo_pending |= 2u; return 1; }
    if (g_eo_debug_cam && eo_is(dik, vk, 0x0e, 0x08)) { g_eo_pending |= 4u; return 1; }
    return 0;
}
static uint32_t eo_player_vehicle(const PopModApi *api) {
    const uint32_t player = get_u32(0x0092d888u);
    uint32_t want = 0;
    if (player && get_u32(player)) api->guest_call(api, get_u32(get_u32(player) + 4u), player, NULL, 0, &want);
    const uint32_t n = get_u32(0x0092cd24u);
    for (uint32_t i = 0; i < n && i < 64; ++i) {
        const uint32_t v = get_u32(0x0092cd28u + 4u * i);
        uint32_t sim = 0;
        if (v && get_u32(v)) api->guest_call(api, get_u32(get_u32(v) + 4u), v, NULL, 0, &sim);
        if (want && sim == want) return v;
    }
    return 0;
}
static void eo_glare(const PopModApi *api, uint32_t veh, uint32_t id, int on) {
    uint32_t is = 0;
    api->guest_call(api, 0x006881c0u, veh, &id, 1, &is);
    if (on && !(is & 0xffu)) api->guest_call(api, 0x00669360u, veh, &id, 1, NULL);
}
static void eo_frame(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    const uint32_t p = g_eo_pending;
    if (!p || get_u32(0x00925e90u) != 6u) return;
    g_eo_pending = 0;
    char line[120];
    if (p & 3u) {
        const uint32_t veh = eo_player_vehicle(api);
        if (veh && (p & 1u)) { g_eo_lights_head = !g_eo_lights_head; eo_glare(api, veh, 7u, g_eo_lights_head); }
        if (veh && (p & 2u)) { g_eo_lights_cop = !g_eo_lights_cop; eo_glare(api, veh, 0x7000u, g_eo_lights_cop); }
        snprintf(line, sizeof line, "core.nfsmw: Extra Options: lights (vehicle %08x) headlights %d police lights %d", veh,
                 g_eo_lights_head, g_eo_lights_cop);
        api->log(api, line);
    }
    if ((p & 4u) && g_eo_str_debug && g_eo_str_drive) {
        g_eo_cam_debug = !g_eo_cam_debug;
        const uint32_t args[2] = {1u, g_eo_cam_debug ? g_eo_str_debug : g_eo_str_drive};
        api->guest_call(api, 0x00479eb0u, 0, args, 2, NULL);
        api->log(api, g_eo_cam_debug ? "core.nfsmw: Extra Options: debug camera on" : "core.nfsmw: Extra Options: debug camera off");
    }
}
static uint32_t eo_guest_string(const PopModApi *api, const char *s) {
    uint32_t at = 0;
    void *p = NULL;
    const uint32_t n = (uint32_t)strlen(s) + 1u;
    if (api->guest_alloc(api, n, &at) != POP_OK || api->guest_ptr(api, at, n, &p) != POP_OK) return 0;
    memcpy(p, s, n);
    return at;
}
static void eo_keys_init(const PopModApi *api) {
    g_eo_freeze_key = setting("freeze_camera_key", 0) != 0;
    g_eo_hotpos = setting("hot_positions", 0) != 0;
    g_eo_light_keys = setting("light_keys", 0) != 0;
    g_eo_debug_cam = setting("debug_camera", 0) != 0;
    int failed = 0;
    if (g_eo_light_keys || g_eo_debug_cam)
        if (install(0x006ee490u, eo_frame, POP_HOOK_BEFORE, 105) != POP_OK) ++failed;
    if ((g_eo_freeze_key || g_eo_hotpos || g_eo_light_keys || g_eo_debug_cam) && api->on_key)
        api->on_key(api, eo_key, NULL, &g_eo_key_hook);
    if (g_eo_debug_cam) { g_eo_str_debug = eo_guest_string(api, "CDActionDebug"); g_eo_str_drive = eo_guest_string(api, "CDActionDrive"); }
    char line[200];
    snprintf(line, sizeof line, "core.nfsmw: Extra Options keys: freeze camera %s, hot positions %s, light keys %s, debug camera %s; hooks failed %d",
             g_eo_freeze_key ? "on" : "off", g_eo_hotpos ? "on" : "off", g_eo_light_keys ? "on" : "off",
             g_eo_debug_cam ? "on" : "off", failed);
    api->log(api, line);
}
/* skip_track_anywhere (Extra Options SkipTrackAnywhere): the skip-track key (T, or L3) also works while driving, not
 * only in the menus. The game's skip-track function 0x517070 plays the next EA Trax track only when game flow
 * 0x925e90 reads 3 (front end); flow reads 3 just for that check: set after 0x5b5b70 / 0x591d90 when they return 0
 * (call sites 0x5170bf / 0x5170ab), restored before the call at 0x5170dc and when 0x517070 returns. Slots 106-109. */
static uint32_t g_eo_flow_saved, g_eo_flow_pending;
static void eo_flow_fake(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (cpu->eax != 0 || g_eo_flow_pending) return;
    const uint32_t f = get_u32(0x00925e90u);
    if (f == 3u) return;
    g_eo_flow_saved = f; g_eo_flow_pending = 1;
    api->guest_write_u32(api, 0x00925e90u, 3u);
    static unsigned fired;
    if (++fired <= 8) {
        char line[120];
        snprintf(line, sizeof line, "core.nfsmw: Extra Options: skip track in game (flow %u read as 3 for the music check)", f);
        api->log(api, line);
    }
}
static void eo_flow_restore(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    if (g_eo_flow_pending) { api->guest_write_u32(api, 0x00925e90u, g_eo_flow_saved); g_eo_flow_pending = 0; }
}
static void eo_skip_track_init(const PopModApi *api) {
    const int skip = setting("skip_track_anywhere", 1) != 0;
    int failed = 0;
    if (skip) {
        if (install_at(0x005b5b70u, 0x005170bfu, eo_flow_fake, POP_HOOK_AFTER, 106) != POP_OK) ++failed;
        if (install_at(0x00591d90u, 0x005170abu, eo_flow_fake, POP_HOOK_AFTER, 107) != POP_OK) ++failed;
        if (install_at(0x005cc240u, 0x005170dcu, eo_flow_restore, POP_HOOK_BEFORE, 108) != POP_OK) ++failed;
        if (install(0x00517070u, eo_flow_restore, POP_HOOK_AFTER, 109) != POP_OK) ++failed;
    }
    char line[140];
    snprintf(line, sizeof line, "core.nfsmw: Extra Options: skip track while driving %s; L3 skips track %s; hooks failed %d",
             skip ? "on" : "off", setting("l3_skip_track", 1) ? "on" : "off", failed);
    api->log(api, line);
}
static void eo_keys_exit(const PopModApi *api) {
    if (g_eo_flow_pending) { api->guest_write_u32(api, 0x00925e90u, g_eo_flow_saved); g_eo_flow_pending = 0; }
    if (g_eo_key_hook) { api->hook_remove(api, g_eo_key_hook); g_eo_key_hook = 0; }
}

#endif
