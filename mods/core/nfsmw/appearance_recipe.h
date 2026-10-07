/* Appearance recipes with a truthful Custom state (Appearance and Quality are separate recipe domains; a single edit
 * is respected; a recipe is applied once when chosen, never re-asserted).
 *   0 Definitive (default)    hybrid_exposure 50, hybrid_vignette 0, hybrid_shadows 0
 *   1 (retired)               an earlier look; read as Definitive
 *   2 PC - Original           no adjustable appearance values
 *   3 Definitive - Custom     Definitive with any of its tuning values changed
 *   4 (retired)               an earlier custom look; read as Definitive
 * Motion blur and cinematic depth of field are separate switches, not part of the recipe. A profile holding 1 or 4
 * moves, once, to Definitive, and the log says so. The menu steps over the empty choice label of slot 1. Choosing
 * Definitive writes its recipe values; choosing Definitive - Custom keeps the current values. The label then follows
 * the actual values (twice a second): an edited value turns Definitive into Custom, values equal to the recipe turn
 * Custom back. */
#define APP_RETIRED_LOOK 1
#define APP_HYBRID_CUSTOM 3
#define APP_RETIRED_CUSTOM 4
static int64_t g_app_seen = -1;
static uint64_t g_app_tick;
static int app_definitive_original(void) {
    return setting("hybrid_exposure", 50) == 50 && setting("hybrid_vignette", 0) == 0 && setting("hybrid_shadows", 0) == 0;
}
static void app_definitive_recipe(const PopModApi *api) {
    api->settings_set(api, "hybrid_exposure", 50);
    api->settings_set(api, "hybrid_vignette", 0);
    api->settings_set(api, "hybrid_shadows", 0);
}
static void appearance_recipe(const PopModApi *api) {
    static const char *const names[] = {"Definitive", "(retired)", "PC - Original", "Definitive - Custom",
                                        "(retired custom)"};
    int64_t v = LOOK_HYBRID;
    api->settings_get(api, "appearance", &v);
    if (v == APP_RETIRED_LOOK || v == APP_RETIRED_CUSTOM || v < 0 || v > APP_RETIRED_CUSTOM) {
        api->settings_set(api, "appearance", LOOK_HYBRID);
        char line[220];
        snprintf(line, sizeof line, "core.nfsmw: appearance %lld (retired) moved to Definitive", (long long)v);
        api->log(api, line);
        g_app_seen = LOOK_HYBRID;
        return;
    }
    if (g_app_seen >= 0 && v != g_app_seen) {   /* chosen in the menu (or by a test key) */
        char line[160];
        if (v == LOOK_HYBRID) {
            app_definitive_recipe(api);
            snprintf(line, sizeof line, "core.nfsmw: appearance recipe %s applied (auto exposure 50, vignette 0, shadow detail 0)", names[v]);
        } else {
            snprintf(line, sizeof line, "core.nfsmw: appearance %s chosen (current values kept)", names[v]);
        }
        api->log(api, line);
        g_app_seen = v;
        return;
    }
    if (g_app_seen >= 0 && ++g_app_tick % 30u) return;
    const int64_t want = v == LOOK_PC ? LOOK_PC : app_definitive_original() ? LOOK_HYBRID : APP_HYBRID_CUSTOM;
    if (want != v) {
        api->settings_set(api, "appearance", want);
        char line[120];
        snprintf(line, sizeof line, "core.nfsmw: appearance shows %s (its values %s the recipe)", names[want],
                 want == APP_HYBRID_CUSTOM ? "differ from" : "match");
        api->log(api, line);
    }
    g_app_seen = want;
}
