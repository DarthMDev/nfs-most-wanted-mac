/* Light pools from the PC game's own street lamps and tunnel lights (setting `clean_pools`). Included by nfsmw.c.
 *
 * Data: clean-pools.bin in the mod folder, built at setup from the player's own PC game (tools/effects/pools_bin.py:
 * the lamp scenery of the PC track data with the lamp class table); clean_pools.h parses it and picks, per draw, the
 * lights whose sphere meets the drawn object's box. Shading: clean-world.fx and clean-worldreflect.fx, copies of the
 * PC game's own WORLD and WORLDREFLECT effects with one added pass (tools/effects/pools_fx.py, from
 * effects/clean-pools.hlsl), installed through their verified resource data entries.
 *
 * Hooks (all PC 1.3):
 *  - call site 007241fe (00724080's call of 006da9b0 for one draw item; edi = item {model, matrix, instance}): the
 *    render entries the call adds, [before, after) of the render list 0093e878 (stride 0x44), are tagged with the
 *    item's lights, moved into each entry's model space when the item is placed by a matrix;
 *  - call site 006e03e1 (006e0200's per-entry call of 006c6d10; esi = render entry): a tagged WORLD / WORLDREFLECT
 *    entry gets its lights and the effect's second pass (pass count 2 for that entry);
 *  - 006e0200 AFTER: the list is done; pass 0's states are re-applied once.
 * The engine's pass loop (006e05b0) draws [eEffect + 0x14] passes but only begins pass 0 when the effect changes,
 * so after an entry that used the added pass the next entry of the same effect is put back on pass 0. */
#ifndef WORLD_LIGHTS_H
#define WORLD_LIGHTS_H
#include "clean_pools.h"

#define WL_MAX_ENTRIES 4096u /* PC render-entry array 0093e878, 0x44 bytes, max 0x1000 */

/* ---- pure part ---- */
/* Placed models: the draw item's matrix M maps model to world, world = local * M (D3D row vectors: rows 0-2 the
 * basis A, row 3 the translation T), and the effects' vertex positions are model-local. A light's world position
 * goes to model space as p * A^-1 - T * A^-1; out[0..8] = A^-1 (row-major), out[9..11] = -T * A^-1. Returns 0 when
 * A is singular or not finite. */
static int wl_inverse_affine(const float m[16], float out[12]) {
    const double a = m[0], b = m[1], c = m[2], d = m[4], e = m[5], f = m[6], g = m[8], h = m[9], i = m[10];
    const double det = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
    if (!(fabs(det) > 1e-12) || !isfinite(det)) return 0;
    const double inv[9] = {(e * i - f * h) / det, (c * h - b * i) / det, (b * f - c * e) / det,
                           (f * g - d * i) / det, (a * i - c * g) / det, (c * d - a * f) / det,
                           (d * h - e * g) / det, (b * g - a * h) / det, (a * e - b * d) / det};
    for (int k = 0; k < 9; ++k) out[k] = (float)inv[k];
    for (int col = 0; col < 3; ++col)
        out[9 + col] = (float)-(m[12] * inv[col] + m[13] * inv[3 + col] + m[14] * inv[6 + col]);
    return 1;
}
static void wl_to_local(const float inv[12], float p[3]) {
    const float x = p[0], y = p[1], z = p[2];
    for (int col = 0; col < 3; ++col)
        p[col] = x * inv[col] + y * inv[3 + col] + z * inv[6 + col] + inv[9 + col];
}
static int wl_is_identity(const float m[16]) {
    for (int k = 0; k < 16; ++k)
        if (fabsf(m[k] - ((k % 5) == 0 ? 1.0f : 0.0f)) > 1e-5f) return 0;
    return 1;
}

