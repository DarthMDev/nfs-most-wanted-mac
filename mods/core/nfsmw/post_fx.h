/* Post-process host for the rebuilt effects. Included by nfsmw.c.
 *
 * Once per rendered frame of the main view, a BEFORE hook on the frame's call of 006c3870 (006e75a1, return
 * 006e75a6), which follows the main view's render 006de300 and precedes the HUD and front end, draws the rebuilt
 * post-process chain (clean_chain.h: glow, auto brightness, edge darkening, motion blur and depth of field) through
 * the game's own Direct3D 9 device. Every render state, sampler state, render target, texture, viewport and vertex
 * declaration it changes is read first and put back, so the game never sees a difference.
 *
 * The host itself holds no lighting algorithm. It owns:
 *  - the frame check (only a frame the main view actually drew; a repeated display frame is not processed again);
 *  - the save and restore of the device state around the chain;
 *  - the HDR-output scene mark (alpha 1 over the finished pre-HUD scene, technique alpha_one of post-host.fx; the
 *    renderer then expands highlights only under that mark, so the HUD, mirror and menus keep normal white);
 *  - the main view's depth surface, captured while the view draws (released at the end of the frame), which the
 *    depth of field reads;
 *  - the main view's colour pass alpha writes (COLORWRITEMODE 15), which the scene mark relies on.
 *
 * One owner: when the PC OverBright bloom or visual treatment is switched on (the PC - Original look, or the
 * golden haze turned back on), the PC path owns bloom and this stage stands aside. */
#ifndef POST_FX_H
#define POST_FX_H

/* Render states a pass sets (ZENABLE, ZWRITEENABLE, ALPHATESTENABLE, SRCBLEND, DESTBLEND, CULLMODE,
 * ALPHABLENDENABLE, FOGENABLE, STENCILENABLE, COLORWRITEENABLE, BLENDOP, SCISSORTESTENABLE, SRGBWRITEENABLE,
 * SEPARATEALPHABLENDENABLE) and sampler-0 states (ADDRESSU/V, MAG/MIN/MIPFILTER, SRGBTEXTURE), with the
 * values the passes use. */
static const uint32_t k_pfx_rs[][2] = {{7, 0}, {14, 0}, {15, 0}, {19, 2}, {20, 2}, {22, 1}, {27, 0},
                                       {28, 0}, {52, 0}, {168, 15}, {171, 1}, {174, 0}, {194, 0}, {206, 0}};
static const uint32_t k_pfx_ss[][2] = {{1, 3}, {2, 3}, {5, 2}, {6, 2}, {7, 0}, {11, 0}};
#define PFX_NRS (sizeof k_pfx_rs / sizeof k_pfx_rs[0])
#define PFX_NSS (sizeof k_pfx_ss / sizeof k_pfx_ss[0])

static int g_pfx_enabled, g_pfx_broken, g_pfx_live = 1, g_pfx_pc_owner_logged, g_pfx_aside;
static uint32_t g_pfx_fx, g_pfx_alpha_tech, g_pfx_decl, g_pfx_mem, g_pfx_key_hook;
static uint32_t g_pfx_prev_game_frame;
static uint64_t g_pfx_calls, g_pfx_repeats, g_pfx_skipped, g_pfx_reseeds, g_pfx_hdr_marks;
/* The depth-stencil surface bound while the main view draws its colour pass, held (one reference from
 * GetDepthStencilSurface) until the end of the frame, so the depth of field reads the scene's depth even when
 * nothing is bound at post-process time (as during the onset of rain). */
static uint32_t g_pfx_view_ds;
static uint64_t g_pfx_sd_tries, g_pfx_sd_captured, g_pfx_sd_unused;
static uint64_t pfx_now_ns(void) {
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000ull + (uint64_t)t.tv_nsec;
}
/* The COLORWRITEMODE the main view's colour pass gets (TEST ONLY override: NFSMW_TEST_EFFECTS_CWMODE). */
static int g_pfx_cwmode = 15, g_pfx_cw_logged;

