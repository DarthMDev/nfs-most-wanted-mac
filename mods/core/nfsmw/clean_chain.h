/* The runner for the rebuilt post-process effects (glow, auto brightness, edge darkening, motion blur, depth of
 * field), written from a functional specification of the technique (downscaled luminance, adaptation, bright pass,
 * blur, bloom; the classic Direct3D HDR approach) and tuned against the PC game's own picture.
 * The runner holds no lighting algorithm: it reads clean-chain.txt from the mod folder (targets, history pairs,
 * constants and an ordered list of single-pass techniques), loads the effect it names (clean-chain.fx, built from
 * effects/clean-chain.hlsl by tools/effects/fxgen.py) and draws the passes in order, setting a complete canonical
 * render state before every pass. Setting `clean_effects` (default on) runs it in the Definitive look.
 * TEST ONLY switch NFSMW_TEST_CLEAN_CHAIN=1 forces the chain on in any look. Included by post_fx.h. */
#ifndef CLEAN_CHAIN_H
#define CLEAN_CHAIN_H

#define CC_MAXT 32   /* targets (16 until 5 Oct: module D's drop needs 17) */
#define CC_MAXP 48   /* passes */
#define CC_MAXS 4    /* samplers s0..s3 per pass */
#define CC_MAXC 8    /* constants per pass */
#define CC_MAXG 16   /* global constants */
#define CC_SRC_NONE  (-1)
#define CC_FRAME     (-2)   /* source: the copy of the finished frame; destination: the frame itself */
#define CC_DEPTH     (-4)   /* source only: the main view's device depth, R32F at frame size (module E; DepthInfo.x says if valid) */
#define CC_FX_MAX    (256 * 1024)

typedef struct {
    char name[24];
    uint32_t fmt;            /* 21 A8R8G8B8, 114 R32F */
    unsigned div, aw, ah;    /* size: frame/div (div > 0) or aw x ah */
    int history;             /* ping-pong pair: .cur (written this frame) and .prev (last frame's) */
    uint32_t tex[2], surf[2];
    unsigned w[2], h[2];
} CcTarget;
typedef struct {
    char tech[32];
    uint32_t htech;
    int dst, dst_prev;                       /* target index or CC_FRAME; dst_prev: .prev of a history (never valid) */
    int src[CC_MAXS], src_prev[CC_MAXS], linear[CC_MAXS];
    uint32_t blend, write;                   /* blend 0 off, 1 add, 2 mul, 3 alpha; write = COLORWRITEENABLE mask */
    uint32_t cparam[CC_MAXC];
    float cval[CC_MAXC][4];
    unsigned nc;
} CcPass;

static int g_cc_env = -1;                    /* 0 unset, 1 forced on (TEST) */
static int g_cc_state;                       /* 0 untried, 1 ready, -1 failed */
static CcTarget g_cc_t[CC_MAXT];
static CcPass g_cc_p[CC_MAXP];
static unsigned g_cc_nt, g_cc_np, g_cc_ng, g_cc_stages;   /* g_cc_stages: bit mask of sampler stages any pass uses */
static uint32_t g_cc_gparam[CC_MAXG];
static float g_cc_gval[CC_MAXG][4];
static uint32_t g_cc_fx, g_cc_mem, g_cc_frame_tex, g_cc_frame_surf;
static unsigned g_cc_frame_w, g_cc_frame_h, g_cc_out_w, g_cc_out_h, g_cc_cur;
static uint32_t g_cc_p_host, g_cc_p_dst, g_cc_p_src[CC_MAXS], g_cc_p_finish;
static uint32_t g_cc_p_expo;
static uint32_t g_cc_p_nearfar, g_cc_p_depthinfo, g_cc_p_vel, g_cc_p_flags, g_cc_p_focus, g_cc_p_motion;   /* module E host inputs */
static uint32_t g_cc_depth_tex, g_cc_depth_surf;
static unsigned g_cc_depth_w, g_cc_depth_h;
static int g_cc_uses_depth;
static uint64_t g_cc_depth_frames, g_cc_depth_missing;
static int g_cc_reset_pending = 1, g_cc_stale;
static uint64_t g_cc_last_ns, g_cc_seen_reseeds, g_cc_seen_recreated;
static uint64_t g_cc_frames, g_cc_resets, g_cc_device_resets, g_cc_target_sets, g_cc_pass_errors;
static char g_cc_fail[160];
static FILE *g_cc_trace;
static int g_cc_trace_t[4] = {-1, -1, -1, -1};   /* up to 4 traced 1x1 r32f targets (list separated by , or +) */
static unsigned g_cc_trace_n;
static uint32_t g_cc_trace_sys;
#define CC_OUT    (g_cc_mem + 0x000u)    /* out pointers; +0x20 D3DLOCKED_RECT */
#define CC_VALS   (g_cc_mem + 0x040u)    /* one float4 */
#define CC_VERTS  (g_cc_mem + 0x080u)    /* 4 x (float4 position, float2 uv) */
#define CC_NAME   (g_cc_mem + 0x100u)    /* one name, 64 bytes */
#define CC_MEM_SIZE 0x200u

static uint32_t cc_name(const char *s) {
    char b[64];
    snprintf(b, sizeof b, "%s", s);
    pfx_put(CC_NAME, b, (uint32_t)strlen(b) + 1u);
    return CC_NAME;
}
static uint32_t cc_param(const char *name) {      /* effect parameter handle or 0 */
    const uint32_t a[] = {0, cc_name(name)};
    const uint32_t h = pfx_call(g_cc_fx, 0x24, a, 2);
    return h == 0x80004005u ? 0 : h;
}
static void cc_set4(uint32_t param, const float v[4]) {
    if (!param) return;
    pfx_put(CC_VALS, v, 16);
    const uint32_t a[] = {param, CC_VALS, 16};
    pfx_call(g_cc_fx, 0x50, a, 3);                                       /* SetValue */
}