/* ---- state ---- */
static Cpl g_cpl;
static int g_wl_ready, g_wl_hooked, g_wl_live = 1;
static uint32_t g_wl_tag[WL_MAX_ENTRIES], g_wl_tag_part[WL_MAX_ENTRIES], g_wl_last_after;  /* tag: 1 lit, 0 none */
/* Per render entry: 0 world-space model (no matrix, or identity); 1 placed (g_wl_inv valid); 2 matrix unreadable
 * or singular (no light). */
static uint8_t g_wl_space[WL_MAX_ENTRIES];
static float g_wl_inv[WL_MAX_ENTRIES][12];
static uint8_t g_cpl_count[WL_MAX_ENTRIES];
static float g_cpl_regs[WL_MAX_ENTRIES][2 * CPL_MAX][4];
static uint32_t g_cpl_values;              /* guest: CleanPools[16], CleanPoolCount, CleanPoolParams, then the names */
static uint32_t g_cpl_handles[2][3];       /* per pass: CleanPools, CleanPoolCount, CleanPoolParams */
static uint64_t g_cpl_items, g_cpl_lit_items, g_cpl_lights_sum, g_cpl_draws, g_wl_single_pass, g_wl_other_view;
static uint32_t g_wl_scratch, g_wl_prev_effect, g_wl_prev_aux, g_wl_key_hook;
/* 00982b25 (set per frame by 006e7220; read only by the two pass-count checks 006e0200/006d7660) is set for one
 * entry so the engine draws the WORLD effect's second pass, and put back at the next entry / after the list. */
static int g_wl_b25_set;
static uint32_t g_wl_b25_old;
static void wl_set_b25(uint32_t v) {
    const uint32_t w = get_u32(0x00982b24u);
    g_api->guest_write_u32(g_api, 0x00982b24u, (w & ~0xff00u) | ((v & 0xffu) << 8));
}
static void wl_restore_flag(void) {
    if (g_wl_b25_set) wl_set_b25(g_wl_b25_old);
    g_wl_b25_set = 0;
}
static void wl_tag(uint32_t before, uint32_t after, int lit, const uint32_t *parts) {
    if (before < g_wl_last_after) {   /* a count below the last seen one: the list was reset */
        memset(g_wl_tag, 0, sizeof g_wl_tag);
        memset(g_wl_tag_part, 0, sizeof g_wl_tag_part);
    }
    g_wl_last_after = after;
    if (after > WL_MAX_ENTRIES) after = WL_MAX_ENTRIES;
    for (uint32_t i = before; i < after; ++i) {
        g_wl_tag[i] = lit ? 1u : 0u;
        g_wl_tag_part[i] = lit ? parts[i - before] : 0u;
    }
}
static int wl_entry_lit(uint32_t index, uint32_t part) {
    return index < WL_MAX_ENTRIES && g_wl_tag[index] && g_wl_tag_part[index] == part;
}
static uint8_t wl_item_space(const PopModApi *api, uint32_t matrix, float inv[12]) {
    if (!matrix) return 0;
    void *m = NULL;
    if (api->guest_ptr(api, matrix, 64, &m) != POP_OK || !m) return 2;
    if (wl_is_identity((const float *)m)) return 0;
    return wl_inverse_affine((const float *)m, inv) ? 1 : 2;
}
static uint32_t wl_fx(uint32_t id3dx, uint32_t slot) { return get_u32(get_u32(id3dx) + slot); }
static void wl_restore_pass0(const PopModApi *api, uint32_t id3dx, int begun) {
    uint32_t r = 0;
    if (begun) {
        const uint32_t a1[] = {id3dx};
        api->guest_call(api, wl_fx(id3dx, 0x108), 0, a1, 1, &r);            /* EndPass */
        const uint32_t a2[] = {id3dx, 0};
        api->guest_call(api, wl_fx(id3dx, 0x100), 0, a2, 2, &r);            /* BeginPass(0) */
    } else {
        const uint32_t a1[] = {id3dx, g_wl_scratch, 0};
        api->guest_call(api, wl_fx(id3dx, 0xfc), 0, a1, 3, &r);             /* Begin */
        const uint32_t a2[] = {id3dx, 0};
        api->guest_call(api, wl_fx(id3dx, 0x100), 0, a2, 2, &r);            /* BeginPass(0) */
        api->guest_call(api, wl_fx(id3dx, 0x108), 0, a2, 1, &r);            /* EndPass */
        api->guest_call(api, wl_fx(id3dx, 0x10c), 0, a2, 1, &r);            /* End */
    }
}