#define PFX_OUT (g_pfx_mem + 0x000u)     /* out pointers */
#define PFX_VP_SAVE (g_pfx_mem + 0x040u) /* D3DVIEWPORT9 */
#define PFX_VERTS (g_pfx_mem + 0x080u)   /* 4 x (float4 position, float2 uv) */
#define PFX_DECL (g_pfx_mem + 0x320u)
#define PFX_NAMES (g_pfx_mem + 0x400u)   /* strings, 32 bytes each */
#define PFX_SCRATCH (g_pfx_mem + 0x600u) /* effect creation: out pointer, +0x100 resource name */
#define PFX_MEM_SIZE 0x800u

static void pfx_put(uint32_t addr, const void *p, uint32_t n) {
    void *dst = NULL;
    if (g_api->guest_ptr(g_api, addr, n, &dst) == POP_OK && dst) memcpy(dst, p, n);
}
/* A COM method: `obj` is `this`; up to 9 further arguments. 0x80004005 when the call itself failed. */
static uint32_t pfx_call(uint32_t obj, uint32_t off, const uint32_t *args, unsigned n) {
    uint32_t a[10], r = 0x80004005u;
    if (!obj || n > 9) return r;
    a[0] = obj;
    for (unsigned i = 0; i < n; ++i) a[i + 1] = args[i];
    const uint32_t fn = get_u32(get_u32(obj) + off);
    if (!fn || g_api->guest_call(g_api, fn, 0, a, n + 1, &r) != POP_OK) return 0x80004005u;
    return r;
}
static void pfx_release(uint32_t *obj) {
    if (*obj) pfx_call(*obj, 0x08, NULL, 0);
    *obj = 0;
}
static uint32_t pfx_name(unsigned i, const char *s) {
    const uint32_t at = PFX_NAMES + 32u * i;
    pfx_put(at, s, (uint32_t)strlen(s) + 1u);
    return at;
}
/* The shared effect parameter COLORWRITEMODE (semantic), through the effect (+0x48) of the first shader object
 * in the game's table 0093de78 (31 entries, filled by 006d6040; all effects share one pool, so the value reaches
 * every effect). Sets it; returns the value, or < 0. */
