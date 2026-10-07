/* Editable per-profile overrides. Only declared native settings and the
 * explicitly reconstructed ExOpts subset are accepted; ASI files and code
 * addresses from user text are never executed. F10 remains the settings UI. */
static char *options_trim(char *s) {
    while (isspace((unsigned char)*s)) ++s;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) --end;
    *end = 0;
    return s;
}

static void native_options_load(const PopModApi *api) {
    const char *profile = getenv("RECOMP_PROFILE_DIR");
    if (!profile || !*profile) return;
    char path[4096];
    if (snprintf(path, sizeof path, "%s/NativeOptions.ini", profile) >= (int)sizeof path) return;
    FILE *file = fopen(path, "rb");
    if (!file) return;
    char line[1024], section[96] = "";
    unsigned number = 0, accepted = 0, rejected = 0;
    /* Precedence: the LAST accepted line for a setting wins, whether it uses
     * the native key or a PC alias; an override of an earlier line is logged. */
    struct { char key[48]; unsigned line; } seen[64];
    unsigned seen_count = 0;
    while (fgets(line, sizeof line, file)) {
        ++number;
        if (!strchr(line, '\n') && !feof(file)) {
            int c; while ((c = fgetc(file)) != EOF && c != '\n') {}
            ++rejected; continue;
        }
        char *s = options_trim(line), *comment = strchr(s, ';');
        if (comment) *comment = 0;
        comment = strstr(s, "//"); if (comment) *comment = 0;
        s = options_trim(s);
        if (!*s || *s == '#') continue;
        if (*s == '[') {
            char *end = strchr(s, ']');
            if (end && !*options_trim(end + 1) && end - s < (int)sizeof section) {
                *end = 0; snprintf(section, sizeof section, "%s", options_trim(s + 1));
            } else { section[0] = 0; ++rejected; }
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq) { ++rejected; continue; }
        *eq = 0;
        const char *key = options_trim(s), *text = options_trim(eq + 1), *native_key = NULL;
        double scale = 1;
        static const struct { const char *section, *key, *native; double scale; } aliases[] = {
            {"Weather", "GeneralRainAmount", "rain_amount", 100},
            {"Weather", "RoadReflectionAmount", "rain_reflection", 100},
            {"Weather", "RainSize", "rain_size", 10000},
            {"Weather", "RainIntensity", "rain_intensity", 100},
            {"Weather", "RainCrossing", "rain_crossing", 10000},
            {"Weather", "RainSpeed", "rain_speed", 10000},
            {"Weather", "RainGravity", "rain_gravity", 100},
            {"Misc", "SkipMovies", "skip_movies", 1},
            {"Misc", "SkipNISs", "skip_nis", 1},
            {"Menu", "AllowLongerProfileNames", "long_profile_names", 1},
            {"Menu", "ShowSpecialVinyls", "special_vinyls", 1},
            {"Menu", "ReplayBlacklistRaces", "replay_blacklist", 1},
            {"Gameplay", "EnableMaxPerformanceOnShop", "max_perf_shop", 1},
            {"Gameplay", "SelectableMarkerCount", "marker_count", 1},
            {"Gameplay", "StartingCash", "starting_cash", 1},
            {"Gameplay", "ForceBlackEdition", "black_edition", 1},
            {"Gameplay", "SkipTrackAnywhere", "skip_track_anywhere", 1},
            {"Pursuit", "HeatLevelOverride", "heat_level_override", 1},
            {"Pursuit", "MinimumHeatLevel", "heat_level_min", 1},
            {"Pursuit", "MaximumHeatLevel", "heat_level_max", 1}
        };
        if (!strcmp(section, "Native")) native_key = key;
        else for (unsigned i = 0; i < sizeof aliases / sizeof aliases[0]; ++i)
            if (!strcmp(section, aliases[i].section) && !strcmp(key, aliases[i].key)) {
                native_key = aliases[i].native; scale = aliases[i].scale; break;
            }
        errno = 0; char *end;
        double value = strtod(text, &end) * scale;
        int64_t current;
        if (!native_key || end == text || *options_trim(end) || errno || !isfinite(value) ||
            value < -2147483647.0 || value > 2147483647.0 || fabs(value - round(value)) > 0.000001 ||
            api->settings_get(api, native_key, &current) != POP_OK ||
            api->settings_set(api, native_key, (int64_t)llround(value)) != POP_OK) {
            char warning[128];
            snprintf(warning, sizeof warning, "core.nfsmw: NativeOptions.ini line %u ignored (unsupported or invalid)", number);
            api->log(api, warning); ++rejected;
        } else {
            ++accepted;
            unsigned k = 0;
            while (k < seen_count && strcmp(seen[k].key, native_key)) ++k;
            if (k < seen_count) {
                char note[160];
                snprintf(note, sizeof note, "core.nfsmw: NativeOptions.ini line %u overrides line %u (%s); the last line wins",
                         number, seen[k].line, native_key);
                api->log(api, note);
                seen[k].line = number;
            } else if (seen_count < sizeof seen / sizeof seen[0]) {
                snprintf(seen[seen_count].key, sizeof seen[seen_count].key, "%s", native_key);
                seen[seen_count++].line = number;
            }
        }
    }
    fclose(file);
    char result[160];
    snprintf(result, sizeof result, "core.nfsmw: NativeOptions.ini applied %u values, ignored %u; active overrides take effect at launch", accepted, rejected);
    api->log(api, result);
}