/* Creates an fx_2_0 effect from a file in the mod folder through the game's D3DXCreateEffectFromResourceA import,
 * swapping the verified IDI_OVERBRIGHT_FX resource entry for the one call and putting it back (the PC OverBright
 * effect itself is untouched). */
static uint32_t cc_create_effect(const PopModApi *api, uint32_t dev, uint32_t scratch, const char *file, char *why, size_t whyn) {
    char path[2048];
    snprintf(path, sizeof path, "%s/%s", api->mod_dir(api), file);
    FILE *f = fopen(path, "rb");
    if (!f) { snprintf(why, whyn, "%s not found", file); return 0; }
    fseek(f, 0, SEEK_END);
    const long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint32_t bytes = 0;
    void *p = NULL;
    if (size <= 8 || size > CC_FX_MAX || api->guest_alloc(api, (uint32_t)size, &bytes) != POP_OK ||
        api->guest_ptr(api, bytes, (uint32_t)size, &p) != POP_OK || fread(p, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        snprintf(why, whyn, "%s could not be read (%ld bytes)", file, size);
        return 0;
    }
    fclose(f);
    if (get_u32(0x009c7508u) != 0x00614cb8u || get_u32(0x009c750cu) != 5620u) { snprintf(why, whyn, "resource entry not the verified one"); return 0; }
    const char *res = "IDI_OVERBRIGHT_FX";
    pfx_put(scratch + 0x100u, res, (uint32_t)strlen(res) + 1u);
    api->guest_write_u32(api, scratch, 0);
    api->guest_write_u32(api, 0x009c7508u, bytes - 0x00400000u);
    api->guest_write_u32(api, 0x009c750cu, (uint32_t)size);
    const uint32_t args[9] = {dev, 0, scratch + 0x100u, 0, 0, 0, 0, scratch, 0};
    uint32_t r = 0x80004005u;
    const PopModStatus st = api->guest_call(api, get_u32(0x008904c0u), 0, args, 9, &r);
    api->guest_write_u32(api, 0x009c7508u, 0x00614cb8u);
    api->guest_write_u32(api, 0x009c750cu, 5620u);
    const uint32_t fx = get_u32(scratch);
    if (st != POP_OK || r != 0 || !fx) { snprintf(why, whyn, "effect creation from %s failed (%08x)", file, r); return 0; }
    return fx;
}

/* The canonical state of a clean pass (contract 2): every render state the chain saves set to its replacement value
 * (Z, Z write, alpha test, culling, fog, stencil, scissor, sRGB write, separate alpha blend off; blend op ADD), then
 * the pass's blend and colour-write mask. */
static void cc_state(uint32_t dev, uint32_t blend, uint32_t write) {
    for (unsigned i = 0; i < PFX_NRS; ++i) {
        const uint32_t a[] = {k_pfx_rs[i][0], k_pfx_rs[i][1]};
        pfx_call(dev, 0xe4, a, 2);
    }
    static const uint32_t bf[4][3] = {{0, 2, 2}, {1, 2, 2} /* ONE, ONE */, {1, 9, 1} /* DESTCOLOR, ZERO */, {1, 5, 6} /* SRCALPHA, INVSRCALPHA */};
    const uint32_t b = blend < 4 ? blend : 0;
    const uint32_t s[][2] = {{27, bf[b][0]}, {19, bf[b][1]}, {20, bf[b][2]}, {171, 1}, {168, write}};
    for (unsigned i = 0; i < 5; ++i) pfx_call(dev, 0xe4, s[i], 2);
}
/* Sampler state of one stage: clamp, point or linear, no mipmaps, no sRGB read. */
static void cc_sampler(uint32_t dev, unsigned stage, int linear) {
    const uint32_t f = linear ? 2u : 1u;
    const uint32_t s[][2] = {{1, 3}, {2, 3}, {5, f}, {6, f}, {7, 0}, {11, 0}};
    for (unsigned i = 0; i < 6; ++i) {
        const uint32_t a[] = {stage, s[i][0], s[i][1]};
        pfx_call(dev, 0x114, a, 3);
    }
}
/* One full-target quad (contract 2), offset by half a destination pixel. */
static void cc_quad(uint32_t dev, uint32_t verts, unsigned w, unsigned h) {
    const float hx = 1.0f / (float)w, hy = 1.0f / (float)h;
    const float v[4][6] = {{-1.0f - hx, 1.0f + hy, 0.5f, 1.0f, 0.0f, 0.0f}, {1.0f - hx, 1.0f + hy, 0.5f, 1.0f, 1.0f, 0.0f},
                           {-1.0f - hx, -1.0f + hy, 0.5f, 1.0f, 0.0f, 1.0f}, {1.0f - hx, -1.0f + hy, 0.5f, 1.0f, 1.0f, 1.0f}};
    pfx_put(verts, v, sizeof v);
    const uint32_t dp[] = {5 /* TRIANGLESTRIP */, 2, verts, 24};
    pfx_call(dev, 0x14c, dp, 4);                                          /* DrawPrimitiveUP */
}

static void cc_release_targets(void) {
    for (unsigned i = 0; i < g_cc_nt; ++i)
        for (unsigned k = 0; k < 2; ++k) {
            pfx_release(&g_cc_t[i].surf[k]);
            pfx_release(&g_cc_t[i].tex[k]);
            g_cc_t[i].w[k] = g_cc_t[i].h[k] = 0;
        }
    pfx_release(&g_cc_frame_surf);
    pfx_release(&g_cc_frame_tex);
    pfx_release(&g_cc_depth_surf);
    pfx_release(&g_cc_depth_tex);
    g_cc_depth_w = g_cc_depth_h = 0;
    g_cc_frame_w = g_cc_frame_h = 0;
    pfx_release(&g_cc_trace_sys);
    g_cc_out_w = g_cc_out_h = 0;
}

/* ---- clean-chain.txt ---- */
static int cc_fail_load(const char *what, unsigned line) {
    snprintf(g_cc_fail, sizeof g_cc_fail, "clean-chain.txt line %u: %s", line, what);
    return 0;
}
static int cc_find(const char *n) {
    for (unsigned i = 0; i < g_cc_nt; ++i)
        if (!strcmp(g_cc_t[i].name, n)) return (int)i;
    return -1;
}
/* "frame", "NAME", "NAME.cur" or "NAME.prev" -> index (or CC_FRAME), *prev; -3 when unknown or misused. */
static int cc_ref(const char *s, int *prev) {
    *prev = 0;
    if (!strcmp(s, "frame")) return CC_FRAME;
    if (!strcmp(s, "depth")) return CC_DEPTH;
    char n[32];
    snprintf(n, sizeof n, "%s", s);
    char *dot = strchr(n, '.');
    int suffix = 0;
    if (dot) {
        *dot = 0;
        if (!strcmp(dot + 1, "cur")) suffix = 1;
        else if (!strcmp(dot + 1, "prev")) suffix = 2;
        else return -3;
    }
    const int t = cc_find(n);
    if (t < 0) return -3;
    if (g_cc_t[t].history != (suffix != 0)) return -3;   /* a history is always named with .cur/.prev, a target never */
    *prev = suffix == 2;
    return t;
}
static int cc_floats(const char *s, float v[4]) {
    v[0] = v[1] = v[2] = v[3] = 0.0f;
    return sscanf(s, "%f,%f,%f,%f", &v[0], &v[1], &v[2], &v[3]) >= 1;
}
static int cc_parse(const PopModApi *api, char *fxname, size_t fxn) {
    char path[2048];
    snprintf(path, sizeof path, "%s/clean-chain.txt", api->mod_dir(api));
    FILE *f = fopen(path, "r");
    if (!f) { snprintf(g_cc_fail, sizeof g_cc_fail, "clean-chain.txt not found"); return 0; }
    char buf[512];
    unsigned ln = 0;
    int ok = 1;
    fxname[0] = 0;
    while (ok && fgets(buf, sizeof buf, f)) {
        ++ln;
        char *hash = strchr(buf, '#');
        if (hash) *hash = 0;
        char *tok[24];
        unsigned nt = 0;
        for (char *t = strtok(buf, " \t\r\n"); t && nt < 24; t = strtok(NULL, " \t\r\n")) tok[nt++] = t;
        if (!nt) continue;
        if (!strcmp(tok[0], "effect") && nt == 2) {
            snprintf(fxname, fxn, "%s", tok[1]);
        } else if ((!strcmp(tok[0], "target") || !strcmp(tok[0], "history")) && nt == 4) {
            if (g_cc_nt >= CC_MAXT) { ok = cc_fail_load("too many targets for the host runner (CC_MAXT)", ln); break; }
            if (cc_find(tok[1]) >= 0 || strlen(tok[1]) >= sizeof g_cc_t[0].name || strchr(tok[1], '.')) { ok = cc_fail_load("bad or duplicate target name", ln); break; }
            CcTarget *t = &g_cc_t[g_cc_nt];
            memset(t, 0, sizeof *t);
            snprintf(t->name, sizeof t->name, "%s", tok[1]);
            t->history = tok[0][0] == 'h';
            if (!strcmp(tok[2], "argb8")) t->fmt = 21;
            else if (!strcmp(tok[2], "r32f")) t->fmt = 114;
            else { ok = cc_fail_load("format must be argb8 or r32f", ln); break; }
            if (!strcmp(tok[3], "frame")) t->div = 1;
            else if (sscanf(tok[3], "frame/%u", &t->div) == 1 && t->div >= 1 && t->div <= 256) {}
            else if (sscanf(tok[3], "%ux%u", &t->aw, &t->ah) == 2 && t->aw >= 1 && t->ah >= 1 && t->aw <= 4096 && t->ah <= 4096) t->div = 0;
            else { ok = cc_fail_load("size must be frame, frame/N or WxH", ln); break; }
            ++g_cc_nt;
        } else if (!strcmp(tok[0], "const") && nt == 3) {
            if (g_cc_ng >= CC_MAXG) { ok = cc_fail_load("too many constants", ln); break; }
            if (!(g_cc_gparam[g_cc_ng] = cc_param(tok[1]))) { ok = cc_fail_load("constant not in the effect", ln); break; }
            if (!cc_floats(tok[2], g_cc_gval[g_cc_ng])) { ok = cc_fail_load("constant value", ln); break; }
            ++g_cc_ng;
        } else if (!strcmp(tok[0], "pass") && nt >= 4 && !strcmp(tok[2], "->")) {
            if (g_cc_np >= CC_MAXP) { ok = cc_fail_load("too many passes", ln); break; }
            CcPass *p = &g_cc_p[g_cc_np];
            memset(p, 0, sizeof *p);
            snprintf(p->tech, sizeof p->tech, "%s", tok[1]);
            const uint32_t a[] = {cc_name(tok[1])};
            p->htech = pfx_call(g_cc_fx, 0x34, a, 1);                     /* GetTechniqueByName */
            if (!p->htech || p->htech == 0x80004005u) { ok = cc_fail_load("technique not in the effect", ln); break; }
            p->dst = cc_ref(tok[3], &p->dst_prev);
            if (p->dst == -3 || p->dst == CC_DEPTH || p->dst_prev) { ok = cc_fail_load("destination must be frame, a target or HISTORY.cur", ln); break; }
            for (unsigned s = 0; s < CC_MAXS; ++s) p->src[s] = CC_SRC_NONE;
            p->write = p->dst == CC_FRAME ? 7u : 15u;                  /* the frame keeps its alpha */
            for (unsigned i = 4; ok && i < nt; ++i) {
                char *eq = strchr(tok[i], '=');
                if (!eq) { ok = cc_fail_load("option without =", ln); break; }
                *eq = 0;
                const char *k = tok[i], *v = eq + 1;
                unsigned s;
                if (!strcmp(k, "blend")) {
                    if (!strcmp(v, "off")) p->blend = 0; else if (!strcmp(v, "add")) p->blend = 1;
                    else if (!strcmp(v, "mul")) p->blend = 2; else if (!strcmp(v, "alpha")) p->blend = 3;
                    else ok = cc_fail_load("blend must be off, add, mul or alpha", ln);
                } else if (!strcmp(k, "write")) {
                    if (!strcmp(v, "rgba")) p->write = 15; else if (!strcmp(v, "rgb")) p->write = 7;
                    else if (!strcmp(v, "r")) p->write = 1; else if (!strcmp(v, "a")) p->write = 8;
                    else ok = cc_fail_load("write must be rgba, rgb, r or a", ln);
                } else if (k[0] == 's' && sscanf(k + 1, "%u", &s) == 1 && s < CC_MAXS && strlen(k) == 2) {
                    char src[32];
                    snprintf(src, sizeof src, "%s", v);
                    char *colon = strchr(src, ':');
                    p->linear[s] = 0;
                    if (colon) {
                        *colon = 0;
                        if (!strcmp(colon + 1, "linear")) p->linear[s] = 1;
                        else if (strcmp(colon + 1, "point")) { ok = cc_fail_load("filter must be point or linear", ln); break; }
                    }
                    p->src[s] = cc_ref(src, &p->src_prev[s]);
                    if (p->src[s] == -3) { ok = cc_fail_load("unknown source", ln); break; }
                    if (p->src[s] == CC_DEPTH) g_cc_uses_depth = 1;
                    if (p->src[s] >= 0 && p->src[s] == p->dst && p->src_prev[s] == p->dst_prev) { ok = cc_fail_load("a pass reads its own destination", ln); break; }
                    g_cc_stages |= 1u << s;
                } else {
                    if (p->nc >= CC_MAXC) { ok = cc_fail_load("too many pass constants", ln); break; }
                    if (!(p->cparam[p->nc] = cc_param(k))) { ok = cc_fail_load("pass constant not in the effect", ln); break; }
                    if (!cc_floats(v, p->cval[p->nc])) { ok = cc_fail_load("pass constant value", ln); break; }
                    ++p->nc;
                }
            }
            if (ok) ++g_cc_np;
        } else {
            ok = cc_fail_load("unknown line", ln);
        }
        if (!g_cc_fx && fxname[0] && ok) {   /* the effect is needed to resolve techniques and parameters */
            g_cc_fx = cc_create_effect(api, get_u32(0x00982bdcu), CC_OUT, fxname, g_cc_fail, sizeof g_cc_fail);
            if (!g_cc_fx) ok = 0;
        }
        if (ok && !g_cc_fx && strcmp(tok[0], "effect")) ok = cc_fail_load("the effect line must come first", ln);
    }
    fclose(f);
    if (ok && !g_cc_np) { snprintf(g_cc_fail, sizeof g_cc_fail, "clean-chain.txt has no passes"); ok = 0; }
    return ok;
}

static void cc_log_fail(const PopModApi *api) {
    char line[240];
    snprintf(line, sizeof line, "core.nfsmw: effects: off: %s", g_cc_fail);
    api->log(api, line);
    g_cc_state = -1;
}
static int cc_load(const PopModApi *api) {
    if (api->guest_alloc(api, CC_MEM_SIZE, &g_cc_mem) != POP_OK) { snprintf(g_cc_fail, sizeof g_cc_fail, "no guest memory"); return 0; }
    char fx[64];
    if (!cc_parse(api, fx, sizeof fx)) return 0;
    g_cc_p_host = cc_param("HostFrame");
    g_cc_p_finish = cc_param("FinishParams");
    g_cc_p_expo = cc_param("ExposureModel");      /* .w = the auto brightness setting */
    g_cc_p_nearfar = cc_param("nearFar");        /* depth of field / motion blur host inputs */
    g_cc_p_depthinfo = cc_param("DepthInfo");
    g_cc_p_vel = cc_param("cameraVelocityVS");
    g_cc_p_flags = cc_param("cameraFlags");
    g_cc_p_focus = cc_param("CameraFocus");
    g_cc_p_motion = cc_param("MotionSettings");
    g_cc_p_dst = cc_param("DstTexelSize");
    for (unsigned s = 0; s < CC_MAXS; ++s) {
        char n[24];
        snprintf(n, sizeof n, "SrcTexelSize%u", s);
        g_cc_p_src[s] = cc_param(n);
    }
    const char *tt = getenv("NFSMW_TEST_CLEAN_TRACE_TARGET");   /* TEST ONLY: per-frame value of a 1x1 R32F target */
    const char *tf = getenv("NFSMW_TEST_CLEAN_TRACE");
    if (tt && *tt && tf && *tf) {
        char list[128];
        snprintf(list, sizeof list, "%s", tt);
        int ok = 1;
        for (char *n = strtok(list, ",+"); n && g_cc_trace_n < 4; n = strtok(NULL, ",+")) {
            int prev = 0;
            const int t = cc_ref(n, &prev);
            if (t < 0 || g_cc_t[t].fmt != 114 || g_cc_t[t].div || g_cc_t[t].aw != 1 || g_cc_t[t].ah != 1) { ok = 0; break; }
            g_cc_trace_t[g_cc_trace_n++] = t;
        }
        if (!ok || !g_cc_trace_n) {
            g_cc_trace_n = 0;
            api->log(api, "core.nfsmw: TEST clean chain: trace targets must be 1x1 r32f targets (HISTORY.cur for a history); no trace");
        } else if ((g_cc_trace = fopen(tf, "w")) != NULL) {
            fprintf(g_cc_trace, "frame,game_frame,dt,reset,value");   /* value = the first target, value2.. the others */
            for (unsigned k = 1; k < g_cc_trace_n; ++k) fprintf(g_cc_trace, ",value%u", k + 1);
            fprintf(g_cc_trace, "\n");
        }
    }
    char line[240];
    snprintf(line, sizeof line, "core.nfsmw: effects: %s loaded: %u targets, %u passes, %u constants; HostFrame %s, DstTexelSize %s",
             fx, g_cc_nt, g_cc_np, g_cc_ng, g_cc_p_host ? "used" : "unused", g_cc_p_dst ? "used" : "unused");
    api->log(api, line);
    return 1;
}

/* Device Reset (006db0d0, the game's reset of the device, reached from its own request 00982c39 at a safe point): no
 * clean target lives across it; all are made again on the next frame and the history counts as invalid. */
static void cc_device_reset(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)api; (void)cpu; (void)inv; (void)user;
    cc_release_targets();
    g_cc_reset_pending = 1;
    ++g_cc_device_resets;
}

