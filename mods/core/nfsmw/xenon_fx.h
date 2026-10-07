/* Sparks and light trails (XenonEffects): contrails behind fast cars and sparks from the game's own spark emitters, in
 * the style of the console version's particles. Switch 'xbox_particles' (default on, live; Definitive and
 * PC - Original alike: it adds effects, it changes no look). The particle maths is xenon_particles.h (matched exactly
 * to NFSMW_XenonEffects' asm under emulation); this file connects it to speed.exe 1.3.
 *
 * Reference implementation: NFSMW_XenonEffects by xan1242 (MIT, commit b5070080), which targets this executable
 * (RELOADED 1.3, md5 c0516b48, = our pinned exe). Its hook points and guest functions are REFERENCE unless marked
 * otherwise; the stubs it replaces (004fb000 returns 0, 006c3a40 returns) are PROVEN from our exe's listing.
 * Reimplemented in C; no code copied. Data: the player's GLOBAL\XenonEffects.tpk from that mod (texture "MAIN"), loaded
 * from <mod>/xenon/ through the overlay when present (no file: the feature logs and stays off).
 *
 * Deliberate differences from the reference (operating standard 10: best native behaviour):
 *  - dt accumulates over all simulation updates since the last render (the reference keeps only the last one, so
 *    particles aged slower whenever the render rate fell below the update rate);
 *  - the emission limiters run on time (1/30 s contrails per car, 1/60 s sparks) instead of frame counts, so 60 and
 *    120 fps emit alike; contrails are limited per car (the reference shared one limiter between all cars);
 *  - the per-car contrail flag lives in a table here (the reference grew CarRenderConn by 8 bytes, 0075e6fc/0075e766);
 *  - the contrail and spark sites are wrap hooks around the functions the reference patched inside (caves at
 *    00750f48 and 0050a5d7); their conditions are re-evaluated after the call (see each hook);
 *  - the particle list is cleared when the world is left (the reference kept particles across restarts);
 *  - NOS starts contrails at any speed, as the shared MW source does (the reference only in its optional Carbon mode). */
#ifndef XENON_FX_H
#define XENON_FX_H
#include "xenon_particles.h"