static int pfx_cwmode_set(int set) {
    uint32_t fx = 0;
    for (uint32_t i = 0; i < 31u && !fx; ++i) {
        const uint32_t shader = get_u32(0x0093de78u + 4u * i);
        if (shader) fx = get_u32(shader + 0x48u);
    }
    if (!fx) return -1;
    const uint32_t a[] = {0, pfx_name(30, "COLORWRITEMODE")};
    const uint32_t h = pfx_call(fx, 0x28, a, 2);                      /* GetParameterBySemantic */
    if (!h || h == 0x80004005u) return -2;
    const uint32_t s[] = {h, (uint32_t)set};
    return pfx_call(fx, 0x68, s, 2) == 0 ? set : -4;                 /* SetInt */
}
/* An A8R8G8B8 (or other `format`) render-target texture and its surface at w x h, kept while the size holds. */
static int pfx_target(uint32_t dev, uint32_t *tex, uint32_t *surf, unsigned *cw, unsigned *ch, unsigned w, unsigned h,
                      uint32_t format) {
    if (*surf && *cw == w && *ch == h) return 1;
    pfx_release(surf);
    pfx_release(tex);
    *cw = *ch = 0;
    const uint32_t a[] = {w, h, 1, 1 /* RENDERTARGET */, format, 0, PFX_OUT, 0};
    if (pfx_call(dev, 0x5c, a, 8) != 0 || !(*tex = get_u32(PFX_OUT))) return 0;          /* CreateTexture */
    const uint32_t s[] = {0, PFX_OUT};
    if (pfx_call(*tex, 0x48, s, 2) != 0 || !(*surf = get_u32(PFX_OUT))) {                 /* GetSurfaceLevel */
        pfx_release(tex);
        return 0;
    }
    *cw = w;
    *ch = h;
    return 1;
}
/* One full-target quad of `tech` (moved half a pixel, as Direct3D 9 needs for texel-exact reads). */
static void pfx_pass_h(uint32_t dev, uint32_t tech, uint32_t surf, unsigned w, unsigned h, uint32_t src, int linear) {
    const uint32_t rt[] = {0, surf};
    pfx_call(dev, 0x94, rt, 2);                                   /* SetRenderTarget (resets the viewport) */
    const uint32_t tx[] = {0, src};
    pfx_call(dev, 0x104, tx, 2);                                  /* SetTexture */
    const uint32_t mag[] = {0, 5, linear ? 2u : 1u}, min[] = {0, 6, linear ? 2u : 1u};
    pfx_call(dev, 0x114, mag, 3);
    pfx_call(dev, 0x114, min, 3);
    const uint32_t t[] = {tech};
    pfx_call(g_pfx_fx, 0xe8, t, 1);                               /* SetTechnique */
    const uint32_t b[] = {PFX_OUT + 0x3cu, 0};
    pfx_call(g_pfx_fx, 0xfc, b, 2);                               /* Begin */
    const uint32_t p[] = {0};
    pfx_call(g_pfx_fx, 0x100, p, 1);                              /* BeginPass */
    const float hx = 1.0f / (float)w, hy = 1.0f / (float)h;
    const float v[4][6] = {{-1.0f - hx, 1.0f + hy, 0.5f, 1.0f, 0.0f, 0.0f}, {1.0f - hx, 1.0f + hy, 0.5f, 1.0f, 1.0f, 0.0f},
                           {-1.0f - hx, -1.0f + hy, 0.5f, 1.0f, 0.0f, 1.0f}, {1.0f - hx, -1.0f + hy, 0.5f, 1.0f, 1.0f, 1.0f}};
    pfx_put(PFX_VERTS, v, sizeof v);
    const uint32_t dp[] = {5 /* TRIANGLESTRIP */, 2, PFX_VERTS, 24};
    pfx_call(dev, 0x14c, dp, 4);                                  /* DrawPrimitiveUP */
    pfx_call(g_pfx_fx, 0x108, NULL, 0);                           /* EndPass */
    pfx_call(g_pfx_fx, 0x10c, NULL, 0);                           /* End */
}

#include "clean_chain.h"   /* the rebuilt effects; needs pfx_call/pfx_target/k_pfx_rs above */

/* The host's own effect (post-host.fx: technique alpha_one) and the quad's vertex declaration. */
static int pfx_create(const PopModApi *api, uint32_t dev) {
    char why[160];
    g_pfx_fx = cc_create_effect(api, dev, PFX_SCRATCH, "post-host.fx", why, sizeof why);
    if (g_pfx_fx) {
        const uint32_t a[] = {pfx_name(0, "alpha_one")};
        g_pfx_alpha_tech = pfx_call(g_pfx_fx, 0x34, a, 1);          /* GetTechniqueByName */
        if (g_pfx_alpha_tech == 0x80004005u) g_pfx_alpha_tech = 0;
    } else {
        char line[220];
        snprintf(line, sizeof line, "core.nfsmw: effects: %s; HDR highlights on the scene unavailable", why);
        api->log(api, line);
    }
    /* float4 POSITION, float2 TEXCOORD0, end */
    const uint8_t decl[24] = {0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 16, 0, 1, 0, 5, 0, 0xff, 0, 0, 0, 17, 0, 0, 0};
    pfx_put(PFX_DECL, decl, sizeof decl);
    const uint32_t d[] = {PFX_DECL, PFX_OUT};
    const uint32_t cr = pfx_call(dev, 0x158, d, 2);                  /* CreateVertexDeclaration */
    g_pfx_decl = get_u32(PFX_OUT);
    if (cr != 0 || !g_pfx_decl) {
        char line[160];
        snprintf(line, sizeof line, "core.nfsmw: effects: CreateVertexDeclaration failed (%08x %08x); effects off", cr, g_pfx_decl);
        api->log(api, line);
        return 0;
    }
    return 1;
}