/* 0 current chain, 1 clean chain, 2 neither (the plain frame). */
static int clean_chain_mode(void) {
    if (g_cc_env < 0) {
        const char *e = getenv("NFSMW_TEST_CLEAN_CHAIN");
        g_cc_env = !e || !*e ? 0 : 1;
    }
    int m = 0;
    /* Without the TEST switch, the setting clean_effects (on by default) runs the chain in the Definitive look. If
     * clean-chain.fx/.txt are missing or fail to load, the frame stays the plain picture and the log says why. */
    if (g_cc_env == 0 && g_cc_state >= 0 && g_look == LOOK_HYBRID && setting("clean_effects", 1)) m = g_pfx_live ? 1 : 2;
    if (g_cc_env == 1) m = g_pfx_live ? 1 : 2;
    if (m != 1) g_cc_stale = 1;   /* a frame without the clean chain: its history is stale when it comes back */
    return m;
}

static int cc_make_targets(uint32_t dev, unsigned out_w, unsigned out_h) {
    int made = 0;
    for (unsigned i = 0; i < g_cc_nt; ++i) {
        CcTarget *t = &g_cc_t[i];
        const unsigned w = t->div ? (out_w + t->div - 1) / t->div : t->aw, h = t->div ? (out_h + t->div - 1) / t->div : t->ah;
        for (unsigned k = 0; k < (t->history ? 2u : 1u); ++k) {
            if (t->surf[k] && t->w[k] == w && t->h[k] == h) continue;
            if (!pfx_target(dev, &t->tex[k], &t->surf[k], &t->w[k], &t->h[k], w ? w : 1, h ? h : 1, t->fmt)) {
                snprintf(g_cc_fail, sizeof g_cc_fail, "target %s (%ux%u, format %u) could not be made", t->name, w, h, t->fmt);
                return -1;
            }
            made = 1;
        }
    }
    if (!g_cc_frame_surf || g_cc_frame_w != out_w || g_cc_frame_h != out_h) {
        if (!pfx_target(dev, &g_cc_frame_tex, &g_cc_frame_surf, &g_cc_frame_w, &g_cc_frame_h, out_w, out_h, 21)) {
            snprintf(g_cc_fail, sizeof g_cc_fail, "frame copy could not be made");
            return -1;
        }
        made = 1;
    }
    g_cc_out_w = out_w;
    g_cc_out_h = out_h;
    return made;
}
static void cc_size(int t, int prev, unsigned *w, unsigned *h) {
    if (t == CC_FRAME) { *w = g_cc_frame_w; *h = g_cc_frame_h; return; }
    if (t == CC_DEPTH) { *w = g_cc_depth_w ? g_cc_depth_w : 1u; *h = g_cc_depth_h ? g_cc_depth_h : 1u; return; }
    const unsigned k = g_cc_t[t].history ? (g_cc_cur ^ (unsigned)prev) : 0u;
    *w = g_cc_t[t].w[k];
    *h = g_cc_t[t].h[k];
}
static uint32_t cc_tex(int t, int prev) {
    if (t == CC_FRAME) return g_cc_frame_tex;
    if (t == CC_DEPTH) return g_cc_depth_tex;
    return g_cc_t[t].tex[g_cc_t[t].history ? (g_cc_cur ^ (unsigned)prev) : 0u];
}
static uint32_t cc_surf(int t, int prev, uint32_t bb) {
    if (t == CC_FRAME) return bb;
    return g_cc_t[t].surf[g_cc_t[t].history ? (g_cc_cur ^ (unsigned)prev) : 0u];
}