/* speed.exe 1.3 addresses (REFERENCE: XenonEffects dllmain.cpp unless marked) */
#define XFX_GAMEFLOW            0x00925e90u  /* 6 = in the world (PROVEN: used by the core mod since Test68) */
#define XFX_NIS_INSTANCE        0x009885c8u  /* nonzero during an in-engine cut-scene */
#define XFX_SHADER_WORLDPRELIT  0x0093debcu  /* shader object; +0x48 its ID3DXEffect */
#define XFX_SHADER_CURRENT      0x00982c80u
#define XFX_PARTICLE_MATRIX     0x00987ab0u
#define XFX_EVIEW_MAIN          0x00919650u  /* the eView EmitterSystem::Render draws (PROVEN: pushed at 006df4b1) */
#define XFX_DEVICE              0x00982bdcu  /* IDirect3DDevice9* (PROVEN: core mod) */
/* guest functions (calling conventions as the reference calls them) */
#define XFX_ATTRIB_INSTANCE     0x00452380u  /* thiscall Instance(collection, msgPort, ucomlist): MW instance 0x14 bytes, data at +8 */
#define XFX_ATTRIB_REFSPEC_INST 0x00456cb0u  /* thiscall Instance(const RefSpec *, msgPort, ucomlist) */
#define XFX_ATTRIB_DTOR         0x0045a430u  /* thiscall ~Instance() */
#define XFX_ATTRIB_GET          0x004546c0u  /* thiscall Get(Attribute *out, hash) -> Attribute * */
#define XFX_ATTRIB_LENGTH       0x00452d40u  /* thiscall Attribute::GetLength() */
#define XFX_ATTRIB_POINTER      0x00454810u  /* thiscall GetAttributePointer(hash, index) -> data or 0 */
#define XFX_ATTRIB_DEFAULT      0x006269b0u  /* cdecl DefaultDataArea(size) */
#define XFX_REFSPEC_COLLECTION  0x004560d0u  /* thiscall RefSpec::GetCollection() */
#define XFX_FIND_COLLECTION     0x00455fd0u  /* cdecl FindCollection(class, key) */
#define XFX_STRINGHASH32        0x004519d0u  /* cdecl Attrib::StringHash32(const char *) */
#define XFX_BSTRINGHASH         0x00460bf0u  /* cdecl bStringHash(const char *) */
#define XFX_GET_TEXTUREINFO     0x00503400u  /* cdecl GetTextureInfo(hash, default_if_missing, include_unloaded) */
#define XFX_SET_TEXTURE         0x006c68b0u  /* cdecl (TextureInfo *, stage) */
#define XFX_PARTICLE_TRANSFORM  0x006c8000u  /* cdecl (D3DXMATRIX *, view id) */
#define XFX_CHECK_HIT_WORLD     0x007854b0u  /* thiscall WCollisionMgr::CheckHitWorld(seg, cinfo, prim mask) -> bool */
#define XFX_CINFO_CTOR          0x004048c0u  /* fastcall (ecx) WorldCollisionInfo constructor */
#define XFX_CREATE_RESFILE      0x0065fd30u  /* cdecl CreateResourceFile(name, type, 0, 0, 0) */
#define XFX_RESFILE_BEGIN       0x006616f0u  /* thiscall BeginLoading(callback, param) */
#define XFX_SERVICE_RESOURCES   0x006626b0u  /* cdecl ServiceResourceLoading() */
/* hooked functions and the call sites they are filtered to */
#define XFX_UPDATE_PARTICLES    0x00508c30u  /* EmitterSystem::UpdateParticles(float dt), thiscall; call site 0050d43c */
#define XFX_UPDATE_RET          0x0050d441u
#define XFX_GLOBAL_STUB         0x004fb000u  /* returns 0 (PROVEN); called in LoadGlobalChunks at 006648bc */
#define XFX_GLOBAL_STUB_RET     0x006648c1u
#define XFX_EMITTERS_RENDER     0x00503d00u  /* EmitterSystem::Render(eView *), thiscall; call site 006df4bb */
#define XFX_EMITTERS_RENDER_RET 0x006df4c0u
#define XFX_EMITTER_SPAWN       0x00509b30u  /* Emitter::SpawnParticles(float, float), thiscall, ret 8 (PROVEN: listing) */
#define XFX_CAR_ONRENDER        0x00750e20u  /* CarRenderConn::OnRender(eView *, ?), thiscall, ret 8 (PROVEN: listing) */
#define XFX_CAR_ENGINE_ANIM     0x00745f20u  /* CarRenderConn::UpdateEngineAnimation(float, PktCarService *); site 00756629 */
#define XFX_CAR_ENGINE_ANIM_RET 0x0075662eu
/* attribute keys (REFERENCE) */
#define XFX_KEY_NGEMITTER       0xb0d98a89u  /* fuelcell_effect: list of NGEmitter refs */
#define XFX_KEY_XENON_FX        0xfe40e637u  /* emitter: list of NGEffect refs (sparks) */
#define XFX_KEY_SPIN_A          0x28638d89u
#define XFX_KEY_SPIN_B          0xd2603865u
#define XFX_KEY_SPIN_C          0xe2cc8106u
#define XFX_CONTRAIL_CLASS      0x6f5943f1u  /* FindCollection(class, key) of the contrail effect */
#define XFX_CONTRAIL_KEY        0x16afde7bu
/* structure fields (REFERENCE unless marked) */
#define XFX_EMITTER_ATTRIB      0x7cu   /* Emitter: its Attrib instance (MW layout) */
#define XFX_EMITTER_PIGGYBACK   0x8cu   /* Emitter: the AcidEffect it belongs to */
#define XFX_EMITTER_MATRIX      0x20u   /* Emitter: world matrix */
#define XFX_EMITTER_VELOCITY    0x60u
#define XFX_EMITTER_START       0x70u   /* Emitter: active from (PROVEN: 00509b72) */
#define XFX_EMITTER_END         0x74u   /* Emitter: active to (PROVEN: 00509b61) */
#define XFX_PIGGYBACK_FLAGS     0x18u   /* AcidEffect flags; bit 4 = draw its NGEffect effects */
#define XFX_CAR_MATRIX_PTR      0x34u   /* CarRenderConn: pointer to the car matrix */
#define XFX_CAR_VELOCITY_PTR    0x38u   /* CarRenderConn: pointer to the velocity */
#define XFX_CAR_FLAGS           0x3f8u  /* CarRenderConn: bit 2 = contrail-capable state */
#define XFX_PKT_NOS             0x71u   /* PktCarService: NOS active (byte), as XenonEffects reads it */
#define XFX_EVIEW_ID            0x04u
#define XFX_EVIEW_TYPE          0x08u   /* the reference's cave tests 1 or 2 here */
#define XFX_EVIEW_MOVERS        0x44u   /* camera-mover list head; first node at +0x48, mover = node - 4 */
#define XFX_MOVER_VT_0X24       0x24u   /* camera mover vtable +0x24, thiscall, no args, bool: PROVEN that the PC's own OnRender
                                         * calls it on the first mover (00750ee8); name unresolved (OutsidePOV per the dbalatoni
                                         * render path, RenderCarPOV per its header order): neutral name (standard 5) */
#define XFX_COLLECTION_KEY      0x20u   /* collection +0x20: its key (the elasticity table's key) */

#define XFX_MAX_PARTICLES 10000u   /* the reference's list size */
#define XFX_MAX_EFFECTS   500u
#define XFX_CONTRAIL_SPEED 44.0f        /* the reference's contrail speed */

typedef struct {
    float intensity;
    float vel[4];
    float pos[4];        /* the matrix's position row; the rotation is identity, as in the reference */
    uint32_t spec, piggyback;
} XfxEffect;
typedef struct { uint32_t car; double last; int on; } XfxCar;

