/* Native PlayStation menu prompts. Structural leads: XtendedInput
 * 60958fc1928308023457e3ae94a2f3e79cf3c9b5 (MIT). Verified against this PE:
 * 00516230 hides console objects; 005b85f0 visits objects during update;
 * 00514cc0/00514c70 recursively change group visibility; 005a45b0 renders.
 * No Windows input plugin or replacement input device is used. */
#define FE_PC 0x8u
#define FE_CONSOLE 0x40u
#define FE_SAVED_VISIBLE 0x01000000u /* XtendedInput's UserParam marker */
static atomic_int g_pad_prompts;
static uint32_t g_pad_key_hook, g_pad_pack_path;
static int g_pad_icons_enabled, g_pad_pack_requested;

static uint32_t frontend_hash(const char *s) {
    uint32_t h = 0xffffffffu;
    while (*s) h = h * 33u + (unsigned char)*s++;
    return h;
}
static int frontend_console(void) {
    return g_pad_icons_enabled && atomic_load(&g_pad_prompts);
}
static int frontend_excluded(uint32_t flags) {
    return (flags & (frontend_console() ? FE_PC : FE_CONSOLE)) != 0;
}
static int32_t frontend_key(const PopModApi *api, int32_t dik, int32_t vk,
                            int32_t down, void *user) {
    (void)api; (void)dik; (void)vk; (void)user;
    if (down) atomic_store(&g_pad_prompts, 0);
    return 0;
}
static void frontend_pad_activity(const PopModApi *api) {
    static uint32_t previous_buttons, previous_pov = 0xffffffffu;
    static int32_t previous_x = 32768, previous_y = 32768;
    if (!g_pad_icons_enabled) return;
    uint32_t pad = get_u32(0x0091f150u), state = 0;
    if (!pad) { atomic_store(&g_pad_prompts, 0); return; }
    uint32_t getter = get_u32(get_u32(pad) + 4);
    PopModStatus status = getter ? api->guest_call(api, getter, pad, NULL, 0, &state) : POP_E_INVAL;
    static int reported;
    if (!reported++ && setting("input_trace", 0)) {
        char line[160];
        snprintf(line, sizeof line, "core.nfsmw: prompt input pad=%08x getter=%08x state=%08x status=%d", pad, getter, state, status);
        api->log(api, line);
    }
    if (status != POP_OK || !state) return;
    /* Actual engine joystick-state layout, corroborated by00628940. */
    uint32_t buttons = 0;
    for (unsigned i = 0; i < 13; ++i)
        if (get_u32(state + 12 + i * 4)) buttons |= 1u << i;
    uint32_t pov = get_u32(state + 0x220);
    int32_t x = (int32_t)get_u32(state), y = (int32_t)get_u32(state + 4);
    if ((buttons & ~previous_buttons) || (pov < 36000 && pov != previous_pov) ||
        ((x < 22768 || x > 42768) && (x - previous_x > 2048 || previous_x - x > 2048)) ||
        ((y < 22768 || y > 42768) && (y - previous_y > 2048 || previous_y - y > 2048)))
        {
            if (!atomic_exchange(&g_pad_prompts, 1))
                api->log(api, "core.nfsmw: PlayStation prompts selected by controller input");
        }
    previous_buttons = buttons; previous_pov = pov; previous_x = x; previous_y = y;
}

/* Preserve each child's actual visibility; a category group may itself be
 * hidden while drawing children. Restoring the entire group would reveal
 * contextual icons the game intentionally hid. */