/* Module E host inputs (contract section 9; PC-side data only). Camera = eView 1 (MB_EVIEW_PLAYER1) +0x40, the PC camera: velocity +0x200 (world units/s), view matrix +0x00 (row-vector, rows
 * 0-2 = rotation), focal distance +0xb4, depth-of-field range +0xb8. nearFar = camera +0xbc / +0xc0: DERIVED
 * (0.5 and 10000 in every logged frame of job 1216; +0xc4/+0xc8, the first guess, hold no floats; centre depth then
 * reconstructs to 3.5-4 m parked behind the car and ~25 m ahead at speed). cameraFlags.y = the drive camera's POV
 * query (mover vtable +0x24): 1 in the near and far chase views, 0 in the hood and bumper views (job 1219, two
 * camera cycles).
 * Depth = the main view's depth surface captured during its colour pass (g_pfx_view_ds, post_fx.h), else the bound
 * one, copied to an R32F target the frame's size. */
static void cc_e_inputs(const PopModApi *api, uint32_t dev, unsigned out_w, unsigned out_h, uint32_t game_frame) {
    int have_depth = 0;
    if (g_cc_uses_depth || g_cc_p_depthinfo) {
        if (pfx_target(dev, &g_cc_depth_tex, &g_cc_depth_surf, &g_cc_depth_w, &g_cc_depth_h, out_w, out_h, 114)) {
            uint32_t ds = 0;
            int borrowed = 0;
            if (g_pfx_view_ds && setting("dof_scene_depth", 1)) { ds = g_pfx_view_ds; borrowed = 1; }
            else { const uint32_t g[] = {CC_OUT}; if (pfx_call(dev, 0xa0, g, 1) == 0) ds = get_u32(CC_OUT); }   /* GetDepthStencilSurface */
            if (ds) {
                const uint32_t d[] = {CC_OUT + 8u};
                if (pfx_call(ds, 0x30, d, 1) == 0 && get_u32(CC_OUT + 8u + 24u) == out_w && get_u32(CC_OUT + 8u + 28u) == out_h) {
                    const uint32_t c[] = {ds, 0, g_cc_depth_surf, 0, 0};
                    have_depth = pfx_call(dev, 0x88, c, 5) == 0;                    /* StretchRect: depth -> R32F */
                }
                if (!borrowed) pfx_release(&ds);
            }
        }
        {   /* TEST ONLY: NFSMW_TEST_CLEAN_NODEPTH=1 reports the depth copy as unavailable (module E fallback test) */
            static int nodepth = -1;
            if (nodepth < 0) nodepth = getenv("NFSMW_TEST_CLEAN_NODEPTH") != NULL;
            if (nodepth) have_depth = 0;
        }
        if (have_depth) ++g_cc_depth_frames; else ++g_cc_depth_missing;
    }
    const float di[4] = {have_depth ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f};
    cc_set4(g_cc_p_depthinfo, di);
    const uint32_t view = 0x009195e0u + 0x70u, cam = get_u32(view + 0x40u);
    uint32_t e_mover_type = 0xffffffffu, e_pov = 0xffffffffu;   /* TEST log only */
    float vel[4] = {0, 0, 0, 0}, nf[4] = {0, 0, 0, 0}, fl[4] = {0, 0, 0, 0}, fo[4] = {0, 0, 0, 0}, ms[4] = {0, 0, 0, 0};
    if (cam) {
        float v[3], m[3][3];
        for (unsigned i = 0; i < 3; ++i) {
            v[i] = get_float(cam + 0x200u + 4u * i);
            for (unsigned j = 0; j < 3; ++j) m[i][j] = get_float(cam + 16u * i + 4u * j);
        }
        for (unsigned j = 0; j < 3; ++j) vel[j] = v[0] * m[0][j] + v[1] * m[1][j] + v[2] * m[2][j];
        vel[3] = sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        nf[0] = get_float(cam + 0xbcu); nf[1] = get_float(cam + 0xc0u);
        fo[0] = get_float(cam + 0xb4u); fo[1] = get_float(cam + 0xb8u);
        fl[0] = (get_u32(0x008f9b28u) & 0xffu) == 0 ? 1.0f : 0.0f;               /* a cut-scene camera is named */
        if (get_u32(view + 0x48u) != view + 0x44u) {                              /* first camera mover: a drive camera in chase mode */
            const uint32_t node = get_u32(view + 0x48u), mover = node ? node - 4u : 0u;
            if (mover) e_mover_type = get_u32(mover + 0x0cu);
            if (mover && get_u32(mover + 0x0cu) == 1u) {
                const uint32_t fn = get_u32(get_u32(mover) + 0x24u);
                uint32_t r = 0;
                if (fn && api->guest_call(api, fn, mover, NULL, 0, &r) == POP_OK) e_pov = r & 0xffu;
                if (e_pov != 0xffffffffu && e_pov) fl[1] = 1.0f;
            }
        }
    }
    {
        const uint32_t vt = get_u32(0x00982af0u);
        ms[0] = setting("motion_blur", 1) ? (float)setting("motion_blur_strength", 10) / 100.0f : 0.0f;
        ms[1] = setting("cinematic_dof", 1) ? (float)setting("dof_strength", 30) / 100.0f : 0.0f;
        ms[2] = vt ? get_float(vt + 0x178u) : 0.0f;                               /* the PC radial blur (NOS, Speedbreaker) */
    }
    {   /* TEST ONLY (module E acceptance): NFSMW_TEST_CLEAN_FOCUS="focal:range" forces CameraFocus,
         * NFSMW_TEST_CLEAN_CUT=1 forces the cut-scene flag. No cut-scene occurs in the test drives. */
        static int tf = -1, tc = -1;
        static float tfo[2];
        if (tf < 0) { const char *e = getenv("NFSMW_TEST_CLEAN_FOCUS"); tf = e && sscanf(e, "%f:%f", &tfo[0], &tfo[1]) == 2; }
        if (tc < 0) tc = getenv("NFSMW_TEST_CLEAN_CUT") != NULL;
        if (tf) { fo[0] = tfo[0]; fo[1] = tfo[1]; }
        if (tc) fl[0] = 1.0f;
    }
    cc_set4(g_cc_p_nearfar, nf); cc_set4(g_cc_p_vel, vel); cc_set4(g_cc_p_flags, fl); cc_set4(g_cc_p_focus, fo); cc_set4(g_cc_p_motion, ms);
    static int elog = -1;   /* TEST ONLY: NFSMW_TEST_CLEAN_E_LOG=1 logs the E inputs every 120 clean frames */
    if (elog < 0) elog = getenv("NFSMW_TEST_CLEAN_E_LOG") != NULL;
    if (elog && g_cc_frames % 120 == 0) {
        char line[384];
        snprintf(line, sizeof line, "core.nfsmw: TEST clean E inputs (game frame %u): depth %d, nearFar %.4g %.4g, velVS %.3f %.3f %.3f |v| %.3f, flags cut %.0f pov %.0f (mover type %d, query %d), focus %.4g %.4g, settings %.2f %.2f radial %.3f; cam+0xbc..0xd0 %.4g %.4g %.4g %.4g %.4g %.4g",
                 game_frame, have_depth, (double)nf[0], (double)nf[1], (double)vel[0], (double)vel[1], (double)vel[2], (double)vel[3],
                 (double)fl[0], (double)fl[1], (int)e_mover_type, (int)e_pov, (double)fo[0], (double)fo[1], (double)ms[0], (double)ms[1], (double)ms[2],
                 cam ? (double)get_float(cam + 0xbcu) : 0.0, cam ? (double)get_float(cam + 0xc0u) : 0.0, cam ? (double)get_float(cam + 0xc4u) : 0.0,
                 cam ? (double)get_float(cam + 0xc8u) : 0.0, cam ? (double)get_float(cam + 0xccu) : 0.0, cam ? (double)get_float(cam + 0xd0u) : 0.0);
        api->log(api, line);
    }
}