static XenonParticle g_xfx_parts[XFX_MAX_PARTICLES];
static uint32_t g_xfx_count, g_xfx_seed = 0xdeadbeefu;   /* the reference's seed */
static XfxEffect g_xfx_queue[XFX_MAX_EFFECTS];
static uint32_t g_xfx_queued;
static XfxCar g_xfx_cars[16];
static float g_xfx_dt;
static double g_xfx_clock = 1.0, g_xfx_spark_last;   /* simulated seconds (the update hook's dt), for the limiters */
static unsigned g_xfx_tex_tries;
static uint32_t g_xfx_mem, g_xfx_verts, g_xfx_index, g_xfx_texinfo, g_xfx_elastic_key[2];
static uint32_t g_xfx_decl, g_xfx_decl_dev;   /* our vertex declaration and the device it was made on */
static int g_xfx_on, g_xfx_tpk, g_xfx_tpk_loaded, g_xfx_ready, g_xfx_broken, g_xfx_was_world;
static uint64_t g_xfx_frames, g_xfx_effects, g_xfx_dropped, g_xfx_spawned, g_xfx_contrails, g_xfx_sparks, g_xfx_hits;
static uint32_t g_xfx_spark_cache_inst[64], g_xfx_spark_cache_coll[64], g_xfx_spark_cache_n[64];
/* guest scratch, one block */
#define XFX_SEG       (g_xfx_mem + 0x000u)   /* 2 x float4 */
#define XFX_CINFO     (g_xfx_mem + 0x040u)   /* WorldCollisionInfo, 0x80 reserved */
#define XFX_MGR       (g_xfx_mem + 0x0c0u)   /* WCollisionMgr {surface exclusion 0, primitive mask 3} */
#define XFX_INST_FX   (g_xfx_mem + 0x100u)   /* Attrib instances, 0x20 each */
#define XFX_INST_EM   (g_xfx_mem + 0x120u)
#define XFX_INST_UV   (g_xfx_mem + 0x140u)
#define XFX_ATTR_OUT  (g_xfx_mem + 0x160u)
#define XFX_STR       (g_xfx_mem + 0x180u)   /* 0x80 for names */
#define XFX_DECL_ELEMS (g_xfx_mem + 0x200u)  /* 4 x D3DVERTEXELEMENT9 (8 bytes each) */
#define XFX_DECL_OUT  (g_xfx_mem + 0x220u)   /* CreateVertexDeclaration's out pointer */
#define XFX_DECL_PREV (g_xfx_mem + 0x224u)   /* GetVertexDeclaration's out pointer */
#define XFX_MEM_SIZE  0x240u

static uint32_t xfx_call(uint32_t fn, uint32_t ecx, const uint32_t *args, uint32_t n) {
    uint32_t r = 0;
    if (g_api->guest_call(g_api, fn, ecx, args, n, &r) != POP_OK) { g_xfx_broken = 1; return 0; }
    return r;
}
/* A COM method (ID3DXEffect, IDirect3DDevice9): stdcall with `this` as the first stack argument, not in ecx
 * (ABI audit, 2 Oct: the first draft passed it in ecx). */
static uint32_t xfx_com(uint32_t obj, uint32_t off, const uint32_t *args, uint32_t n) {
    uint32_t a[10];
    if (!obj || n > 9) { g_xfx_broken = 1; return 0x80004005u; }
    a[0] = obj;
    for (uint32_t i = 0; i < n; ++i) a[i + 1] = args[i];
    return xfx_call(get_u32(get_u32(obj) + off), 0, a, n + 1);
}
static uint32_t xfx_str(const char *s) {
    void *p = NULL;
    const size_t n = strlen(s) + 1;
    if (n > 0x80 || g_api->guest_ptr(g_api, XFX_STR, (uint32_t)n, &p) != POP_OK || !p) return 0;
    memcpy(p, s, n);
    return XFX_STR;
}
static void xfx_read(uint32_t addr, void *out, uint32_t n) {
    void *p = NULL;
    if (g_api->guest_ptr(g_api, addr, n, &p) == POP_OK && p) memcpy(out, p, n); else memset(out, 0, n);
}

/* the world query for xp_collision_time: CheckHitWorld on the segment, the hit point and normal from the info */
static int xfx_hit(void *user, const float a[4], const float b[4], float point[4], float normal[4]) {
    (void)user;
    float seg[8];
    memcpy(seg, a, 16);
    memcpy(seg + 4, b, 16);
    void *p = NULL;
    if (g_api->guest_ptr(g_api, XFX_SEG, sizeof seg, &p) != POP_OK || !p) return 0;
    memcpy(p, seg, sizeof seg);
    xfx_call(XFX_CINFO_CTOR, XFX_CINFO, NULL, 0);
    g_api->guest_write_u32(g_api, XFX_MGR, 0);
    g_api->guest_write_u32(g_api, XFX_MGR + 4u, 3);
    const uint32_t args[3] = {XFX_SEG, XFX_CINFO, 3};
    if (!(xfx_call(XFX_CHECK_HIT_WORLD, XFX_MGR, args, 3) & 0xffu)) return 0;
    xfx_read(XFX_CINFO, point, 16);
    xfx_read(XFX_CINFO + 0x10u, normal, 16);
    ++g_xfx_hits;
    return 1;
}

/* AddXenonEffect: queue one effect (the reference's XenonEffectDef) */
static void xfx_add(float intensity, uint32_t spec, uint32_t matrix, uint32_t vel, uint32_t piggyback) {
    if (g_xfx_queued >= XFX_MAX_EFFECTS) { ++g_xfx_dropped; return; }
    XfxEffect *e = &g_xfx_queue[g_xfx_queued++];
    e->intensity = intensity;
    xfx_read(vel, e->vel, 16);
    xfx_read(matrix + 0x30u, e->pos, 16);
    e->spec = spec;
    e->piggyback = piggyback;
    ++g_xfx_effects;
}

