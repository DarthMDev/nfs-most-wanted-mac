/* The pinned PC SND engine has a discrete six-channel output mode (3).
 * 0082007e builds PCM EXTENSIBLE with mask 0x3f; 008233bc interleaves
 * FL, FR, C, LFE, BL, BR. Private run70 measured distinct race samples on
 * every plane. This selects that original mixer, not a stereo upmixer or
 * Dolby encoder. The native host must preserve all six channels.
 *
 * Test53: SND always runs that discrete mode. The Audio Mode choice - Mono,
 * Stereo or 5.1 PCM - is how the host PRESENTS that mix (PopModApi
 * audio_output: ITU fold for Mono/Stereo, discrete for 5.1), so every change
 * is heard immediately, including while cycling in the menu, without
 * restarting SND or resizing its ring. Movies opened afterwards follow the
 * same choice. Test48's load-time path (SND mode setter 0081b4ac, restart
 * 0081b841 via the load transition 004d163a) still runs, but the pin below
 * keeps its request at mode 3, so the ring never changes shape. */
static int g_audio_engine = -1;       /* SND output actually configured: 1 = 5.1 PCM */
static uint32_t g_audio_engine_mode = 0xffffffffu; /* last SND mode seen at configure */

static void native_audio_mode(const PopModApi *api, pop_cpu_v1 *cpu,
                              PopHookInvocation *inv, void *user) {
    (void)inv;
    (void)user;
    /* Every request, including the load transition's 0 (Mono) or 8 (Stereo
     * preset, unsupported by this PE's table 009c1afc), keeps the discrete mix. */
    api->guest_write_u32(api, cpu->esp + 4, 3);
}

static void native_audio_configured(const PopModApi *api, pop_cpu_v1 *cpu,
                                    PopHookInvocation *inv, void *user) {
    (void)inv;
    (void)user;
    const uint32_t config = get_u32(cpu->esp + 4), mode = get_u32(cpu->esp + 8); /* cdecl */
    const int surround = mode == 3;
    /* 0081ef6f sets speaker type +0x3f for every mode, but only mode 3 also
     * sets +0x40 = 1 and nothing clears it; restart copies the running config.
     * The PC game never left mode 3 at runtime. Leaving it must match a clean
     * launch in the target mode (measured: 5.1 -> Mono otherwise built a
     * 2-channel ring instead of mono). Scoped to that transition only. */
    if (!surround && g_audio_engine_mode == 3 && config && (get_u32(config + 0x40) & 0xffu)) {
        api->guest_write_u8(api, config + 0x40, 0);
        api->log(api, "core.nfsmw: cleared SND 5.1 flag carried over from mode 3");
    }
    if (mode != g_audio_engine_mode) {
        char line[160];
        snprintf(line, sizeof line, "core.nfsmw: SND output configured mode=%u (5.1 PCM %s)%s", mode,
                 surround ? "on" : "off", g_audio_engine < 0 ? "" : " - applied in process");
        api->log(api, line);
    }
    g_audio_engine = surround;
    g_audio_engine_mode = mode;
}

static void native_audio_init(const PopModApi *api, pop_cpu_v1 *cpu,
                              PopHookInvocation *inv, void *user) {
    (void)cpu;
    (void)inv;
    (void)user;
    const uint32_t mode = 3;
    uint32_t result = 0;
    PopModStatus status = api->guest_call(api, 0x0081b4acu, 0, &mode, 1, &result);
    char line[160];
    snprintf(line, sizeof line,
             "core.nfsmw: discrete 5.1 SND mix requested before init (call=%d result=%d); Audio Mode selects the presentation",
             status, (int32_t)result);
    api->log(api, line);
}

/* Retail PC UIOptionsScreen / UIAudioModeOption, verified from the pinned PE.
 * The profile's AudioSettings.mode remains 0/1. PCM is a native draft until
 * EXITCOMPLETE, so the stock discard dialog and defaults still work. Never
 * restart SND or resize its live ring from an option widget. */
static uint32_t g_audio_screen, g_audio_text;
static int g_audio_active, g_audio_initial, g_audio_draft, g_audio_edited;

