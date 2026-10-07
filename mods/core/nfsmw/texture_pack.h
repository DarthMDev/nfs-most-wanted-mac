/* Native named-texture substitution for a TexWizard-format texture pack the player installed in the game folder
 * (TRACKS\TexWizardX360\textures.bin, as the community Xbox 360 Stuff Pack installs it). No Windows ASI is loaded:
 * the setup (tools/setup_kit/texture_pack_map.py) turns the pack's own name list into texture-pack.map (sorted
 * name-hash pairs); the textures themselves stay the player's files. Every engine call here is verified against the
 * pinned PC 1.3 executable (address leads: the TexWizard project's MW_Address.h). Setting `texture_pack`. */
#define TP_MAP_LIMIT 8192u
static struct { uint32_t from, to; } g_tp_map[TP_MAP_LIMIT];
static uint32_t g_tp_count, g_tp_path;
static int g_tp_requested;
static unsigned g_tp_hits;
static int g_tp_fallback;

static int texture_pack_init(const PopModApi *api) {
    if (!setting("texture_pack", 1)) return 0;
    char path[2048], magic[32];
    snprintf(path, sizeof path, "%s/texture-pack.map", api->mod_dir(api));
    FILE *f = fopen(path, "rb");
    if (!f) {
        api->log(api, "core.nfsmw: texture pack: no texture-pack.map; pack not active");
        return 0;
    }
    if (!fgets(magic, sizeof magic, f) || strcmp(magic, "NFSMW-TEXPACK-MAP-1\n")) {
        fclose(f); return 0;
    }
    unsigned from, to;
    int parsed;
    while ((parsed = fscanf(f, "%x %x", &from, &to)) == 2) {
        if (g_tp_count == TP_MAP_LIMIT || !from || !to ||
            (g_tp_count && from <= g_tp_map[g_tp_count - 1].from)) {
            g_tp_count = 0; fclose(f); return 0;
        }
        g_tp_map[g_tp_count].from = from;
        g_tp_map[g_tp_count++].to = to;
    }
    fclose(f);
    if (parsed != EOF || !g_tp_count) { g_tp_count = 0; return 0; }
    static const char pack[] = "TRACKS\\TexWizardX360\\textures.bin";
    void *guest = NULL;
    if (api->guest_alloc(api, sizeof pack, &g_tp_path) != POP_OK ||
        api->guest_ptr(api, g_tp_path, sizeof pack, &guest) != POP_OK)
        return 0;
    memcpy(guest, pack, sizeof pack);
    snprintf(path, sizeof path, "core.nfsmw: texture pack map ready, %u names", g_tp_count);
    api->log(api, path);
    return 1;
}

/* The pack is loaded at the game's global-resource loading boundary. Resource
 * registration and asynchronous completion stay inside the game engine. */
static void texture_pack_load(const PopModApi *api, pop_cpu_v1 *cpu,
                              PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    api->call_original(api, cpu->target, cpu);
    frontend_pad_load(api);
    if (g_tp_requested || !g_tp_count) return;
    const uint32_t create[] = {g_tp_path, 1, 0, 0, 0};
    const uint32_t begin[] = {0, 0};
    uint32_t resource = 0, unused = 0;
    if (api->guest_call(api, 0x0065fd30u, 0, create, 5, &resource) == POP_OK && resource &&
        api->guest_call(api, 0x006616f0u, resource, begin, 2, &unused) == POP_OK) {
        g_tp_requested = 1;
        api->log(api, "core.nfsmw: texture pack loading requested");
    }
}

/* Resolve only a named replacement present in the engine's texture table.
 * A missing/not-yet-loaded replacement falls back to the original name,
 * never the engine's default texture. No caching of unloadable pointers. */
static void texture_pack_find(const PopModApi *api, pop_cpu_v1 *cpu,
                             PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t hash = get_u32(cpu->esp + 4), first = 0, last = g_tp_count;
    while (first < last) {
        uint32_t mid = first + (last - first) / 2;
        if (g_tp_map[mid].from < hash) first = mid + 1;
        else last = mid;
    }
    if (!g_tp_fallback && g_tp_requested && first < g_tp_count && g_tp_map[first].from == hash) {
        pop_cpu_v1 saved = *cpu;
        uint32_t default_flag = get_u32(cpu->esp + 8);
        api->guest_write_u32(api, saved.esp + 4, g_tp_map[first].to);
        api->guest_write_u32(api, saved.esp + 8, 0);
        api->call_original(api, cpu->target, cpu);
        api->guest_write_u32(api, saved.esp + 4, hash);
        api->guest_write_u32(api, saved.esp + 8, default_flag);
        if (cpu->eax) {
            if (g_tp_hits++ < 48) {
                char line[144];
                snprintf(line, sizeof line, "core.nfsmw: texture pack resolved %08x -> %08x, info=%08x",
                         hash, g_tp_map[first].to, cpu->eax);
                api->log(api, line);
            }
            return;
        }
        /* call_original may run only once per invocation. Its RET is already
         * committed; use a nested guest call for fallback and change only EAX. */
        const uint32_t args[] = {hash, default_flag, get_u32(saved.esp + 12)};
        uint32_t result = 0;
        g_tp_fallback = 1;
        PopModStatus status = api->guest_call(api, saved.target, 0, args, 3, &result);
        g_tp_fallback = 0;
        if (status == POP_OK) cpu->eax = result;
        return;
    }
    api->call_original(api, cpu->target, cpu);
}