/* the MW fuelcell_emitter data (as the reference's bridge reads it) into the Carbon layout xp_spawn takes */
static void xfx_emitter_data(uint32_t mw, uint32_t collection, XenonEmitterData *d) {
    uint8_t raw[0x94];
    xfx_read(mw, raw, sizeof raw);
    memset(d, 0, sizeof *d);
    memcpy(d, raw, 0x80);                       /* same fields up to GravityDelta */
    memcpy(&d->length_start, raw + 0x80, 4);
    memcpy(&d->length_delta, raw + 0x84, 4);
    memcpy(&d->life_variance, raw + 0x88, 4);
    memcpy(&d->num_particles, raw + 0x8c, 4);
    d->contrail = raw[0x93];
    const uint32_t key = collection ? get_u32(collection + XFX_COLLECTION_KEY) : 0;
    d->elasticity = key && key == g_xfx_elastic_key[0] ? 160.0f : key && key == g_xfx_elastic_key[1] ? 120.0f : 0.0f;
    /* Ledger L3 open item: does any MW emitter carry a nonzero NumParticlesVariance (MW +0x70)? One line per
     * distinct emitter collection key, at most 64, so the shipped game data answers it in any run with particles on. */
    static uint32_t seen[64];
    static unsigned n_seen;
    if (key && n_seen < 64u) {
        for (unsigned i = 0; i < n_seen; ++i) if (seen[i] == key) return;
        seen[n_seen++] = key;
        char line[200];
        snprintf(line, sizeof line, "core.nfsmw: sparks and light trails: emitter %08x: NumParticles %.3f, NumParticlesVariance %.4f, "
                 "Life %.3f, LifeVariance %.3f, contrail %u", key, (double)d->num_particles, (double)d->num_particles_variance,
                 (double)d->life, (double)d->life_variance, (unsigned)d->contrail);
        g_api->log(g_api, line);
    }
}
static uint32_t xfx_instance(uint32_t inst, uint32_t ctor, uint32_t what, uint32_t default_size) {
    for (unsigned i = 0; i < 0x20u; i += 4) g_api->guest_write_u32(g_api, inst + i, 0);
    const uint32_t a[3] = {what, 0, 0};
    xfx_call(ctor, inst, a, 3);
    uint32_t data = get_u32(inst + 8u);
    if (!data) {
        const uint32_t s[1] = {default_size};
        data = xfx_call(XFX_ATTRIB_DEFAULT, 0, s, 1);
        g_api->guest_write_u32(g_api, inst + 8u, data);
    }
    return data;
}
static uint32_t xfx_list_length(uint32_t inst, uint32_t key) {
    const uint32_t a[2] = {XFX_ATTR_OUT, key};
    const uint32_t attr = xfx_call(XFX_ATTRIB_GET, inst, a, 2);
    return attr ? xfx_call(XFX_ATTRIB_LENGTH, attr, NULL, 0) : 0;
}
static uint32_t xfx_ref_collection(uint32_t inst, uint32_t key, uint32_t i) {
    const uint32_t a[2] = {key, i};
    uint32_t ref = xfx_call(XFX_ATTRIB_POINTER, inst, a, 2);
    if (!ref) {
        const uint32_t s[1] = {0x0c};
        ref = xfx_call(XFX_ATTRIB_DEFAULT, 0, s, 1);
    }
    return ref ? xfx_call(XFX_REFSPEC_COLLECTION, ref, NULL, 0) : 0;
}
static uint8_t xfx_byte_attr(uint32_t inst, uint32_t key) {
    const uint32_t a[2] = {key, 0};
    const uint32_t p = xfx_call(XFX_ATTRIB_POINTER, inst, a, 2);
    return p ? (uint8_t)(get_u32(p) & 0xffu) : 0;
}

/* NGEffect: every NGEmitter of the effect spawns its share of particles for this step */
static void xfx_expand(const XfxEffect *e, float dt) {
    if (e->piggyback && !((get_u32(e->piggyback + XFX_PIGGYBACK_FLAGS) >> 4) & 1u)) return;
    if (!xfx_instance(XFX_INST_FX, XFX_ATTRIB_INSTANCE, e->spec, 1)) return;
    const uint32_t n = xfx_list_length(XFX_INST_FX, XFX_KEY_NGEMITTER);
    for (uint32_t i = 0; i < n && !g_xfx_broken; ++i) {
        const uint32_t coll = xfx_ref_collection(XFX_INST_FX, XFX_KEY_NGEMITTER, i);
        const uint32_t mw = xfx_instance(XFX_INST_EM, XFX_ATTRIB_INSTANCE, coll, 0x98);
        if (mw) {
            const uint32_t uvp = xfx_instance(XFX_INST_UV, XFX_ATTRIB_REFSPEC_INST, mw + 0x60u, 0x10);
            XenonEmitterData d;
            xfx_emitter_data(mw, coll, &d);
            XenonEmitterUV uv;
            xfx_read(uvp, &uv, sizeof uv);
            XenonEmitterIn in;
            memset(&in, 0, sizeof in);
            in.d = &d;
            in.uv = &uv;
            for (int k = 0; k < 3; ++k) in.inherit[k] = e->vel[k];
            in.local_world[0] = in.local_world[5] = in.local_world[10] = 1.0f;
            for (int k = 0; k < 4; ++k) in.local_world[12 + k] = e->pos[k];
            in.rot[0] = xfx_byte_attr(XFX_INST_EM, XFX_KEY_SPIN_A);
            in.rot[1] = xfx_byte_attr(XFX_INST_EM, XFX_KEY_SPIN_B);
            in.rot[2] = xfx_byte_attr(XFX_INST_EM, XFX_KEY_SPIN_C);
            g_xfx_spawned += e->piggyback
                ? xp_spawn(&in, dt, 1.0f, 0, g_xfx_parts, &g_xfx_count, XFX_MAX_PARTICLES, &g_xfx_seed, xfx_hit, NULL)
                : xp_spawn(&in, dt, xp_contrail_intensity(e->vel), 1, g_xfx_parts, &g_xfx_count, XFX_MAX_PARTICLES, &g_xfx_seed, xfx_hit, NULL);
            xfx_call(XFX_ATTRIB_DTOR, XFX_INST_UV, NULL, 0);
        }
        xfx_call(XFX_ATTRIB_DTOR, XFX_INST_EM, NULL, 0);
    }
    xfx_call(XFX_ATTRIB_DTOR, XFX_INST_FX, NULL, 0);
}