/* Owner-selected ExOpts 8a094fe options (Test52). The patched instructions
 * in game.toml read the slots below; their seeds are the original constants,
 * so an option that is off leaves the game unmodified. Changes apply at the
 * next launch. Hooks for the options that need them are in nfsmw.c. */
#define EXO_PROFILE_LEN 0x00a37c00u   /* keyboard limit, 7 */
#define EXO_PROFILE_KBD 0x00a37c04u   /* keyboard mode, 6 */
#define EXO_LIST_LEN 0x00a37c08u      /* load-list name length, 8 */
#define EXO_LIST_IGNORE 0x00a37c0cu   /* character that hides a save, ' ' */
#define EXO_PERF_FLAG 0x00a37c10u     /* mask for FEDatabase [+0x12c] bit 0 */
#define EXO_PERF_BYTE 0x00a37c14u     /* mask for the byte at 009b9e8d */
#define EXO_MARKERS 0x00a37c18u       /* selectable markers, 2 */
#define EXO_VINYL_MAX 0x00a37c1cu     /* last vinyl category, 0x409 */
#define EXO_VINYL_BASE 0x00a37c20u    /* vinyl switch base, 0x404 */
#define EXO_VINYL_HASH 0x00a37c24u    /* last case: category name hash */
#define EXO_VINYL_NAME 0x00a37c28u    /* last case: EBP (group label hash) */
#define EXO_VINYL_GROUP 0x00a37c2cu   /* last case: vinyl group, 7 */
#define EXO_RIVAL_BEATEN 0x00a37c30u  /* mask: defeated rival blocks a race */
#define EXO_BOSS_STATS 0x00a37c34u    /* mask: boss-race stats reset */
#define EXO_RIVAL_FLAG 0x00a37c38u    /* value of the defeated flag, 1 */
#define EXO_BLACK_EDITION 0x00a37c40u /* ORed into the content check, 0 */
#define EXO_CURRENT_RIVAL 0x0091ca3cu

static void exo_replay_slots(const PopModApi *api, int on) {
    api->guest_write_u8(api, EXO_RIVAL_BEATEN, on ? 0u : 0xffu);
    api->guest_write_u32(api, EXO_BOSS_STATS, on ? 0u : 0xffffffffu);
    api->guest_write_u8(api, EXO_RIVAL_FLAG, on ? 0u : 1u);
}