static void pfx_frame_body(const PopModApi *api) {
    g_mirror_fresh = 0;
    if (!g_pfx_enabled || g_pfx_broken) return;
    if (get_u32(0x009017fcu) || get_u32(0x00901828u)) {                /* PC OverBright / visual treatment on */
        if (!g_pfx_pc_owner_logged++)
            api->log(api, "core.nfsmw: effects: the PC bloom/visual treatment is on; it owns bloom, this stage stands aside");
        ++g_pfx_skipped;
        g_pfx_aside = 1;
        return;
    }
    /* Only a frame the main view actually drew: 006de300 counts its renders at 006df5a9 ([009885b8]) and returns
     * early without drawing when the view is inactive (006de424/006de42d, e.g. front-end frames). */
    const uint32_t game_frame = get_u32(0x009885b8u);
    if (game_frame == g_pfx_prev_game_frame) { ++g_pfx_repeats; return; }
    g_pfx_prev_game_frame = game_frame;
    const uint32_t dev = get_u32(0x00982bdcu), bb = get_u32(0x00982a28u);
    if (!dev || !bb) return;
    if (!g_pfx_decl) {
        if (!pfx_create(api, dev)) { g_pfx_broken = 1; return; }
        api->log(api, "core.nfsmw: effects: host ready");
    }
    /* The frame: render target 0 must be the back buffer the view was drawn into. */
    const uint32_t g0[] = {0, PFX_OUT};
    if (pfx_call(dev, 0x98, g0, 2) != 0) return;                       /* GetRenderTarget */
    uint32_t saved_rt = get_u32(PFX_OUT);
    if (saved_rt != bb) { pfx_release(&saved_rt); ++g_pfx_skipped; return; }
    const uint32_t desc_out[] = {PFX_OUT + 8u};
    if (pfx_call(bb, 0x30, desc_out, 1) != 0) { pfx_release(&saved_rt); return; }   /* GetDesc */
    const unsigned out_w = get_u32(PFX_OUT + 8u + 24u), out_h = get_u32(PFX_OUT + 8u + 28u);
    {   /* the frame's size, format and multisampling, once per change */
        static uint32_t logged_key;
        const uint32_t ms = get_u32(PFX_OUT + 8u + 16u), q = get_u32(PFX_OUT + 8u + 20u), fmt = get_u32(PFX_OUT + 8u);
        const uint32_t key = out_w * 2654435761u ^ out_h * 40503u ^ ms * 977u ^ q * 31u ^ fmt;
        if (key != logged_key) {
            logged_key = key;
            char line[160];
            snprintf(line, sizeof line, "core.nfsmw: main view frame %ux%u format %u multisample type %u quality %u (FSAA setting %u)",
                     out_w, out_h, fmt, ms, q, get_u32(0x00901808u));
            api->log(api, line);
        }
    }
    if (out_w < 64u || out_h < 64u) { pfx_release(&saved_rt); return; }
    if (g_pfx_aside) {
        /* Back from the PC treatment (an appearance change): the effects' history is stale. */
        g_pfx_aside = 0;
        g_pfx_pc_owner_logged = 0;
        ++g_pfx_reseeds;
        api->log(api, "core.nfsmw: effects: back from the PC treatment; history reset");
    }
    ++g_pfx_calls;

    /* ---- save ---- */
    uint32_t rs[PFX_NRS], ss[PFX_NSS], saved_tex = 0, saved_decl = 0;
    for (unsigned i = 0; i < PFX_NRS; ++i) {
        const uint32_t a[] = {k_pfx_rs[i][0], PFX_OUT};
        rs[i] = pfx_call(dev, 0xe8, a, 2) == 0 ? get_u32(PFX_OUT) : k_pfx_rs[i][1];
    }
    for (unsigned i = 0; i < PFX_NSS; ++i) {
        const uint32_t a[] = {0, k_pfx_ss[i][0], PFX_OUT};
        ss[i] = pfx_call(dev, 0x110, a, 3) == 0 ? get_u32(PFX_OUT) : k_pfx_ss[i][1];
    }
    {
        const uint32_t a[] = {0, PFX_OUT};
        if (pfx_call(dev, 0x100, a, 2) == 0) saved_tex = get_u32(PFX_OUT);        /* GetTexture */
        const uint32_t d[] = {PFX_OUT};
        if (pfx_call(dev, 0x160, d, 1) == 0) saved_decl = get_u32(PFX_OUT);      /* GetVertexDeclaration */
        const uint32_t v[] = {PFX_VP_SAVE};
        pfx_call(dev, 0xc0, v, 1);                                                /* GetViewport */
    }
    for (unsigned i = 0; i < PFX_NRS; ++i) {
        const uint32_t a[] = {k_pfx_rs[i][0], k_pfx_rs[i][1]};
        pfx_call(dev, 0xe4, a, 2);
    }
    for (unsigned i = 0; i < PFX_NSS; ++i) {
        const uint32_t a[] = {0, k_pfx_ss[i][0], k_pfx_ss[i][1]};
        pfx_call(dev, 0x114, a, 3);
    }
    const uint32_t sd[] = {g_pfx_decl};
    pfx_call(dev, 0x15c, sd, 1);                                                  /* SetVertexDeclaration */

    if (clean_chain_mode() == 1) clean_chain_run(api, dev, bb, out_w, out_h, game_frame);

    /* HDR output: mark the finished pre-HUD scene - alpha 1 over the whole frame (colour untouched); the D3D9
     * renderer clears it under everything drawn after the marker set below (the HUD, mirror, menus), and the
     * presenter expands highlights only where it is still 1. */
    const int hdr_mark = g_pfx_alpha_tech && g_pfx_live && setting("hdr_output", 0);
    if (hdr_mark) {
        const uint32_t st[][2] = {{27, 0}, {168, 8}};
        for (unsigned i = 0; i < 2; ++i) pfx_call(dev, 0xe4, st[i], 2);
        pfx_pass_h(dev, g_pfx_alpha_tech, bb, out_w, out_h, 0, 0);
        ++g_pfx_hdr_marks;
    }
    /* ---- restore ---- */
    {
        const uint32_t rt[] = {0, saved_rt};
        pfx_call(dev, 0x94, rt, 2);
        const uint32_t v[] = {PFX_VP_SAVE};
        pfx_call(dev, 0xbc, v, 1);                                                /* SetViewport */
        const uint32_t t[] = {0, saved_tex};
        pfx_call(dev, 0x104, t, 2);
        const uint32_t d[] = {saved_decl};
        pfx_call(dev, 0x15c, d, 1);
        for (unsigned i = 0; i < PFX_NRS; ++i) {
            const uint32_t a[] = {k_pfx_rs[i][0], rs[i]};
            pfx_call(dev, 0xe4, a, 2);
        }
        for (unsigned i = 0; i < PFX_NSS; ++i) {
            const uint32_t a[] = {0, k_pfx_ss[i][0], ss[i]};
            pfx_call(dev, 0x114, a, 3);
        }
        pfx_release(&saved_tex);
        pfx_release(&saved_decl);
        pfx_release(&saved_rt);
    }
    if (hdr_mark) {   /* private render state 250 (d3d9.cpp resets it at Present) */
        const uint32_t m[] = {250, 1};
        pfx_call(dev, 0xe4, m, 2);
        /* private render state 251 = the rear-view mirror texture, which the HUD lays over the scene: it keeps the
         * HDR highlight expansion. 'mirror_hdr', default on. */
        if (setting("mirror_hdr", 1) && get_u32(0x00982a7cu)) {
            const uint32_t mt[] = {251, get_u32(0x00982a7cu)};
            pfx_call(dev, 0xe4, mt, 2);
        }
    }
    if (g_pfx_calls == 1 || g_pfx_calls % 3600 == 0) {
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: effects: call %llu (game frame %u), %llu skipped, %llu frames without a view render",
                 (unsigned long long)g_pfx_calls, game_frame, (unsigned long long)g_pfx_skipped,
                 (unsigned long long)g_pfx_repeats);
        api->log(api, line);
    }
}
/* The frame's held main-view depth surface goes at the end of every frame, whatever path the frame took. */
static void pfx_frame(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    pfx_frame_body(api);
    pfx_release(&g_pfx_view_ds);
}