/* Our own vertex declaration for XenonVertex {float3 position, D3DCOLOR, float2 uv} = 24 bytes (2 Oct). The reference
 * set none and drew with whatever declaration was current. In this port a mismatched declaration would make the
 * Metal layout stride differ from the 24-byte inline data (the backend widens the stride to fit the declaration),
 * so the GPU would read past the uploaded vertices. The kit's SetFVF is a stub, so a real declaration is created
 * (CreateVertexDeclaration 0x158) and the previous one is restored after the draw (Get 0x160 / Set 0x15c; method
 * offsets from the kit's IDirect3DDevice9 table). Returns 0 if it cannot be made (the draw is then skipped). */
static uint32_t xfx_decl(uint32_t dev) {
    if (g_xfx_decl && g_xfx_decl_dev == dev) return g_xfx_decl;
    void *p = NULL;
    if (g_api->guest_ptr(g_api, XFX_DECL_ELEMS, 32u, &p) != POP_OK || !p) return 0;
    /* Stream, Offset (WORD); Type, Method, Usage, UsageIndex (BYTE) */
    static const uint8_t elems[32] = {
        0, 0, 0, 0,  2 /* FLOAT3 */, 0, 0 /* POSITION */, 0,
        0, 0, 12, 0, 4 /* D3DCOLOR */, 0, 10 /* COLOR */, 0,
        0, 0, 16, 0, 1 /* FLOAT2 */, 0, 5 /* TEXCOORD */, 0,
        0xff, 0, 0, 0, 17 /* UNUSED: D3DDECL_END */, 0, 0, 0};
    memcpy(p, elems, sizeof elems);
    g_api->guest_write_u32(g_api, XFX_DECL_OUT, 0);
    const uint32_t a[2] = {XFX_DECL_ELEMS, XFX_DECL_OUT};
    if (xfx_com(dev, 0x158u, a, 2) != 0) return 0;               /* CreateVertexDeclaration */
    g_xfx_decl = get_u32(XFX_DECL_OUT);
    g_xfx_decl_dev = g_xfx_decl ? dev : 0;
    return g_xfx_decl;
}

/* the draw: the game's world-prelit effect, its particle transform, texture MAIN, no depth writes; two triangles per
 * quad (indices 0 1 2, 0 2 3 as the reference's index buffer), with our own vertex declaration (xfx_decl). */
static void xfx_draw(void) {
    const uint32_t dev = get_u32(XFX_DEVICE);
    if (!dev || !g_xfx_count || !g_xfx_texinfo) return;
    void *vp = NULL;
    if (g_api->guest_ptr(g_api, g_xfx_verts, XFX_MAX_PARTICLES * 4u * (uint32_t)sizeof(XenonVertex), &vp) != POP_OK || !vp) return;
    const uint32_t quads = xp_build_quads(g_xfx_parts, g_xfx_count, (XenonVertex *)vp, XFX_MAX_PARTICLES);
    if (!quads) return;
    const uint32_t decl = xfx_decl(dev);
    if (!decl) return;
    g_api->guest_write_u32(g_api, XFX_SHADER_CURRENT, get_u32(XFX_SHADER_WORLDPRELIT));
    const uint32_t shader = get_u32(XFX_SHADER_CURRENT), fx = shader ? get_u32(shader + 0x48u) : 0;
    if (!fx) return;
    const uint32_t b[2] = {XFX_ATTR_OUT, 0};   /* pPasses: a scratch word (the reference passed NULL; our D3DX may not accept it) */
    xfx_com(fx, 0xfcu, b, 2);                                    /* Begin */
    const uint32_t t[2] = {XFX_PARTICLE_MATRIX, get_u32(XFX_EVIEW_MAIN + XFX_EVIEW_ID)};
    xfx_call(XFX_PARTICLE_TRANSFORM, 0, t, 2);
    const uint32_t p0[1] = {0};
    xfx_com(fx, 0x100u, p0, 1);                                  /* BeginPass */
    const uint32_t zw0[2] = {14, 0};
    xfx_com(dev, 0xe4u, zw0, 2);                                /* ZWRITEENABLE off */
    const uint32_t st[2] = {g_xfx_texinfo, 0};
    xfx_call(XFX_SET_TEXTURE, 0, st, 2);
    xfx_com(fx, 0x104u, NULL, 0);                                /* CommitChanges */
    g_api->guest_write_u32(g_api, XFX_DECL_PREV, 0);
    const uint32_t gp[1] = {XFX_DECL_PREV}, sd[1] = {decl};
    xfx_com(dev, 0x160u, gp, 1);                                /* GetVertexDeclaration (AddRefs) */
    const uint32_t prev = get_u32(XFX_DECL_PREV);
    xfx_com(dev, 0x15cu, sd, 1);                                /* SetVertexDeclaration (ours) */
    const uint32_t d[8] = {4 /* TRIANGLELIST */, 0, 4u * quads, 2u * quads, g_xfx_index, 101 /* INDEX16 */, g_xfx_verts,
                           (uint32_t)sizeof(XenonVertex)};
    xfx_com(dev, 0x150u, d, 8);                                 /* DrawIndexedPrimitiveUP */
    if (prev) {
        const uint32_t pd[1] = {prev};
        xfx_com(dev, 0x15cu, pd, 1);                            /* restore the game's declaration */
        xfx_com(prev, 0x8u, NULL, 0);                           /* Release the reference GetVertexDeclaration added */
    }
    const uint32_t zw1[2] = {14, 1};
    xfx_com(dev, 0xe4u, zw1, 2);
    xfx_com(fx, 0x108u, NULL, 0);                                /* EndPass */
    xfx_com(fx, 0x10cu, NULL, 0);                                /* End */
}

