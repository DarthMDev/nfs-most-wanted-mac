/* Test149 (3 Oct 2026): optional switch for the PC preculler, the game's precomputed per-cell visibility.
 * Owner reports: trees pop in as the car passes the stadium (both loops, 3 Oct). The scenery probe shows dozens of
 * trees and fences at 300-450 m (250-640 px at 4K) going precull -> drawn in one frame (07:29:05.142): the camera
 * entered a new preculler cell whose authored visibility list includes them. Original data (Test127b), but the pop
 * is large at 4K.
 * Mechanism (PROVEN, 00731c40-00731cc7): before deciding instances, the cull setup writes cell = -1 and computes a
 * real cell only if "preculler allowed"; allowed is cleared for cells marked in the exception bitmap 0x009a2f40
 * (2048 bytes, 128x128 cells, loaded per track from PrecullerBooBooScript.hoo by 00728a30). With cell -1, 00729120
 * skips the preculler test entirely (frustum, size and LOD still apply).
 * Switch `preculler` (default on = original). Off: every frame the bitmap is filled with 0xFF (after saving the loaded
 * copy), i.e. the exception every cell already can have; turning it back on restores the saved copy. A track load
 * rewrites the bitmap, and the next frame saves and fills it again. Cost: more instances reach the frustum/size
 * tests and the draw list (Test149 measures the draw-list peak against its 5000 limit). */
#define PRECULL_EXCEPTIONS 0x009a2f40u
#define PRECULL_BYTES 2048u
static uint8_t g_pc_saved[PRECULL_BYTES];
static int g_pc_have_saved;
static void preculler_switch(const PopModApi *api) {
    const int off = setting("preculler", 1) == 0;
    if (!off && !g_pc_have_saved) return;
    uint8_t *map = NULL;
    if (api->guest_ptr(api, PRECULL_EXCEPTIONS, PRECULL_BYTES, (void **)&map) != POP_OK || !map) return;
    int all = 1;
    for (uint32_t i = 0; i < PRECULL_BYTES && all; ++i) all = map[i] == 0xFF;
    if (off) {
        if (all) return;
        memcpy(g_pc_saved, map, PRECULL_BYTES);   /* the loaded exceptions (a new track load lands here too) */
        g_pc_have_saved = 1;
        memset(map, 0xFF, PRECULL_BYTES);
        static uint32_t n;
        if (++n <= 3) api->log(api, "core.nfsmw: preculler off: precomputed visibility skipped for every cell");
    } else if (all) {
        memcpy(map, g_pc_saved, PRECULL_BYTES);
        g_pc_have_saved = 0;
        api->log(api, "core.nfsmw: preculler on again: the track's exception cells restored");
    } else {
        g_pc_have_saved = 0;   /* a load replaced our fill: nothing to restore */
    }
}