static uint32_t native_audio_options(void) {
    uint32_t database = get_u32(0x0091cf90u);
    uint32_t options = database ? get_u32(database + 0x10) : 0;
    return options && get_u32(options + 0x24) == 0 ? options : 0;
}

/* The presentation for the current choice: the draft 5.1 flag, else the
 * game's own AudioSettings.mode (0 Mono, 1 Stereo) - both live while the
 * Audio screen cycles, and restored by its Discard. Called every frame and
 * immediately after each change; the host call happens only on a change. */
static int g_audio_draft_known;
static void native_audio_present(const PopModApi *api) {
    static uint32_t sent_mode = 0xffffffffu, sent_flags = 0xffffffffu;
    if (!g_audio_draft_known) {
        g_audio_draft = setting("surround_audio", 0) != 0;
        g_audio_draft_known = 1;
    }
    const uint32_t options = native_audio_options();
    const uint32_t mode = g_audio_draft ? POP_AUDIO_OUTPUT_SURROUND51
                          : !options || get_u32(options + 0x70) ? POP_AUDIO_OUTPUT_STEREO
                                                                : POP_AUDIO_OUTPUT_MONO;
    /* Dolby Digital carries speaker roles in the bitstream, so the endpoint
     * exchange only applies to PCM. */
    const int dolby = setting("audio_dolby_digital", 0) != 0;
    const uint32_t flags = dolby ? POP_AUDIO_DOLBY_DIGITAL
                           : setting("audio_center_lfe_swap", 0) ? POP_AUDIO_SWAP_CENTER_LFE
                                                                 : 0;
    if ((mode == sent_mode && flags == sent_flags) || !api->audio_output)
        return;
    PopModStatus status = api->audio_output(api, mode, flags);
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: audio presentation %s%s (status=%d)",
             mode == POP_AUDIO_OUTPUT_SURROUND51 ? (dolby ? "5.1" : "5.1 PCM")
             : mode == POP_AUDIO_OUTPUT_STEREO  ? "Stereo"
                                                : "Mono",
             dolby ? ", Dolby Digital output" : flags ? ", centre/LFE exchanged" : "", status);
    api->log(api, line);
    sent_mode = mode;
    sent_flags = flags;
}

static void native_audio_string(const PopModApi *api, uint32_t object, const char *text) {
    void *buffer = NULL;
    if (!object || !g_audio_text || strlen(text) >= 128 ||
        api->guest_ptr(api, g_audio_text, 128, &buffer) != POP_OK)
        return;
    memcpy(buffer, text, strlen(text) + 1);
    /* FEPrintf skips a newly created FEString with no custom-text buffer.
     * Use its own string setter so the first draw after reopening works too. */
    if (api->guest_call(api, 0x0057e8c0u, object + 0x64, &g_audio_text, 1, NULL) == POP_OK)
        api->guest_write_u32(api, object + 0x1c, get_u32(object + 0x1c) | 0x400002u);
}

static void native_audio_setup(const PopModApi *api, pop_cpu_v1 *cpu,
                               PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    g_audio_screen = cpu->ecx;
    g_audio_initial = g_audio_draft = setting("surround_audio", 0) != 0;
    g_audio_edited = 0;
    api->call_original(api, cpu->target, cpu);
    /* PC SetupAudio hides this widget outside frontend flow 3. Append the
     * same game-owned widget in Pause; its list/destructor retain ownership.
     * Sequence and sizes are the actual 0052955b..00529593 instructions. */
    if (get_u32(0x00925e90u) != 3) {
        uint32_t widget = 0;
        const uint32_t size = 0x5c, enabled = 1;
        if (api->guest_call(api, 0x00652ad0u, 0, &size, 1, &widget) == POP_OK && widget) {
            if (api->guest_call(api, 0x00589300u, widget, &enabled, 1, NULL) == POP_OK) {
                api->guest_write_u32(api, widget, 0x0089bac8u);
                const uint32_t add[] = {widget, enabled};
                api->guest_call(api, 0x00588570u, g_audio_screen, add, 2, NULL);
            } else {
                api->guest_call(api, 0x00652b00u, 0, &widget, 1, NULL);
            }
        }
    }
}