static int xfx_live(void) { return g_xfx_on && g_xfx_ready && !g_xfx_broken && setting("xbox_particles", 1); }

/* ---- hooks ---- */
static void xfx_update_dt(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)inv; (void)user;
    uint32_t bits = 0;
    api->guest_read_u32(api, cpu->esp + 4u, &bits);
    float dt;
    memcpy(&dt, &bits, 4);
    if (dt > 0.0f && dt < 1.0f) { g_xfx_dt += dt; g_xfx_clock += dt; }
}
/* LoadGlobalChunks' empty stub at 006648bc: the reference loads its texture pack here */
static void xfx_global_load(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    api->call_next(api, inv, cpu);
    if (!g_xfx_on || g_xfx_tpk_loaded || !g_xfx_tpk || !g_xfx_mem) return;
    g_xfx_tpk_loaded = 1;
    const uint32_t name = xfx_str("GLOBAL\\XenonEffects.tpk");
    const uint32_t c[5] = {name, 0, 0, 0, 0};
    const uint32_t rf = name ? xfx_call(XFX_CREATE_RESFILE, 0, c, 5) : 0;
    if (!rf) { api->log(api, "core.nfsmw: sparks and light trails: texture pack could not be opened; off"); g_xfx_broken = 1; return; }
    const uint32_t bl[2] = {0, 0};
    xfx_call(XFX_RESFILE_BEGIN, rf, bl, 2);
    xfx_call(XFX_SERVICE_RESOURCES, 0, NULL, 0);
    const uint32_t h[1] = {xfx_str("MAIN")};
    const uint32_t hash = h[0] ? xfx_call(XFX_BSTRINGHASH, 0, h, 1) : 0;
    const uint32_t g[3] = {hash, 0, 0};
    g_xfx_texinfo = hash ? xfx_call(XFX_GET_TEXTUREINFO, 0, g, 3) : 0;   /* retried at the first renders if not yet */
    const uint32_t e1[1] = {xfx_str("emsprk_line1")};
    g_xfx_elastic_key[0] = e1[0] ? xfx_call(XFX_STRINGHASH32, 0, e1, 1) : 0;
    const uint32_t e2[1] = {xfx_str("emsprk_line2")};
    g_xfx_elastic_key[1] = e2[0] ? xfx_call(XFX_STRINGHASH32, 0, e2, 1) : 0;
    g_xfx_ready = !g_xfx_broken;
    char line[160];
    snprintf(line, sizeof line, "core.nfsmw: sparks and light trails: texture pack loaded, MAIN %s; %s",
             g_xfx_texinfo ? "found" : "missing", g_xfx_ready ? "ready" : "off");
    api->log(api, line);
}
/* after EmitterSystem::Render of the main view (006df4bb): age, expand the queued effects, draw */
static void xfx_render(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    api->call_next(api, inv, cpu);
    const int world = get_u32(XFX_GAMEFLOW) == 6u;
    if (!world) {
        if (g_xfx_was_world) { g_xfx_count = 0; g_xfx_queued = 0; g_xfx_dt = 0.0f; }   /* left the world: clear */
        g_xfx_was_world = 0;
        return;
    }
    g_xfx_was_world = 1;
    const float dt = g_xfx_dt;
    g_xfx_dt = 0.0f;
    if (!xfx_live()) { g_xfx_count = 0; g_xfx_queued = 0; return; }
    if (!g_xfx_texinfo && g_xfx_tex_tries < 600u) {   /* the reference looks MAIN up after the global load (0066493e) */
        ++g_xfx_tex_tries;
        const uint32_t h[1] = {xfx_str("MAIN")};
        const uint32_t hash = h[0] ? xfx_call(XFX_BSTRINGHASH, 0, h, 1) : 0;
        const uint32_t g[3] = {hash, 0, 0};
        g_xfx_texinfo = hash ? xfx_call(XFX_GET_TEXTUREINFO, 0, g, 3) : 0;
        if (g_xfx_texinfo) api->log(api, "core.nfsmw: sparks and light trails: texture MAIN found");
    }
    g_xfx_count = xp_age(g_xfx_parts, g_xfx_count, dt, xfx_hit, NULL);
    for (uint32_t i = 0; i < g_xfx_queued && !g_xfx_broken; ++i) xfx_expand(&g_xfx_queue[i], dt);
    g_xfx_queued = 0;
    xfx_draw();
    ++g_xfx_frames;
    if (g_xfx_broken) api->log(api, "core.nfsmw: sparks and light trails: a guest call failed; off for this session");
}
/* Emitter::SpawnParticles: after the game's own particles, the emitter's NGEffect effects (sparks). The reference's
 * cave runs at the normal end (0050a5d7) only; the early exits are mirrored (time window, 00509b4b-00509b7a); its
 * rare mid-loop exit (0050a5e7) cannot be seen from outside and is accepted (DERIVED). */