static void extra_options_owner(const PopModApi *api) {
    /* Longer profile names (Test52 safety decision, differs from ExOpts):
     * only the keyboard's LENGTH limit follows the option (7 -> 15; the
     * memcard entry's name field at +8 is 32 bytes, the keyboard's buffers
     * 256, clamp 155). The keyboard keeps the game's own character mode 6
     * (ExOpts' mode 0 admits every character, including path separators),
     * and the load list keeps the game's own rule of hiding names with a
     * space. The list admits names up to 15 characters whether or not the
     * option is on, so a career saved with a longer name never disappears
     * when the option is turned off; only new names are limited to 7. */
    int longer = setting("long_profile_names", 1) != 0;
    api->guest_write_u32(api, EXO_PROFILE_LEN, longer ? 15u : 7u);
    api->guest_write_u32(api, EXO_PROFILE_KBD, 6u);
    api->guest_write_u32(api, EXO_LIST_LEN, 16u);
    api->guest_write_u32(api, EXO_LIST_IGNORE, (uint32_t)' ');
    g_exo_max_perf = setting("max_perf_shop", 1) != 0;
    api->guest_write_u8(api, EXO_PERF_FLAG, g_exo_max_perf ? 0u : 1u);
    api->guest_write_u8(api, EXO_PERF_BYTE, g_exo_max_perf ? 0u : 0xffu);
    int64_t markers = setting("marker_count", 6);
    if (markers < 1) markers = 1;
    if (markers > 6) markers = 6;
    api->guest_write_u32(api, EXO_MARKERS, (uint32_t)markers);
    g_exo_special_vinyls = setting("special_vinyls", 1) != 0;
    api->guest_write_u32(api, EXO_VINYL_MAX, g_exo_special_vinyls ? 0x40au : 0x409u);
    g_exo_replay_blacklist = setting("replay_blacklist", 1) != 0;
    exo_replay_slots(api, g_exo_replay_blacklist);
    int black = setting("black_edition", 1) != 0;
    api->guest_write_u32(api, EXO_BLACK_EDITION, black ? 1u : 0u);
    int64_t cash = setting("starting_cash", 0); /* dollars, as the PC INI; off by default (a new career = the game's $30,000 + the $10,000 UG2 bonus) */
    if (cash < 0) cash = 0;
    if (cash > 100000000) cash = 100000000;
    g_exo_starting_cash = (uint32_t)cash;
    char line[256];
    snprintf(line, sizeof line,
             "core.nfsmw: Extra Options: longer profile names %s, max performance on shop %s, "
             "markers %d, special vinyls %s, replay blacklist %s, Black Edition %s, starting cash %u",
             longer ? "on" : "off", g_exo_max_perf ? "on" : "off", (int)markers,
             g_exo_special_vinyls ? "on" : "off", g_exo_replay_blacklist ? "on" : "off",
             black ? "on" : "off", (unsigned)g_exo_starting_cash);
    api->log(api, line);
}

/* CustomizeParts::Setup (007bd060): for the special category, the switch's
 * last case (contest, group 7) is pointed at the special group (8) for the
 * duration of the call. The values are ExOpts' VinylMenuCodeCave2. */
static void exo_vinyl_parts(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    int special = g_exo_special_vinyls && get_u32(cpu->ecx + 0x148) == 0x40au;
    if (special) {
        api->guest_write_u32(api, EXO_VINYL_BASE, 0x405u);
        api->guest_write_u32(api, EXO_VINYL_HASH, 0xb715070au);
        api->guest_write_u32(api, EXO_VINYL_NAME, 0x55778e5au);
        api->guest_write_u32(api, EXO_VINYL_GROUP, 8u);
    }
    api->call_original(api, cpu->target, cpu);
    if (special) {
        api->log(api, "core.nfsmw: special vinyl parts listed (group 8)");
        api->guest_write_u32(api, EXO_VINYL_BASE, 0x404u);
        api->guest_write_u32(api, EXO_VINYL_HASH, 0xcd057d21u);
        api->guest_write_u32(api, EXO_VINYL_NAME, 0x1ba508fcu);
        api->guest_write_u32(api, EXO_VINYL_GROUP, 7u);
    }
}

/* CustomizeSub::SetupVinylGroups adds the contest category last (call at
 * 007bc8fa); the special category follows it, as ExOpts' VinylMenuCodeCave. */
static void exo_vinyl_groups(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t self = cpu->ecx;
    api->call_original(api, cpu->target, cpu);
    if (!g_exo_special_vinyls)
        return;
    uint32_t args[4] = {get_u32(0x00905eb0u), 0x55778e5au, 0xb715070au, 0x40au};
    api->log(api, api->guest_call(api, 0x007bb560u, self, args, 4, NULL) == POP_OK
                      ? "core.nfsmw: special vinyl category added"
                      : "core.nfsmw: special vinyl category could not be added");
}

#include "career_cash.h"

/* 00624360 stores the career's current rival into 0091ca3c before its
 * dialog. ExOpts keeps the chosen rival instead when the dialog's argument
 * is positive; these restore it just after the store (before the 00573020
 * call), which is the same state at every later read. */
static void exo_rival_screen(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)api; (void)cpu; (void)inv; (void)user;
    g_exo_rival = get_u32(EXO_CURRENT_RIVAL);
}
static void exo_rival_keep(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (g_exo_replay_blacklist && (int32_t)get_u32(cpu->esi + 8) >= 1) {
        char line[128];
        snprintf(line, sizeof line, "core.nfsmw: rival dialog keeps selected rival %u (career rival %u)",
                 g_exo_rival, get_u32(EXO_CURRENT_RIVAL));
        api->guest_write_u32(api, EXO_CURRENT_RIVAL, g_exo_rival);
        api->log(api, line);
    }
}
/* 00600766 looks up the career's rival for the name and picture; ExOpts
 * shows the selected rival (0091ca3c) instead. */