/* Before the main view's colour pass (006de210 called at 006ded1c, right after 006de300 set COLORWRITEMODE to
 * 7): alpha writes on, and the view's depth surface taken while the view draws with it. */
static void pfx_view_colour(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)cpu; (void)inv; (void)user;
    if (!g_pfx_enabled || g_pfx_broken) return;
    if (setting("dof_scene_depth", 1) && get_u32(0x00925e90u) == 6) {
        const uint32_t dev = get_u32(0x00982bdcu), g[] = {PFX_OUT};
        if (g_pfx_view_ds) ++g_pfx_sd_unused;
        pfx_release(&g_pfx_view_ds);
        ++g_pfx_sd_tries;
        if (dev && pfx_call(dev, 0xa0, g, 1) == 0) g_pfx_view_ds = get_u32(PFX_OUT);   /* GetDepthStencilSurface */
        if (g_pfx_view_ds) ++g_pfx_sd_captured;
    }
    if (g_pfx_cwmode < 0) return;
    if (get_u32(0x009017fcu) || get_u32(0x00901828u)) return;          /* the PC path owns bloom */
    /* In the world only (game state 00925e90 == 6): the front-end garage and start screen keep the game's own
     * pass, so the pipeline variants with alpha in the write mask are made at the first world frame, not as a
     * hold on the start screen or the main menu. */
    if (get_u32(0x00925e90u) != 6) return;
    if (pfx_cwmode_set(g_pfx_cwmode) < 0 && !g_pfx_cw_logged++)
        api->log(api, "core.nfsmw: effects: COLORWRITEMODE not reachable; the frame keeps no alpha");
}