/* The two effects that get the pass, each through its verified resource data entry. */
typedef struct {
    uint32_t type, entry_va, rva, size;
    const char *file, *name;
    int ready;
    uint32_t cached_fx;
} WlPass;
static WlPass g_wl_passes[2] = {
    {0u, 0x009c7668u, 0x005c83e0u, 49980u, "clean-world.fx", "WORLD", 0, 0},
    {1u, 0x009c7658u, 0x005dbf08u, 57784u, "clean-worldreflect.fx", "WORLDREFLECT", 0, 0},
};
static WlPass *wl_pass_for(uint32_t type) {
    for (unsigned i = 0; i < 2; ++i)
        if (g_wl_passes[i].ready && g_wl_passes[i].type == type) return &g_wl_passes[i];
    return NULL;
}
static int wl_install_effect(const PopModApi *api, WlPass *ps) {
    char path[2048];
    snprintf(path, sizeof path, "%s/%s", api->mod_dir(api), ps->file);
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    long size = -1;
    if (get_u32(ps->entry_va) != ps->rva || get_u32(ps->entry_va + 4) != ps->size || fseek(f, 0, SEEK_END) != 0 ||
        (size = ftell(f)) <= 0 || size > 1024 * 1024 || fseek(f, 0, SEEK_SET) != 0) { fclose(f); return 0; }
    uint32_t address = 0;
    void *bytes = NULL;
    if (api->guest_alloc(api, (uint32_t)size, &address) != POP_OK ||
        api->guest_ptr(api, address, (uint32_t)size, &bytes) != POP_OK ||
        fread(bytes, 1, (size_t)size, f) != (size_t)size) { fclose(f); return 0; }
    fclose(f);
    api->guest_write_u32(api, ps->entry_va, address - 0x00400000u);
    api->guest_write_u32(api, ps->entry_va + 4, (uint32_t)size);
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: light pools: pass installed in the %s effect (%s)", ps->name, ps->file);
    api->log(api, line);
    return ps->ready = 1;
}