static void frontend_mode_tree(uint32_t object, unsigned depth, int hide, int requested) {
    if (!object || depth > 32) return;
    uint32_t flags = get_u32(object + 0x1c), saved = get_u32(object + 0x28);
    if (hide) {
        if (requested || !(flags & 1u))
            g_api->guest_write_u32(g_api, object + 0x28, saved | FE_SAVED_VISIBLE);
        if (!(flags & 1u))
            g_api->guest_write_u32(g_api, object + 0x1c, flags | 0x2400001u);
    } else {
        if (frontend_excluded(flags)) return;
        if (saved & FE_SAVED_VISIBLE) {
            g_api->guest_write_u32(g_api, object + 0x28, saved & ~FE_SAVED_VISIBLE);
            g_api->guest_write_u32(g_api, object + 0x1c, (flags & ~1u) | 0x2400000u);
        }
    }
    if (get_u32(object + 0x18) == 5) {
        uint32_t count = get_u32(object + 0x60), child = get_u32(object + 0x64);
        if (count > 4096) return;
        for (uint32_t i = 0; i < count && child; ++i) {
            frontend_mode_tree(child, depth + 1, hide, requested);
            child = get_u32(child + 4);
        }
    }
}
static void frontend_show(uint32_t object, unsigned depth) {
    if (!object || depth > 32) return;
    uint32_t flags = get_u32(object + 0x1c), saved = get_u32(object + 0x28);
    if (frontend_excluded(flags)) {
        frontend_mode_tree(object, depth, 1, 1);
        return;
    }
    g_api->guest_write_u32(g_api, object + 0x28, saved & ~FE_SAVED_VISIBLE);
    g_api->guest_write_u32(g_api, object + 0x1c, (flags & ~1u) | 0x2400000u);
    if (get_u32(object + 0x18) == 5) {
        uint32_t count = get_u32(object + 0x60), child = get_u32(object + 0x64);
        if (count > 4096) return;
        for (uint32_t i = 0; i < count && child; ++i) {
            frontend_show(child, depth + 1);
            child = get_u32(child + 4);
        }
    }
}
static void frontend_object(uint32_t object) {
    if (!g_pad_icons_enabled || !object) return;
    uint32_t flags = get_u32(object + 0x1c);
    if (frontend_excluded(flags)) {
        frontend_mode_tree(object, 0, 1, 0);
    } else if (flags & (FE_PC | FE_CONSOLE)) {
        frontend_mode_tree(object, 0, 0, 0);
    }
    if (get_u32(object + 0x18) == 1) {
        static const char *const names[][2] = {
            {"CROSS","PS3_CROSS"}, {"TRIANGLE","PS3_TRIANGLE"},
            {"SQUARE","PS3_SQUARE"}, {"CIRCLE","PS3_CIRCLE"},
            {"SELECT","PS3_R1"}, {"L2","PS3_L1"},
            {"L3","PS3_R3"}, {"R3","PS3_L3"},
            {"START","PS3_START"}, {"L1","PS3_L2"}, {"R1","PS3_R2"},
            {"LEFT_ANALOG","PS3_LANALOG"}, {"RIGHT_ANALOG","PS3_RANALOG"},
            {"DPAD","PS3_DPAD"}, {"DPAD_UPDOWN","PS3_DPAD_Y"},
            {"DPAD_LEFTRIGHT","PS3_DPAD_X"}, {"DPAD_UP","PS3_DPAD_UP"},
            {"DPAD_DOWN","PS3_DPAD_DOWN"}, {"DPAD_LEFT","PS3_DPAD_LEFT"},
            {"DPAD_RIGHT","PS3_DPAD_RIGHT"}
        };
        uint32_t hash = get_u32(object + 0x24);
        for (unsigned i = 0; i < sizeof names / sizeof names[0]; ++i) {
            if (hash == frontend_hash(names[i][0])) {
                g_api->guest_write_u32(g_api, object + 0x24, frontend_hash(names[i][1]));
                g_api->guest_write_u32(g_api, object + 0x1c, get_u32(object + 0x1c) | 0x400000u);
                break;
            }
        }
    }
}
static void frontend_pad_loaded(const PopModApi *api, pop_cpu_v1 *cpu,
                                PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (!g_pad_icons_enabled) { api->call_original(api, cpu->target, cpu); return; }
    frontend_object(get_u32(cpu->esp + 4));
    api->hook_return(api, cpu, 1, 4); /* callback is stdcall; actual RET4 */
}
static void frontend_pad_tick(const PopModApi *api, pop_cpu_v1 *cpu,
                              PopHookInvocation *inv, void *user) {
    (void)api; (void)inv; (void)user;
    frontend_object(get_u32(cpu->esp + 4));
}
static void frontend_pad_visible(const PopModApi *api, pop_cpu_v1 *cpu,
                                 PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    /* EnableMaxPerformanceOnShop: the performance category's max button is
     * made visible from 007b11dd; ExOpts centres it at (-241, 180) first. */
    if (g_exo_max_perf && get_u32(cpu->esp) == 0x007b11e2u && get_u32(cpu->esp + 4)) {
        uint32_t args[3] = {get_u32(cpu->esp + 4), float_bits(-241.0f), float_bits(180.0f)};
        api->log(api, api->guest_call(api, 0x00525050u, 0, args, 3, NULL) == POP_OK
                          ? "core.nfsmw: max performance button shown"
                          : "core.nfsmw: max performance button could not be placed");
    }
    if (!g_pad_icons_enabled) { api->call_original(api, cpu->target, cpu); return; }
    frontend_show(get_u32(cpu->esp + 4), 0);
    api->hook_return(api, cpu, cpu->eax, 0);
}
static void frontend_pad_invisible(const PopModApi *api, pop_cpu_v1 *cpu,
                                   PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t object = get_u32(cpu->esp + 4);
    if (g_pad_icons_enabled && object)
        api->guest_write_u32(api, object + 0x28, get_u32(object + 0x28) & ~FE_SAVED_VISIBLE);
}
static void frontend_pad_render(const PopModApi *api, pop_cpu_v1 *cpu,
                                PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t object = get_u32(cpu->esp + 4);
    if (object && frontend_excluded(get_u32(object + 0x1c))) {
        api->hook_return(api, cpu, cpu->eax, 8); return;
    }
    api->call_original(api, cpu->target, cpu);
}
static void frontend_pad_load(const PopModApi *api) {
    if (!g_pad_pack_path || g_pad_pack_requested) return;
    const uint32_t args[] = {g_pad_pack_path, 1, 0, 0, 0}, begin[] = {0,0};
    uint32_t resource = 0;
    if (api->guest_call(api, 0x0065fd30u, 0, args, 5, &resource) == POP_OK && resource &&
        api->guest_call(api, 0x006616f0u, resource, begin, 2, NULL) == POP_OK) {
        g_pad_pack_requested = 1;
        api->log(api, "core.nfsmw: PlayStation button texture resource requested");
    }
}
static void frontend_pad_init(const PopModApi *api) {
    static const char path[] = "GLOBAL\\XtendedInputButtons.tpk";
    void *data = NULL;
    g_pad_icons_enabled = (int)setting("ps_icons", 1);
    if (!g_pad_icons_enabled) return;
    api->on_key(api, frontend_key, NULL, &g_pad_key_hook);
    if (api->guest_alloc(api, sizeof path, &g_pad_pack_path) == POP_OK &&
        api->guest_ptr(api, g_pad_pack_path, sizeof path, &data) == POP_OK)
        memcpy(data, path, sizeof path);
}
static void frontend_pad_exit(void) {
    /* The mod host owns event subscriptions and removes them on unload. */
    g_pad_icons_enabled = 0;
}