/* TEST ONLY (NFSMW_TEST_EFFECTS_KEY set): F3 switches the effects off/on, so one scene can be captured both
 * ways; the key is consumed. */
static int32_t pfx_test_key(const PopModApi *api, int32_t dik, int32_t vk, int32_t down, void *user) {
    (void)vk; (void)user;
    if (dik != 0x3d) return 0; /* DIK_F3 */
    if (down) {
        g_pfx_live = !g_pfx_live;
        char line[120];
        snprintf(line, sizeof line, "core.nfsmw: effects: TEST %s at game frame %u", g_pfx_live ? "on" : "off",
                 get_u32(0x009885b8u));
        api->log(api, line);
    }
    return 1;
}

static int pfx_init(const PopModApi *api) {
    if (api->guest_alloc(api, PFX_MEM_SIZE, &g_pfx_mem) != POP_OK) return 0;
    {
        const char *cw = getenv("NFSMW_TEST_EFFECTS_CWMODE");            /* TEST ONLY */
        if (cw && *cw) g_pfx_cwmode = atoi(cw);
    }
    if (getenv("NFSMW_TEST_EFFECTS_KEY") && api->on_key)
        api->on_key(api, pfx_test_key, NULL, &g_pfx_key_hook);
    clean_chain_init(api);
    g_pfx_enabled = 1;
    return 1;
}
static void pfx_exit(const PopModApi *api) {
    if (g_pfx_enabled) {
        char line[300];
        snprintf(line, sizeof line, "core.nfsmw: effects: %llu frames, %llu skipped, %llu frames without a view render, %llu HDR marks; scene depth: %llu tries, %llu captured, %llu replaced unused",
                 (unsigned long long)g_pfx_calls, (unsigned long long)g_pfx_skipped, (unsigned long long)g_pfx_repeats,
                 (unsigned long long)g_pfx_hdr_marks, (unsigned long long)g_pfx_sd_tries,
                 (unsigned long long)g_pfx_sd_captured, (unsigned long long)g_pfx_sd_unused);
        api->log(api, line);
    }
    clean_chain_exit(api);
    pfx_release(&g_pfx_view_ds);
    if (g_pfx_key_hook) { api->hook_remove(api, g_pfx_key_hook); g_pfx_key_hook = 0; }
    g_pfx_enabled = 0;
}
#endif