/* ---- hooks ---- */
static void wl_render_item(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    const uint32_t instance = get_u32(cpu->edi + 8);
    const uint32_t matrix = get_u32(cpu->edi + 4); /* 0: the model is in world space */
    void *p = NULL;
    if (instance) api->guest_ptr(api, instance, 0x40, &p);
    const uint32_t before = get_u32(0x00982cdcu);
    api->call_original(api, cpu->target, cpu);
    uint32_t after = get_u32(0x00982cdcu);
    if (after < before || after > WL_MAX_ENTRIES) after = before;
    uint32_t parts[64];
    if (after - before > 64) after = before + 64; /* one model: a handful of parts */
    for (uint32_t i = before; i < after; ++i) parts[i - before] = get_u32(0x0093e878u + i * 0x44u);
    sign_trace_item(api, instance, before, after);
    if (!g_wl_ready) return;
    {
        float inv[12];
        const uint8_t space = wl_item_space(api, matrix, inv);
        for (uint32_t i = before; i < after && i < WL_MAX_ENTRIES; ++i) {
            g_wl_space[i] = space;
            if (space == 1) memcpy(g_wl_inv[i], inv, sizeof inv);
        }
    }
    /* Lights whose sphere meets the instance's world box (instance +0x00 min, +0x0c max), nearest to the camera
     * first, moved into each entry's space. */
    uint32_t m = 0, idx[CPL_MAX];
    ++g_cpl_items;
    if (p && before < WL_MAX_ENTRIES && g_wl_space[before] != 2) {
        const float *inst = (const float *)p;
        float eye[3] = {0, 0, 0}, viewm[16], vinv[12];
        const uint32_t cam = get_u32(0x009195e0u + 0x70u + 0x40u);
        int have_eye = 0;
        if (cam) {
            for (unsigned k = 0; k < 16; ++k) viewm[k] = get_float(cam + 4u * k);
            if (wl_inverse_affine(viewm, vinv)) { wl_to_local(vinv, eye); have_eye = 1; }
        }
        m = cpl_select(&g_cpl, inst, inst + 3, have_eye ? eye : NULL, idx);
    }
    if (m) {
        ++g_cpl_lit_items;
        g_cpl_lights_sum += m;
        float block[2 * CPL_MAX][4];
        cpl_block(&g_cpl, idx, m, block);
        for (uint32_t i = before; i < after && i < WL_MAX_ENTRIES; ++i) {
            memcpy(g_cpl_regs[i], block, sizeof block);
            if (g_wl_space[i] == 1)
                for (uint32_t j = 0; j < m; ++j) wl_to_local(g_wl_inv[i], g_cpl_regs[i][2 * j]);
            g_cpl_count[i] = (uint8_t)m;
        }
    }
    wl_tag(before, after, m != 0, parts);
    if (g_cpl_items == 1 || g_cpl_items % 2000000 == 0) {
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: light pools: %llu draw items, %llu with lights (mean %.2f lights), %llu lit draws",
                 (unsigned long long)g_cpl_items, (unsigned long long)g_cpl_lit_items,
                 g_cpl_lit_items ? (double)g_cpl_lights_sum / (double)g_cpl_lit_items : 0.0, (unsigned long long)g_cpl_draws);
        api->log(api, line);
    }
}
/* One lit WORLD / WORLDREFLECT entry: its lights into the effect, and the second pass. */
static void wl_world_entry(const PopModApi *api, WlPass *ps, uint32_t eeffect, uint32_t index, uint32_t id3dx) {
    const unsigned k = ps->type ? 1u : 0u;
    if (ps->cached_fx != id3dx) {
        static const uint32_t off[3] = {0u, 11u, 26u};   /* name offsets in the guest name block */
        for (unsigned h = 0; h < 3; ++h) {
            const uint32_t args[] = {id3dx, 0, g_cpl_values + 18 * 16 + off[h]};
            g_cpl_handles[k][h] = 0;
            api->guest_call(api, wl_fx(id3dx, 0x24), 0, args, 3, &g_cpl_handles[k][h]);   /* GetParameterByName */
        }
        ps->cached_fx = id3dx;
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: light pools: %s effect %08x, handles %08x %08x %08x", ps->name, id3dx,
                 g_cpl_handles[k][0], g_cpl_handles[k][1], g_cpl_handles[k][2]);
        api->log(api, line);
    }
    if (!g_cpl_handles[k][0] || !g_cpl_handles[k][1] || !g_cpl_handles[k][2] || index >= WL_MAX_ENTRIES) return;
    void *dst = NULL;
    if (api->guest_ptr(api, g_cpl_values, 18 * 16, &dst) != POP_OK || !dst) return;
    float *v = (float *)dst;
    memcpy(v, g_cpl_regs[index], sizeof g_cpl_regs[index]);
    v[64] = (float)g_cpl_count[index]; v[65] = v[66] = v[67] = 0.0f;
    memcpy(v + 68, g_cpl.params, 16);
    {   /* setting clean_pools_strength (%, live): scales the global strength */
        static uint32_t calls;
        static float scale = 1.0f;
        if ((calls++ & 255u) == 0) scale = (float)setting("clean_pools_strength", 100) / 100.0f;
        v[70] *= scale;
    }
    static const uint32_t voff[3] = {0u, 16u * 16u, 17u * 16u}, vsize[3] = {16u * 16u, 16u, 16u};
    for (unsigned h = 0; h < 3; ++h) {
        uint32_t r = 0;
        const uint32_t args[] = {id3dx, g_cpl_handles[k][h], g_cpl_values + voff[h], vsize[h]};
        api->guest_call(api, wl_fx(id3dx, 0x50), 0, args, 4, &r);          /* SetValue */
    }
    g_api->guest_write_u32(g_api, eeffect + 0x14, 2);
    g_wl_prev_aux = 1;
    ++g_cpl_draws;
}
static void wl_draw_entry(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    const uint32_t entry = cpu->esi;
    wl_restore_flag();
    if (entry < 0x0093e878u || (entry - 0x0093e878u) % 0x44u) return;
    const uint32_t eeffect = get_u32(entry + 0x10);
    const uint32_t index = (entry - 0x0093e878u) / 0x44u;
    WlPass *ps = NULL;
    if (g_wl_ready) {
        if (g_wl_prev_aux && eeffect == g_wl_prev_effect && get_u32(eeffect + 0x48))
            wl_restore_pass0(api, get_u32(eeffect + 0x48), 1);
        g_wl_prev_aux = 0;
        g_wl_prev_effect = eeffect;
        ps = eeffect ? wl_pass_for(get_u32(eeffect + 4)) : NULL;
    }
    if (ps) {
        g_api->guest_write_u32(g_api, eeffect + 0x14, 1);
        const uint32_t id3dx = get_u32(eeffect + 0x48);
        if (wl_entry_lit(index, get_u32(entry)) && g_wl_live && g_look != LOOK_PC && id3dx) {
            int other_view = 0;
            /* 006e0200 draws a single pass, whatever the count, for WORLD/WORLDBONE/type 4 when the effect's byte
             * +0x40 is set, unless 00982b25 is set and the view (first word of 00982a20's array) is 0. Main view:
             * 00982b25 is set for this entry. Other views (mirror, reflections): no added pass. */
            if (ps->type == 0u) {
                const uint32_t list = get_u32(0x00982a20u);
                const uint32_t b25 = (get_u32(0x00982b24u) >> 8) & 0xffu;
                if ((b25 == 0 || (list && get_u32(list) != 0)) && (get_u32(eeffect + 0x40) & 0xffu)) {
                    if (list && get_u32(list) != 0) { ++g_wl_other_view; other_view = 1; }
                    else {
                        ++g_wl_single_pass;
                        g_wl_b25_old = b25;
                        g_wl_b25_set = 1;
                        wl_set_b25(1);
                    }
                }
            }
            if (!other_view) wl_world_entry(api, ps, eeffect, index, id3dx);
        }
    }
    sign_trace_draw(index, get_u32(entry), ps ? 1u : 0u);
}
static void wl_list_done(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    wl_restore_flag();
    if (g_wl_prev_aux && g_wl_prev_effect && get_u32(g_wl_prev_effect + 0x48))
        wl_restore_pass0(api, get_u32(g_wl_prev_effect + 0x48), 0);
    g_wl_prev_aux = 0;
    g_wl_prev_effect = 0;
}
/* TEST ONLY (NFSMW_TEST_AUX_PASS_KEY set): F4 switches the pools off/on at run time; the key is consumed. */
static int32_t wl_test_key(const PopModApi *api, int32_t dik, int32_t vk, int32_t down, void *user) {
    (void)vk; (void)user;
    if (dik != 0x3e) return 0; /* DIK_F4 */
    if (down) {
        g_wl_live = !g_wl_live;
        api->log(api, g_wl_live ? "core.nfsmw: light pools: TEST on" : "core.nfsmw: light pools: TEST off");
    }
    return 1;
}