/* Called by pfx_frame_body inside its save/restore of render target, viewport, stage-0 texture and sampler state,
 * render states and vertex declaration (the quad declaration is set). Stages 1-3 are saved and restored here. */
static void clean_chain_run(const PopModApi *api, uint32_t dev, uint32_t bb, unsigned out_w, unsigned out_h, uint32_t game_frame) {
    if (g_cc_state < 0) return;
    if (!g_cc_state) {
        if (!cc_load(api)) { cc_log_fail(api); return; }
        g_cc_state = 1;
    }
    int reset = g_cc_reset_pending || g_cc_stale || g_pfx_reseeds != g_cc_seen_reseeds;
    const int made = cc_make_targets(dev, out_w, out_h);
    if (made < 0) { cc_release_targets(); cc_log_fail(api); return; }
    if (made) {
        reset = 1;
        ++g_cc_target_sets;
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: effects: targets made for a %ux%u frame (set %llu)", out_w, out_h,
                 (unsigned long long)g_cc_target_sets);
        api->log(api, line);
    }
    g_cc_reset_pending = g_cc_stale = 0;
    g_cc_seen_reseeds = g_pfx_reseeds;
    const uint64_t now = pfx_now_ns();
    float dt = g_cc_last_ns ? (float)((double)(now - g_cc_last_ns) / 1e9) : 0.0f;
    if (dt > 0.1f) dt = 0.1f;   /* a stall or a pause is not a tenth of a second of adaptation more */
    g_cc_last_ns = now;
    if (reset) ++g_cc_resets;

    const uint32_t sr[] = {bb, 0, g_cc_frame_surf, 0, 0 /* NONE */};
    if (pfx_call(dev, 0x88, sr, 5) != 0) { ++g_cc_pass_errors; return; }  /* StretchRect frame -> copy */
    const float host[4] = {dt, reset ? 1.0f : 0.0f, (float)out_w, (float)out_h};
    cc_set4(g_cc_p_host, host);
    for (unsigned i = 0; i < g_cc_ng; ++i) cc_set4(g_cc_gparam[i], g_cc_gval[i]);
    for (unsigned i = 0; g_cc_p_expo && i < g_cc_ng; ++i)   /* setting hybrid_exposure (auto brightness %) */
        if (g_cc_gparam[i] == g_cc_p_expo) {
            float e[4] = {g_cc_gval[i][0], g_cc_gval[i][1], g_cc_gval[i][2], (float)setting("hybrid_exposure", 50) / 100.0f};
            cc_set4(g_cc_p_expo, e);
        }
    if (g_cc_p_finish) {   /* host values override the manifest's neutral defaults: the settings + the PC request */
        const uint32_t vt = get_u32(0x00982af0u);                 /* the PC visual treatment object */
        float ev = vt ? get_float(vt + 0x17cu) : 0.0f;            /* its effect-vignette amount */
        if (!(ev > 0.0f)) ev = 0.0f;
        if (ev > 1.0f) ev = 1.0f;
        static int zov = -2;   /* TEST ONLY: NFSMW_TEST_CLEAN_FINISH_Z forces z (the white-edge path never occurs in the fixtures) */
        static float zval;
        if (zov == -2) { const char *e = getenv("NFSMW_TEST_CLEAN_FINISH_Z"); zov = e && *e; if (zov) zval = (float)atof(e); }
        if (zov) ev = zval;
        const float fp[4] = {(float)setting("hybrid_vignette", 0) / 100.0f, (float)setting("hybrid_shadows", 0) / 100.0f, ev, 0.0f};
        cc_set4(g_cc_p_finish, fp);
        static float last[3] = {-1.0f, -1.0f, -1.0f};
        if (fp[0] != last[0] || fp[1] != last[1] || (fp[2] > 0.0f) != (last[2] > 0.0f)) {
            last[0] = fp[0]; last[1] = fp[1]; last[2] = fp[2];
            char line[200];
            snprintf(line, sizeof line, "core.nfsmw: effects: FinishParams vignette %.2f, shadows %.2f, PC effect vignette %.3f (game frame %u)",
                     (double)fp[0], (double)fp[1], (double)fp[2], game_frame);
            api->log(api, line);
        }
    }

    cc_e_inputs(api, dev, out_w, out_h, game_frame);   /* module E host inputs (depth copy + camera data) */
    /* stages 1-3: save texture and sampler state (stage 0 is the chain's) */
    uint32_t stex[CC_MAXS] = {0}, sss[CC_MAXS][6];
    static const uint32_t sst[6] = {1, 2, 5, 6, 7, 11};
    for (unsigned s = 1; s < CC_MAXS; ++s) {
        if (!(g_cc_stages & (1u << s))) continue;
        const uint32_t a[] = {s, CC_OUT};
        if (pfx_call(dev, 0x100, a, 2) == 0) stex[s] = get_u32(CC_OUT);   /* GetTexture */
        for (unsigned k = 0; k < 6; ++k) {
            const uint32_t g[] = {s, sst[k], CC_OUT};
            sss[s][k] = pfx_call(dev, 0x110, g, 3) == 0 ? get_u32(CC_OUT) : 0;
        }
    }
    for (unsigned i = 0; i < g_cc_np; ++i) {
        const CcPass *p = &g_cc_p[i];
        unsigned dw, dh;
        cc_size(p->dst, 0, &dw, &dh);
        cc_state(dev, p->blend, p->write);
        const uint32_t rt[] = {0, cc_surf(p->dst, 0, bb)};
        if (pfx_call(dev, 0x94, rt, 2) != 0) { ++g_cc_pass_errors; continue; }   /* SetRenderTarget (sets the viewport) */
        for (unsigned s = 0; s < CC_MAXS; ++s) {
            if (!(g_cc_stages & (1u << s))) continue;
            const uint32_t tx = p->src[s] == CC_SRC_NONE ? 0u : cc_tex(p->src[s], p->src_prev[s]);
            const uint32_t a[] = {s, tx};
            pfx_call(dev, 0x104, a, 2);                                    /* SetTexture (null where unused) */
            if (tx) cc_sampler(dev, s, p->linear[s]);
            if (tx && g_cc_p_src[s]) {
                unsigned sw, sh;
                cc_size(p->src[s], p->src_prev[s], &sw, &sh);
                const float ts[4] = {1.0f / (float)sw, 1.0f / (float)sh, (float)sw, (float)sh};
                cc_set4(g_cc_p_src[s], ts);
            }
        }
        const float ds[4] = {1.0f / (float)dw, 1.0f / (float)dh, (float)dw, (float)dh};
        cc_set4(g_cc_p_dst, ds);
        for (unsigned k = 0; k < p->nc; ++k) cc_set4(p->cparam[k], p->cval[k]);
        const uint32_t te[] = {p->htech};
        pfx_call(g_cc_fx, 0xe8, te, 1);                                   /* SetTechnique */
        const uint32_t b[] = {CC_OUT + 0x3cu, 0};
        if (pfx_call(g_cc_fx, 0xfc, b, 2) != 0) { ++g_cc_pass_errors; continue; }   /* Begin */
        const uint32_t pz[] = {0};
        pfx_call(g_cc_fx, 0x100, pz, 1);                                  /* BeginPass */
        cc_quad(dev, CC_VERTS, dw, dh);
        pfx_call(g_cc_fx, 0x108, NULL, 0);                                /* EndPass */
        pfx_call(g_cc_fx, 0x10c, NULL, 0);                                /* End */
    }
    for (unsigned s = 1; s < CC_MAXS; ++s) {
        if (!(g_cc_stages & (1u << s))) continue;
        const uint32_t a[] = {s, stex[s]};
        pfx_call(dev, 0x104, a, 2);
        pfx_release(&stex[s]);
        for (unsigned k = 0; k < 6; ++k) {
            const uint32_t v[] = {s, sst[k], sss[s][k]};
            pfx_call(dev, 0x114, v, 3);
        }
    }
    if (g_cc_trace && g_cc_trace_n) {   /* TEST ONLY: blocking readbacks (distort cost: never in cost runs) */
        if (!g_cc_trace_sys) {
            const uint32_t a[] = {1, 1, 114, 2 /* SYSTEMMEM */, CC_OUT, 0};
            if (pfx_call(dev, 0x90, a, 6) == 0) g_cc_trace_sys = get_u32(CC_OUT);   /* CreateOffscreenPlainSurface */
        }
        fprintf(g_cc_trace, "%llu,%u,%.5f,%d", (unsigned long long)g_cc_frames, game_frame, (double)dt, reset);
        for (unsigned k = 0; k < g_cc_trace_n; ++k) {
            const int t = g_cc_trace_t[k];
            float val = -1.0f;
            const uint32_t g[] = {g_cc_t[t].surf[g_cc_t[t].history ? g_cc_cur : 0u], g_cc_trace_sys};
            if (g_cc_trace_sys && pfx_call(dev, 0x80, g, 2) == 0) {                 /* GetRenderTargetData */
                const uint32_t lk[] = {CC_OUT + 0x20u, 0, 0x10u /* READONLY */};
                if (pfx_call(g_cc_trace_sys, 0x34, lk, 3) == 0) {                   /* LockRect */
                    const uint32_t bits = get_u32(CC_OUT + 0x24u);
                    if (bits) val = get_float(bits);
                    pfx_call(g_cc_trace_sys, 0x38, NULL, 0);                        /* UnlockRect */
                }
            }
            fprintf(g_cc_trace, ",%.6g", (double)val);
        }
        fprintf(g_cc_trace, "\n");
    }
    g_cc_cur ^= 1u;   /* this frame's .cur is next frame's .prev */
    if (++g_cc_frames == 1) api->log(api, "core.nfsmw: effects: first frame drawn");
}