static void exo_rival_shown(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (!g_exo_replay_blacklist) { api->call_original(api, cpu->target, cpu); return; }
    static uint32_t logged = 0xffffffffu;
    if (logged != get_u32(EXO_CURRENT_RIVAL)) {
        char line[96];
        logged = get_u32(EXO_CURRENT_RIVAL);
        snprintf(line, sizeof line, "core.nfsmw: rival name/picture from selected rival %u", logged);
        api->log(api, line);
    }
    api->hook_return(api, cpu, get_u32(EXO_CURRENT_RIVAL), 0);
}

/* ExOpts 8a094fe, matched to our PE's seven original float values and live
 * readers. Data writes are meaningful in a static recompilation; patching
 * instructions in guest memory would not be. Changes apply next launch. */
static void extra_options_apply(const PopModApi *api) {
    static const struct { uint32_t address; const char *key; int def; float scale; } rain[] = {
        {0x00904b38u, "rain_reflection", 100, 100},
        {0x00904a90u, "rain_size", 100, 10000},
        {0x00904a14u, "rain_amount", 100, 100},
        {0x00904a94u, "rain_intensity", 45, 100},
        {0x00904a24u, "rain_crossing", 200, 10000},
        {0x00904a28u, "rain_speed", 300, 10000},
        {0x00904a2cu, "rain_gravity", 35, 100}
    };
    for (unsigned i = 0; i < sizeof rain / sizeof rain[0]; ++i)
        put_float(rain[i].address, (float)setting(rain[i].key, rain[i].def) / rain[i].scale);
    api->guest_write_u8(api, 0x00926144u, (uint8_t)setting("skip_movies", 0));
    api->guest_write_u8(api, 0x009260a4u, (uint8_t)setting("skip_nis", 0));
    api->log(api, "core.nfsmw: native Extra Options weather/cinematic settings applied; movie/NIS defaults preserved");
    extra_options_owner(api);
}

/* Test88: Heat Level Override (ExOpts [Pursuit]). Off: the slot keeps 5.0 and the event's
 * own heat range is used - the original game. On: every event's heat range becomes heat_level_min..max and the
 * car's top heat (meters, SetHeatLevel clamp, safehouse icon) becomes heat_level_max. 00443dc3 reads the event's
 * maximum (+0xe0) and minimum (+0xdc) and passes them to 00402060 (value, min, max); the hook replaces both
 * arguments and the event's fields, as ExOpts' code cave does. */
#define HEAT_TOP_SLOT 0x00a37c44u
static int g_heat_logged = -1;
static void heat_levels_apply(const PopModApi *api) {
    const int on = (int)setting("heat_level_override", 0);
    int lo = (int)setting("heat_level_min", 1), hi = (int)setting("heat_level_max", 10);
    if (lo < 1) lo = 1;
    if (hi > 10) hi = 10;
    if (hi < lo) hi = lo;
    put_float(HEAT_TOP_SLOT, on ? (float)hi : 5.0f);
    const int state = on ? lo * 16 + hi : 0;
    if (state != g_heat_logged) {
        g_heat_logged = state;
        char line[120];
        if (on) snprintf(line, sizeof line, "core.nfsmw: Heat Level Override on: heat %d to %d in every event", lo, hi);
        else snprintf(line, sizeof line, "core.nfsmw: Heat Level Override off (each event's own heat range, top heat 5)");
        api->log(api, line);
    }
}
static void heat_levels_event(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    if (setting("heat_level_override", 0)) {
        int lo = (int)setting("heat_level_min", 1), hi = (int)setting("heat_level_max", 10);
        if (lo < 1) lo = 1;
        if (hi > 10) hi = 10;
        if (hi < lo) hi = lo;
        const uint32_t min_bits = float_bits((float)lo), max_bits = float_bits((float)hi);
        api->guest_write_u32(api, cpu->esp + 8u, min_bits);    /* (value, min, max) */
        api->guest_write_u32(api, cpu->esp + 12u, max_bits);
        if (cpu->esi) {
            api->guest_write_u32(api, cpu->esi + 0xdcu, min_bits);
            api->guest_write_u32(api, cpu->esi + 0xe0u, max_bits);
        }
    }
    api->call_original(api, cpu->target, cpu);
}