static int wl_load(const PopModApi *api) {
    if (!setting("clean_pools", 1)) return 0;
    char path[2048];
    snprintf(path, sizeof path, "%s/clean-pools.bin", api->mod_dir(api));
    FILE *f = fopen(path, "rb");
    uint8_t *b = NULL;
    long n = -1;
    int ok = 0;
    if (f && fseek(f, 0, SEEK_END) == 0 && (n = ftell(f)) > 0 && n < 16 * 1024 * 1024 && fseek(f, 0, SEEK_SET) == 0 &&
        (b = (uint8_t *)malloc((size_t)n)) && fread(b, 1, (size_t)n, f) == (size_t)n)
        ok = cpl_parse(&g_cpl, b, (size_t)n);
    free(b);
    if (f) fclose(f);
    char line[200];
    snprintf(line, sizeof line, ok ? "core.nfsmw: light pools: %u PC lamp lights (fade %.0f-%.0f m, strength %.2f, road normal %.2f)"
                                   : "core.nfsmw: light pools: clean-pools.bin missing or rejected (%u); run the setup to build it",
             g_cpl.n, (double)g_cpl.params[0], (double)g_cpl.params[1], (double)g_cpl.params[2], (double)g_cpl.params[3]);
    api->log(api, line);
    if (!ok) return 0;
    static const char names[] = "CleanPools\0CleanPoolCount\0CleanPoolParams";
    void *bytes = NULL;
    if (api->guest_alloc(api, 18 * 16 + sizeof names, &g_cpl_values) != POP_OK ||
        api->guest_ptr(api, g_cpl_values + 18 * 16, sizeof names, &bytes) != POP_OK) return 0;
    memcpy(bytes, names, sizeof names);
    if (api->guest_alloc(api, 16, &g_wl_scratch) != POP_OK) return 0;
    int any = 0;
    for (unsigned i = 0; i < 2; ++i) any |= wl_install_effect(api, &g_wl_passes[i]);
    if (!any) api->log(api, "core.nfsmw: light pools: clean-world.fx / clean-worldreflect.fx missing or not the verified entries; off");
    return any;
}
static void wl_install(const PopModApi *api) {
    const int ready = wl_load(api);
    if (!ready && !st_enabled()) return;   /* the hooks also feed the scenery sign trace (scenery_trace) */
    if (install_at(0x006da9b0u, 0x00724203u, wl_render_item, POP_HOOK_REPLACE, 68) != POP_OK ||
        install_at(0x006c6d10u, 0x006e03e6u, wl_draw_entry, POP_HOOK_BEFORE, 69) != POP_OK) {
        api->log(api, "core.nfsmw: light pools unavailable (hook)");
        return;
    }
    g_wl_hooked = 1;
    if (!ready) return;
    if (install(0x006e0200u, wl_list_done, POP_HOOK_AFTER, 70) != POP_OK) {
        api->log(api, "core.nfsmw: light pools: pass unavailable (hook 006e0200)");
        return;
    }
    g_wl_ready = 1;
    if (getenv("NFSMW_TEST_AUX_PASS_KEY") && api->on_key)
        api->on_key(api, wl_test_key, NULL, &g_wl_key_hook);
}
static void wl_exit(const PopModApi *api) {
    if (g_wl_ready) {
        char line[240];
        snprintf(line, sizeof line, "core.nfsmw: light pools: %llu draw items, %llu lit, %llu lit draws (%llu past the single-pass check), %llu in other views (no pass)",
                 (unsigned long long)g_cpl_items, (unsigned long long)g_cpl_lit_items, (unsigned long long)g_cpl_draws,
                 (unsigned long long)g_wl_single_pass, (unsigned long long)g_wl_other_view);
        api->log(api, line);
    }
    if (g_wl_key_hook) { api->hook_remove(api, g_wl_key_hook); g_wl_key_hook = 0; }
    cpl_free(&g_cpl);
    g_wl_ready = g_wl_hooked = 0;
}
#endif