static int clean_chain_init(const PopModApi *api) {
    clean_chain_mode();
    if (!g_cc_env) return 0;
    if (!g_hooks[104] &&   /* slot 104 (hook_slot_free is defined after this file) */
        api->hook_install(api, 0x006db0d0u, cc_device_reset, POP_HOOK_BEFORE, NULL, &g_hooks[104]) == POP_OK)
        api->log(api, "core.nfsmw: TEST clean chain: forced on");
    else
        api->log(api, "core.nfsmw: TEST clean chain: device reset hook failed; targets are still made again on size changes");
    return 1;
}
static void clean_chain_exit(const PopModApi *api) {
    if (!g_cc_env) return;
    char line[320];
    if (g_cc_uses_depth || g_cc_p_depthinfo) {
        snprintf(line, sizeof line, "core.nfsmw: TEST clean chain: depth for module E: %llu frames with the view's depth, %llu without",
                 (unsigned long long)g_cc_depth_frames, (unsigned long long)g_cc_depth_missing);
        api->log(api, line);
    }
    snprintf(line, sizeof line, "core.nfsmw: TEST clean chain: %llu frames, %llu history resets, %llu device resets, %llu target sets, %llu pass errors%s%s",
             (unsigned long long)g_cc_frames, (unsigned long long)g_cc_resets, (unsigned long long)g_cc_device_resets,
             (unsigned long long)g_cc_target_sets, (unsigned long long)g_cc_pass_errors, g_cc_state < 0 ? "; failed: " : "",
             g_cc_state < 0 ? g_cc_fail : "");
    api->log(api, line);
    if (g_cc_trace) { fclose(g_cc_trace); g_cc_trace = NULL; }
    g_cc_env = 0;
}
#endif