static void xfx_emitter_spawn(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t self = cpu->ecx;
    uint32_t t_bits = 0;
    api->guest_read_u32(api, cpu->esp + 8u, &t_bits);
    api->call_next(api, inv, cpu);
    if (!xfx_live() || !self) return;
    float t;
    memcpy(&t, &t_bits, 4);
    if (!(t > 0.0f) || t > get_float(self + XFX_EMITTER_END) || t < get_float(self + XFX_EMITTER_START)) return;
    const uint32_t inst = get_u32(self + XFX_EMITTER_ATTRIB);
    if (!inst) return;
    const uint32_t coll = get_u32(inst + 4u), slot = (inst >> 4) & 63u;
    uint32_t n;
    if (g_xfx_spark_cache_inst[slot] == inst && g_xfx_spark_cache_coll[slot] == coll) {
        n = g_xfx_spark_cache_n[slot];
    } else {
        n = xfx_list_length(inst, XFX_KEY_XENON_FX);
        g_xfx_spark_cache_inst[slot] = inst;
        g_xfx_spark_cache_coll[slot] = coll;
        g_xfx_spark_cache_n[slot] = n;
    }
    if (!n || g_xfx_clock - g_xfx_spark_last < 1.0 / 60.0) return;   /* spark limiter: 60 per second */
    g_xfx_spark_last = g_xfx_clock;
    for (uint32_t i = 0; i < n; ++i) {
        const uint32_t spec = xfx_ref_collection(inst, XFX_KEY_XENON_FX, i);
        if (spec) { xfx_add(1.0f, spec, self + XFX_EMITTER_MATRIX, self + XFX_EMITTER_VELOCITY, get_u32(self + XFX_EMITTER_PIGGYBACK)); ++g_xfx_sparks; }
    }
}
static XfxCar *xfx_car(uint32_t car, int make) {
    XfxCar *free_slot = NULL;
    for (unsigned i = 0; i < sizeof g_xfx_cars / sizeof g_xfx_cars[0]; ++i) {
        if (g_xfx_cars[i].car == car) return &g_xfx_cars[i];
        if (!g_xfx_cars[i].car && !free_slot) free_slot = &g_xfx_cars[i];
    }
    if (!make) return NULL;
    if (!free_slot) free_slot = &g_xfx_cars[car % (sizeof g_xfx_cars / sizeof g_xfx_cars[0])];
    free_slot->car = car;
    free_slot->last = -1.0;
    free_slot->on = 0;
    return free_slot;
}
/* UpdateEngineAnimation at 00756629: the car's contrail state - NOS or speed >= 44, never during a cut-scene (the
 * reference's speed test, with NOS as its optional Carbon mode and dbalatoni's reconstruction have it). */
static void xfx_engine_anim(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t self = cpu->ecx;
    uint32_t pkt = 0;
    api->guest_read_u32(api, cpu->esp + 8u, &pkt);   /* UpdateEngineAnimation(float, PktCarService *) */
    api->call_next(api, inv, cpu);
    if (!xfx_live() || !self) return;
    XfxCar *c = xfx_car(self, 1);
    c->on = 0;
    if (!(get_u32(self + XFX_CAR_FLAGS) & 4u)) return;
    const uint32_t v = get_u32(self + XFX_CAR_VELOCITY_PTR);
    if (!v) return;
    const float x = get_float(v), y = get_float(v + 4u), z = get_float(v + 8u);
    const int nos = pkt && (get_u32(pkt + XFX_PKT_NOS) & 0xffu) != 0;
    c->on = (nos || sqrtf(x * x + y * y + z * z) >= XFX_CONTRAIL_SPEED) && !get_u32(XFX_NIS_INSTANCE);
}
/* CarRenderConn::OnRender: for a car whose contrail state is on, in a player view (type 1 or 2) whose first camera
 * mover's vtable +0x24 is true (as the PC's own OnRender asks at 00750ee8), queue the contrail effect; its intensity
 * follows the reference (xp_contrail_intensity, at expansion).
 * The reference's cave also skipped this when the function left early (its 00751017 / 007511d0 paths); the
 * mover-list and view tests below cover the first; the second (the car being the camera's own target in some modes)
 * is not mirrored (HYPOTHESIS that it does not matter; check in the image pass). */