static void native_audio_draw(const PopModApi *api, pop_cpu_v1 *cpu,
                              PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t widget = cpu->ecx, options = native_audio_options();
    api->call_original(api, cpu->target, cpu);
    if (!g_audio_screen || !options) return;
    if (g_audio_draft)
        native_audio_string(api, get_u32(widget + 0x30),
                            setting("audio_dolby_digital", 0) ? "Dolby 5.1" : "5.1 PCM");
    /* Every choice is presented immediately (native_audio_present); there
     * is no pending state to announce. */
}

static void native_audio_act(const PopModApi *api, pop_cpu_v1 *cpu,
                             PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t options = native_audio_options(), widget = cpu->ecx;
    uint32_t action = get_u32(cpu->esp + 8);
    if (!g_audio_screen || !options ||
        (action != 0x9120409eu && action != 0xb5971bf1u)) {
        api->call_original(api, cpu->target, cpu);
        return;
    }
    int mode = g_audio_draft ? 2 : (get_u32(options + 0x70) != 0);
    mode = (mode + (action == 0xb5971bf1u ? 1 : 2)) % 3;
    g_audio_draft = mode == 2;
    g_audio_edited = 1;
    api->guest_write_u32(api, options + 0x70, mode ? 1 : 0);
    api->guest_write_u8(api, widget + 0x2a, 1);
    uint32_t vtable = get_u32(widget);
    api->guest_call(api, get_u32(vtable + 0x38), widget, &action, 1, NULL);
    api->guest_call(api, 0x0051b2d0u, widget, NULL, 0, NULL);
    native_audio_present(api); /* heard now, while cycling */
    api->hook_return(api, cpu, cpu->eax, 8); /* actual thiscall RET 8 */
}

static void native_audio_unchanged(const PopModApi *api, pop_cpu_v1 *cpu,
                                   PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    int audio = cpu->ecx == g_audio_screen && native_audio_options();
    api->call_original(api, cpu->target, cpu);
    if (audio && g_audio_draft != g_audio_initial)
        cpu->eax &= ~0xffu; /* original result is bool AL, not the entire EAX */
}

static void native_audio_restore(const PopModApi *api, pop_cpu_v1 *cpu,
                                 PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (cpu->ecx == g_audio_screen && native_audio_options()) {
        g_audio_draft = g_audio_initial;
        g_audio_edited = 0;
    }
    api->call_original(api, cpu->target, cpu);
}

static void native_audio_defaults(const PopModApi *api, pop_cpu_v1 *cpu,
                                  PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (cpu->ecx == g_audio_screen && native_audio_options()) {
        g_audio_draft = 0;
        g_audio_edited = 1;
    }
    api->call_original(api, cpu->target, cpu);
}

static void native_audio_notification(const PopModApi *api, pop_cpu_v1 *cpu,
                                      PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    /* 00545b26 is the actual PC EXITCOMPLETE branch. Persist only after its
     * dirty-state calculation. A prior RestoreOriginals clears edited, so
     * leaving through Discard cannot change the native preference. */
    int commit = cpu->ecx == g_audio_screen && native_audio_options() &&
                 get_u32(cpu->esp + 4) == 0xe1fde1d1u && g_audio_edited;
    int requested = g_audio_draft;
    api->call_original(api, cpu->target, cpu);
    if (commit) {
        PopModStatus status = api->settings_set(api, "surround_audio", requested);
        char line[192];
        snprintf(line, sizeof line,
                 "core.nfsmw: Audio menu saved PCM5.1=%d (status=%d); SND mix mode=%u; "
                 "the presentation already follows the choice",
                 requested, status, g_audio_engine_mode);
        api->log(api, line);
        g_audio_edited = 0;
    }
}

static void native_audio_destroy(const PopModApi *api, pop_cpu_v1 *cpu,
                                 PopHookInvocation *inv, void *user) {
    (void)api; (void)inv; (void)user;
    if (cpu->ecx == g_audio_screen) g_audio_screen = 0;
}

static void native_audio_exit(const PopModApi *api) {
    if (g_audio_text) api->guest_free(api, g_audio_text);
    g_audio_text = g_audio_screen = 0;
}