static void xfx_car_render(const PopModApi *api, pop_cpu_v1 *cpu, PopHookInvocation *inv, void *user) {
    (void)user;
    const uint32_t self = cpu->ecx;
    uint32_t view = 0;
    api->guest_read_u32(api, cpu->esp + 4u, &view);
    api->call_next(api, inv, cpu);
    if (!xfx_live() || !self || !view) return;
    XfxCar *c = xfx_car(self, 0);
    if (!c || !c->on || g_xfx_clock - c->last < 1.0 / 30.0) return;   /* contrail limiter: 30 per second per car */
    const uint32_t type = get_u32(view + XFX_EVIEW_TYPE);
    if (type != 1u && type != 2u) return;
    const uint32_t head = view + XFX_EVIEW_MOVERS, node = get_u32(head + 4u);
    if (node == head || !node) return;
    const uint32_t mover = node - 4u, fn = get_u32(get_u32(mover) + XFX_MOVER_VT_0X24);
    if (!fn || !(xfx_call(fn, mover, NULL, 0) & 0xffu)) return;
    const uint32_t a[2] = {XFX_CONTRAIL_CLASS, XFX_CONTRAIL_KEY};
    const uint32_t spec = xfx_call(XFX_FIND_COLLECTION, 0, a, 2);
    const uint32_t vel = get_u32(self + XFX_CAR_VELOCITY_PTR), mat = get_u32(self + XFX_CAR_MATRIX_PTR);
    if (!spec || !vel || !mat) return;
    c->last = g_xfx_clock;
    xfx_add(0.0f, spec, mat, vel, 0);   /* the intensity follows the velocity, at expansion */
    ++g_xfx_contrails;
}
/* Slots 90..95. The texture pack comes from <mod>/xenon (overlay, GLOBAL/XenonEffects.tpk). */
static void xfx_init(const PopModApi *api) {
    if (!setting("xbox_particles", 1)) { api->log(api, "core.nfsmw: sparks and light trails off (setting; applies at next launch)"); return; }
    char path[2048];
    snprintf(path, sizeof path, "%s/xenon/GLOBAL/XenonEffects.tpk", api->mod_dir(api));
    FILE *f = fopen(path, "rb");
    if (!f) { api->log(api, "core.nfsmw: sparks and light trails: no xenon/GLOBAL/XenonEffects.tpk beside the mod; off"); return; }
    fclose(f);
    snprintf(path, sizeof path, "%s/xenon", api->mod_dir(api));
    uint32_t layer = 0;
    if (api->overlay_push(api, path, &layer) != POP_OK) { api->log(api, "core.nfsmw: sparks and light trails: overlay refused; off"); return; }
    g_xfx_tpk = 1;
    if (api->guest_alloc(api, XFX_MEM_SIZE, &g_xfx_mem) != POP_OK ||
        api->guest_alloc(api, XFX_MAX_PARTICLES * 4u * (uint32_t)sizeof(XenonVertex), &g_xfx_verts) != POP_OK ||
        api->guest_alloc(api, XFX_MAX_PARTICLES * 6u * 2u, &g_xfx_index) != POP_OK) {
        api->log(api, "core.nfsmw: sparks and light trails: guest memory unavailable; off");
        return;
    }
    {   /* indices 0 1 2, 0 2 3 per quad */
        void *p = NULL;
        if (api->guest_ptr(api, g_xfx_index, XFX_MAX_PARTICLES * 12u, &p) != POP_OK || !p) return;
        uint16_t *ix = (uint16_t *)p;
        for (uint32_t q = 0; q < XFX_MAX_PARTICLES && 4u * q + 3u < 65536u; ++q) {
            const uint16_t b = (uint16_t)(4u * q);
            ix[6 * q] = b; ix[6 * q + 1] = (uint16_t)(b + 1); ix[6 * q + 2] = (uint16_t)(b + 2);
            ix[6 * q + 3] = b; ix[6 * q + 4] = (uint16_t)(b + 2); ix[6 * q + 5] = (uint16_t)(b + 3);
        }
    }
    int ok = 0;
    ok += install_at(XFX_UPDATE_PARTICLES, XFX_UPDATE_RET, xfx_update_dt, POP_HOOK_BEFORE, 90) == POP_OK;
    ok += install_at(XFX_GLOBAL_STUB, XFX_GLOBAL_STUB_RET, xfx_global_load, POP_HOOK_WRAP, 91) == POP_OK;
    ok += install_at(XFX_EMITTERS_RENDER, XFX_EMITTERS_RENDER_RET, xfx_render, POP_HOOK_WRAP, 92) == POP_OK;
    ok += install(XFX_EMITTER_SPAWN, xfx_emitter_spawn, POP_HOOK_WRAP, 93) == POP_OK;
    ok += install(XFX_CAR_ONRENDER, xfx_car_render, POP_HOOK_WRAP, 94) == POP_OK;
    ok += install_at(XFX_CAR_ENGINE_ANIM, XFX_CAR_ENGINE_ANIM_RET, xfx_engine_anim, POP_HOOK_WRAP, 95) == POP_OK;
    g_xfx_on = ok == 6;
    char line[128];
    snprintf(line, sizeof line, "core.nfsmw: sparks and light trails: %d of 6 hooks%s", ok, g_xfx_on ? "" : "; off");
    api->log(api, line);
}
static void xfx_exit(const PopModApi *api) {
    if (!g_xfx_on) return;
    char line[240];
    snprintf(line, sizeof line, "core.nfsmw: sparks and light trails: %llu frames, %llu effects (%llu contrails, %llu sparks, %llu dropped), %llu particles, %llu impacts",
             (unsigned long long)g_xfx_frames, (unsigned long long)g_xfx_effects, (unsigned long long)g_xfx_contrails,
             (unsigned long long)g_xfx_sparks, (unsigned long long)g_xfx_dropped, (unsigned long long)g_xfx_spawned,
             (unsigned long long)g_xfx_hits);
    api->log(api, line);
}
#endif
