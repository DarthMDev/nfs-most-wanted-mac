/* shadow_cast.cpp - the scenery shadow-caster test 006d7a40, native (Test150).
 *
 * 006d7a40 cdecl (pos, size) -> AL: does a scenery instance outside the view cast a shadow into it?
 * Only caller: the scenery renderer 006da9b0 (twice), once per instance per pass. It builds a box
 * from pos[0..2] and size[0] (all three far corners take size[0]: the original's own choice, kept),
 * then:
 *   006c9680  thiscall (planes 0x987ce0; box) -> 0 outside, 1 inside, 2 crossing: the box against
 *             six planes, each with a corner-selection mask at +0x60. Inside or crossing -> AL = 1.
 *   006c85f0  thiscall (sphere out; box): the box's bounding sphere.
 *   006c9570  thiscall (planes; sphere, dir): the sphere swept along the sun direction
 *             (0x988000 times 0x890da8) meets the planes. Hit -> ++[0x982d08], AL = 1; else AL = 0.
 * In a sunny tree-lined area (Rosewood) this runs for every tree outside the view, several thousand
 * times a frame; the two inner box routines were still translated x87 (Test147: about 0.4 ms a frame
 * of the main thread there, plus 0.67 ms in native_006c9570).
 *
 * Here all of it runs on host doubles, in the same order and through the same x86.h helpers as
 * culling.c and aabb.cpp, so AL, the caster counter and every status-word bit match the
 * translation. The guest stack below the caller's ESP (the original's locals and the callees'
 * argument slots, scratch nobody reads afterwards) is left alone; ECX and EDX, which its only
 * caller overwrites before reading, are left as they were.
 *
 * RECOMP_NATIVE=0 runs the translation. RECOMP_NATIVE_SELFTEST=1 compares the two on random input
 * the first time it is called. TEST ONLY: RECOMP_AB_PERIOD=<s> with RECOMP_AB_SWITCHES containing
 * "shadowcast" alternates translation / native by period (platform/ab_phase.h); RECOMP_SHADOWCAST_STATS=1
 * logs calls, outcomes and time per call for each side every 5 s. RECOMP_SHADOWCAST_EXACT=1 turns the
 * early-out off (swept_early_h). */
#include "runtime/guest.h"
extern "C" {
#include "funcs.h"
}
#include "runtime/memory.h"
#include "platform/ab_phase.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern "C" void native_006d7a40(X86 *c);

static const uint32_t PLANES = 0x00987ce0u;     /* six planes (a, b, c, d); corner masks at +0x60 */
static const uint32_t SHADOWS_ON = 0x008faf14u; /* scenery shadows enabled */
static const uint32_t SUN_DIR = 0x00988000u;
static const uint32_t K_SWEEP = 0x00890da8u;    /* sweep length */
static const uint32_t CASTERS = 0x00982d08u;    /* count of swept hits */
static const uint32_t K_ZERO = 0x00890968u, K_ONE = 0x0089096cu, K_HALF = 0x008933d4u;
static const uint32_t K_RADIUS = 0x00a3781cu;   /* radius scale: game.toml redirects 0x008910c4 here */

static void sc_selftest(X86 *c);
static int g_sc_native = -1;
static int g_sc_ab = 0;    /* "shadowcast" is in RECOMP_AB_SWITCHES */
static int g_sc_stats = 0; /* RECOMP_SHADOWCAST_STATS=1 */
static int g_sc_exact = 0; /* RECOMP_SHADOWCAST_EXACT=1: no early-out (see swept_early_h) */

static int sc_native_on(X86 *c) {
    if (__builtin_expect(g_sc_native >= 0, 1))
        return g_sc_native && !(g_sc_ab && recomp_ab::old_phase.load(std::memory_order_relaxed));
    const char *e = getenv("RECOMP_NATIVE");
    const int want = !(e && e[0] == '0');
    const char *sw = recomp_env("AB_SWITCHES");
    g_sc_ab = sw && strstr(sw, "shadowcast") != nullptr;
    const char *st = getenv("RECOMP_SHADOWCAST_STATS");
    g_sc_stats = (st && st[0] == '1') || getenv("RECOMP_SHADOWCAST_TRACE") != nullptr;
    const char *ex = getenv("RECOMP_SHADOWCAST_EXACT");
    g_sc_exact = ex && ex[0] == '1';
    const char *t = getenv("RECOMP_NATIVE_SELFTEST");
    if (t && t[0] == '1')
        sc_selftest(c);
    g_sc_native = want;
    return sc_native_on(c);
}

static inline uint32_t ah_of(const X86 *c) {
    return (uint32_t)(fstsw(c) >> 8);
}
/* JP after TEST AH,mask: taken on even parity of the masked bits. */
static inline bool jp(uint32_t ah, uint32_t mask) {
    return (__builtin_popcount(ah & mask) & 1) == 0;
}

/* 006c9680: 0 outside, 1 inside, 2 crossing. */
static int box_vs_planes(X86 *c, const float box[6]) {
    const double zero = rdf32(K_ZERO);
    int crossing = 0;
    for (uint32_t i = 0; i < 6; ++i) {
        const uint32_t pl = PLANES + 16u * i;
        const uint32_t m = rd32(PLANES + 0x60u + 4u * i);
        const float xp = (m & 1u) ? box[0] : box[3], xn = (m & 1u) ? box[3] : box[0];
        const float yp = (m & 2u) ? box[1] : box[4], yn = (m & 2u) ? box[4] : box[1];
        const float zp = (m & 4u) ? box[2] : box[5], zn = (m & 4u) ? box[5] : box[2];
        const double a = rdf32(pl), b = rdf32(pl + 4), cc = rdf32(pl + 8), d = rdf32(pl + 12);
        double s = fx87(c, fx87(c, (double)xp * a) + fx87(c, (double)zp * cc));
        s = fx87(c, s + fx87(c, (double)yp * b));
        s = fx87(c, s + d);
        fcom(c, s, zero);
        if (!jp(ah_of(c), 0x05)) /* below the plane: outside */
            return 0;
        double t = fx87(c, fx87(c, (double)xn * a) + fx87(c, (double)zn * cc));
        t = fx87(c, t + fx87(c, (double)yn * b));
        t = fx87(c, t + d);
        fcom(c, t, zero);
        if (!jp(ah_of(c), 0x05))
            crossing = 1;
    }
    return crossing ? 2 : 1;
}

/* 006c85f0: centre and radius of the box's bounding sphere. */
static void box_sphere(X86 *c, const float box[6], float sph[4]) {
    const double half = rdf32(K_HALF);
    const double s0 = fx87(c, (double)box[0] + (double)box[3]);
    const double s1 = fx87(c, (double)box[4] + (double)box[1]);
    const double s2 = fx87(c, (double)box[5] + (double)box[2]);
    const float t2 = fto_float(c, s2);
    sph[0] = fto_float(c, fx87(c, s0 * half));
    sph[1] = fto_float(c, fx87(c, s1 * half));
    sph[2] = fto_float(c, fx87(c, (double)t2 * half));
    const float dx = fto_float(c, fx87(c, (double)box[3] - (double)sph[0]));
    const float dy = fto_float(c, fx87(c, (double)box[4] - (double)sph[1]));
    const float dz = fto_float(c, fx87(c, (double)box[5] - (double)sph[2]));
    double r = fx87(c, (double)dz * (double)dz);
    r = fx87(c, r + fx87(c, (double)dy * (double)dy));
    r = fx87(c, r + fx87(c, (double)dx * (double)dx));
    sph[3] = fto_float(c, fx87(c, sqrt(r)));
}

/* 006c9440 on host values (culling.c clip_plane): the t range of P + t*D within +-P[3] of a plane. */
static bool clip_h(X86 *c, uint32_t plane, const float P[4], const float D[3], float *lo_out, float *hi_out) {
    const double pl0 = rdf32(plane), pl1 = rdf32(plane + 4), pl2 = rdf32(plane + 8);
    double dist = fx87(c, fx87(c, fx87(c, (double)P[1] * pl1) + fx87(c, (double)P[2] * pl2)) +
                              fx87(c, pl0 * (double)P[0]));
    dist = fx87(c, dist + (double)rdf32(plane + 12));
    double den = fx87(c, fx87(c, (double)D[1] * pl1) + fx87(c, (double)D[2] * pl2));
    den = fx87(c, den + fx87(c, pl0 * (double)D[0]));
    fucom(c, den, (double)rdf32(K_ZERO));
    if (!jp(ah_of(c), 0x44)) {
        /* den == 0: parallel. Inside when dist <= P[3]. */
        fcom(c, dist, (double)P[3]);
        if (jp(ah_of(c), 0x41))
            return false;
        uint32_t lo = 0, hi = 0x749dc5aeu; /* 0, 1e32 */
        memcpy(lo_out, &lo, 4);
        memcpy(hi_out, &hi, 4);
        return true;
    }
    const double inv = fx87(c, fdivz(c, (double)rdf32(K_ONE), den));
    const double p3 = P[3];
    const float t1f = fto_float(c, fx87(c, fx87(c, p3 - dist) * inv));
    const double t2 = fx87(c, inv * fx87(c, -p3 - dist));
    const double t1 = t1f;
    fcom(c, t1, t2);
    const double lo = jp(ah_of(c), 0x05) ? t2 : t1;
    *lo_out = fto_float(c, lo);
    fcom(c, t1, t2);
    const double hi = (ah_of(c) & 0x41) ? t2 : t1;
    *hi_out = fto_float(c, hi);
    return true;
}

/* 006c9510 on host values: the sphere is on the inner side of all six planes. */
static uint8_t sphere_in_h(X86 *c, float sx, float sy, float sz, float r) {
    const double zero = rdf32(K_ZERO);
    uint32_t p = PLANES + 4;
    uint8_t dl = 1;
    for (int i = 0; i < 6 && dl; ++i, p += 16) {
        double s = fx87(c, (double)rdf32(p + 4) * (double)sz);
        s = fx87(c, s + fx87(c, (double)rdf32(p - 4) * (double)sx));
        s = fx87(c, s + fx87(c, (double)sy * (double)rdf32(p)));
        s = fx87(c, s + (double)rdf32(p + 8));
        s = fx87(c, s + (double)r);
        fcom(c, s, zero);
        dl &= (ah_of(c) & 1) ? 0 : 1;
    }
    return dl;
}

/* 006c9570 on host values (culling.c native_006c9570). */
/* One swept candidate: the sphere moved t along dir, its radius scaled, inside all six planes. */
static uint8_t candidate_h(X86 *c, const float sph[4], const float dir[3], double t) {
    const double k = rdf32(K_RADIUS);
    const double r = sph[3];
    const double m0 = fx87(c, t * (double)dir[0]);
    const float f1 = fto_float(c, fx87(c, t * (double)dir[1]));
    const float f2 = fto_float(c, fx87(c, t * (double)dir[2]));
    const float v0 = fto_float(c, fx87(c, m0 + (double)sph[0]));
    const float v1 = fto_float(c, fx87(c, (double)sph[1] + (double)f1));
    const float v2 = fto_float(c, fx87(c, (double)sph[2] + (double)f2));
    const float v3 = fto_float(c, fx87(c, r * k));
    return sphere_in_h(c, v0, v1, v2, v3);
}

/* Early-out form (default): the result is the OR over the candidates in the original's order, so
 * the first hit decides it; candidates are tested as they are found and the rest of the work is
 * skipped. AL and the caster counter are the original's; only the FPU status bits left behind by
 * the skipped compares differ, and nothing reads those (the only caller tests AL; every later x87
 * compare in it sets its own). RECOMP_SHADOWCAST_EXACT=1 keeps the full sequence. */
static uint8_t swept_early_h(X86 *c, const float sph[4], const float dir[3]) {
    const double zero = rdf32(K_ZERO);
    float lo = 0.0f, hi = 0.0f;
    for (uint32_t j = 0; j < 6; ++j) {
        if (!clip_h(c, PLANES + 16u * j, sph, dir, &lo, &hi))
            continue;
        fcom(c, lo, zero);
        if (!(ah_of(c) & 1) && candidate_h(c, sph, dir, lo))
            return 1;
        fcom(c, hi, zero);
        if (!(ah_of(c) & 1) && candidate_h(c, sph, dir, hi))
            return 1;
    }
    return 0;
}

static uint8_t swept_h(X86 *c, const float sph[4], const float dir[3]) {
    if (!g_sc_exact)
        return swept_early_h(c, sph, dir);
    const double zero = rdf32(K_ZERO);
    float list[13];
    int n = 0;
    float lo = 0.0f, hi = 0.0f;
    for (uint32_t j = 0; j < 6; ++j) {
        if (!clip_h(c, PLANES + 16u * j, sph, dir, &lo, &hi))
            continue;
        fcom(c, lo, zero);
        if (!(ah_of(c) & 1))
            list[++n] = lo;
        fcom(c, hi, zero);
        if (!(ah_of(c) & 1))
            list[++n] = hi;
    }
    uint8_t result = 0;
    for (int i = 0; i < n; ++i)
        result |= candidate_h(c, sph, dir, list[i + 1]);
    return result;
}

/* Outcomes: 0 shadows off, 1 box in view, 2 swept hit, 3 swept miss. */
static int caster_native(X86 *c, uint32_t pos, uint32_t size) {
    if (rd32(SHADOWS_ON) == 0) {
        c->r[R_EAX] = 1;
        return 0;
    }
    float box[6] = {rdf32(pos), rdf32(pos + 4), rdf32(pos + 8), rdf32(size), rdf32(size), rdf32(size)};
    const int r = box_vs_planes(c, box);
    if (r == 1 || r == 2) {
        c->r[R_EAX] = 1;
        return 1;
    }
    float sph[4];
    box_sphere(c, box, sph);
    const double k = rdf32(K_SWEEP);
    const double px = fx87(c, (double)rdf32(SUN_DIR) * k);
    const double py = fx87(c, (double)rdf32(SUN_DIR + 4) * k);
    const double pz = fx87(c, (double)rdf32(SUN_DIR + 8) * k);
    const float dir[3] = {fto_float(c, px), fto_float(c, py), fto_float(c, pz)};
    const uint8_t hit = swept_h(c, sph, dir);
    c->r[R_EAX] = hit;
    if (hit) {
        wr32(CASTERS, rd32(CASTERS) + 1u);
        return 2;
    }
    return 3;
}

/* TEST ONLY statistics, per side (0 translation, 1 native). */
static struct {
    uint64_t calls[2], ns[2], outcome[2][4];
    uint64_t last_ns;
} g_scs;

static inline uint64_t sc_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void sc_stats_flush(uint64_t now) {
    if (!g_scs.last_ns) {
        g_scs.last_ns = now;
        return;
    }
    if (now - g_scs.last_ns < 5000000000ull)
        return;
    const double secs = (double)(now - g_scs.last_ns) / 1e9;
    for (int s = 0; s < 2; ++s) {
        if (!g_scs.calls[s])
            continue;
        fprintf(stderr,
                "[shadowcast] %s: %.0f calls/s, %.1f ns/call, %.3f ms/s; off %llu in-view %llu hit %llu miss %llu\n",
                s ? "native" : "translated", (double)g_scs.calls[s] / secs, (double)g_scs.ns[s] / (double)g_scs.calls[s],
                (double)g_scs.ns[s] / 1e6 / secs, (unsigned long long)g_scs.outcome[s][0],
                (unsigned long long)g_scs.outcome[s][1], (unsigned long long)g_scs.outcome[s][2],
                (unsigned long long)g_scs.outcome[s][3]);
    }
    memset(&g_scs, 0, sizeof g_scs);
    g_scs.last_ns = now;
}

/* TEST ONLY (Test154): RECOMP_SHADOWCAST_TRACE="wx:wy:wz:rcam:rcast" (world coordinates as the position log prints
 * them). While the main camera is within rcam of the point: once per game frame, the camera position and the sun-
 * shadow matrix the shadow pass set this frame ([[0x00919c70]+0x40], 16 floats); and for every caster test whose box
 * minimum lies within rcast of the point, its box, outcome and its smallest corner margin to the six caster planes.
 * Plain doubles, no x87 helper calls: the guest's FPU state is untouched. */
static int g_sct_on = -1;
static double g_sct_p[3], g_sct_pc[3], g_sct_rcam, g_sct_rcast;
static uint32_t g_sct_frame = 0xffffffffu;
static int g_sct_in = 0;
static uint64_t g_sct_lines = 0;
static void sc_trace(uint32_t pos, uint32_t size, int outcome) {
    if (g_sct_on < 0) {
        const char *e = getenv("RECOMP_SHADOWCAST_TRACE");
        double v[5];
        g_sct_on = e && sscanf(e, "%lf:%lf:%lf:%lf:%lf", &v[0], &v[1], &v[2], &v[3], &v[4]) == 5;
        if (g_sct_on) {
            /* render space is the world space the scenery probe and the QA teleport use (z up) */
            g_sct_p[0] = v[0], g_sct_p[1] = v[1], g_sct_p[2] = v[2];
            g_sct_rcam = v[3], g_sct_rcast = v[4];
            fprintf(stderr, "[sctrace] on: point %.1f,%.1f,%.1f rcam %.0f rcast %.0f\n", v[0], v[1], v[2], v[3], v[4]);
        }
    }
    if (!g_sct_on || g_sct_lines > 400000)
        return;
    const uint32_t frame = rd32(0x009885b8u);
    if (frame != g_sct_frame) {
        g_sct_frame = frame;
        const uint32_t cam = rd32(0x00919650u + 0x40u);
        g_sct_in = 0;
        if (cam) {
            const double cx = rdf32(cam + 0x40), cy = rdf32(cam + 0x44), cz = rdf32(cam + 0x48);
            const double dx = cx - g_sct_p[0], dy = cy - g_sct_p[1];
            const double d = sqrt(dx * dx + dy * dy);
            g_sct_in = d <= g_sct_rcam;
            if (g_sct_in) {
                const uint32_t blk = rd32(0x00919c70u), vblk = rd32(0x00919650u);
                char m[900];
                int n = 0;
                for (int i = 0; i < 16 && blk; ++i)
                    n += snprintf(m + n, sizeof m - (size_t)n, " %.6g", (double)rdf32(blk + 0x40u + 4u * (uint32_t)i));
                n += snprintf(m + n, sizeof m - (size_t)n, " V");
                for (int i = 0; i < 16 && vblk; ++i)
                    n += snprintf(m + n, sizeof m - (size_t)n, " %.6g", (double)rdf32(vblk + 0x40u + 4u * (uint32_t)i));
                /* 006da9b0 transforms each caster box by world x *[0x00982d04] before 006d7a40 (only while
                 * [0x00982b38], the shadow pass): the caster test runs in that space. Put the point there too, and
                 * report its signed distance to each of the six planes at 0x987ce0 (the receiver volume). */
                const uint32_t L = rd32(0x00982d04u);
                for (int jj = 0; jj < 3 && L; ++jj) {
                    double acc = rdf32(L + 4u * (uint32_t)(12 + jj));
                    for (int ii = 0; ii < 3; ++ii)
                        acc += g_sct_p[ii] * (double)rdf32(L + 4u * (uint32_t)(ii * 4 + jj));
                    g_sct_pc[jj] = acc;
                }
                n += snprintf(m + n, sizeof m - (size_t)n, " P %.2f %.2f %.2f margins", g_sct_pc[0], g_sct_pc[1], g_sct_pc[2]);
                for (uint32_t pi = 0; pi < 6 && L; ++pi) {
                    const uint32_t pl = PLANES + 16u * pi;
                    const double v = g_sct_pc[0] * rdf32(pl) + g_sct_pc[1] * rdf32(pl + 4) + g_sct_pc[2] * rdf32(pl + 8) + rdf32(pl + 12);
                    n += snprintf(m + n, sizeof m - (size_t)n, " %.3f", v);
                }
                struct timespec ts;
                clock_gettime(CLOCK_REALTIME, &ts);
                struct tm tm;
                localtime_r(&ts.tv_sec, &tm);
                fprintf(stderr, "[sctrace] F %u %02d:%02d:%02d.%03ld cam %.2f,%.2f,%.2f d %.1f sun %.4f,%.4f,%.4f M%s\n",
                        frame, tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec / 1000000, cx, cy, cz, d,
                        (double)rdf32(SUN_DIR), (double)rdf32(SUN_DIR + 4), (double)rdf32(SUN_DIR + 8), blk ? m : " none");
                ++g_sct_lines;
            }
        }
    }
    if (!g_sct_in)
        return;
    const double bx = rdf32(pos), by = rdf32(pos + 4), bz = rdf32(pos + 8), s = rdf32(size);
    /* boxes reach 006d7a40 already transformed (006be800 in 006da9b0): test both world and camera space */
    const double dx = bx - g_sct_p[0], dy = by - g_sct_p[1];
    const double ex = bx - g_sct_pc[0], ey = by - g_sct_pc[1], ez = bz - g_sct_pc[2];
    const int world_near = dx * dx + dy * dy <= g_sct_rcast * g_sct_rcast;
    const int cam_near = ex * ex + ey * ey + ez * ez <= g_sct_rcast * g_sct_rcast;
    if (!world_near && !cam_near)
        return;
    const double box[6] = {bx, by, bz, s, s, s};
    double best = 1e30;
    int bestp = -1;
    for (uint32_t i = 0; i < 6; ++i) {
        const uint32_t pl = PLANES + 16u * i, m = rd32(PLANES + 0x60u + 4u * i);
        const double xp = (m & 1u) ? box[0] : box[3], yp = (m & 2u) ? box[1] : box[4], zp = (m & 4u) ? box[2] : box[5];
        const double v = xp * rdf32(pl) + yp * rdf32(pl + 4) + zp * rdf32(pl + 8) + rdf32(pl + 12);
        if (v < best)
            best = v, bestp = (int)i;
    }
    fprintf(stderr, "[sctrace] C %u %s box %.2f,%.2f,%.2f s %.2f -> %s margin %.3f plane %d\n", g_sct_frame,
            cam_near ? "cam" : "world", bx, by, bz, s,
            outcome == 0 ? "off" : outcome == 1 ? "in-view" : outcome == 2 ? "hit" : "miss", best, bestp);
    ++g_sct_lines;
}

extern "C" void native_006d7a40(X86 *c) {
    const int native = sc_native_on(c);
    if (__builtin_expect(!g_sc_stats, 1)) {
        if (!native) {
            fn_006d7a40(c);
            return;
        }
        const uint32_t esp = c->r[R_ESP];
        caster_native(c, rd32(esp + 4), rd32(esp + 8));
        c->eip = rd32(esp);
        c->r[R_ESP] = esp + 4;
        return;
    }
    const uint32_t esp = c->r[R_ESP];
    const uint32_t casters = rd32(CASTERS);
    const uint32_t tpos = rd32(esp + 4), tsize = rd32(esp + 8);
    const uint64_t t0 = sc_now();
    int outcome;
    if (!native) {
        fn_006d7a40(c);
        /* Classify from the result: off/in-view both return 1 without a hit, so reread the inputs. */
        if (rd32(CASTERS) != casters)
            outcome = 2;
        else if ((c->r[R_EAX] & 0xffu) == 0)
            outcome = 3;
        else
            outcome = rd32(SHADOWS_ON) ? 1 : 0;
    } else {
        outcome = caster_native(c, rd32(esp + 4), rd32(esp + 8));
        c->eip = rd32(esp);
        c->r[R_ESP] = esp + 4;
    }
    const uint64_t t1 = sc_now();
    g_scs.calls[native] += 1;
    g_scs.ns[native] += t1 - t0;
    g_scs.outcome[native][outcome] += 1;
    sc_stats_flush(t1);
    sc_trace(tpos, tsize, outcome);
}

/* ---- self-test ----------------------------------------------------------------------------------
 * The translation (whose inner 006c9570 is culling.c's native, as shipped) and the replacement on
 * random planes, masks, sun directions and boxes, written into the real globals for the duration of
 * the test and restored afterwards. */
static uint32_t s_rnd_state = 0x7f4a7c15u;
static uint32_t s_rnd(void) {
    s_rnd_state = s_rnd_state * 1664525u + 1013904223u;
    return s_rnd_state;
}
static float s_rnd_float(void) {
    uint32_t k = s_rnd() % 128;
    if (k == 0)
        return 0.0f;
    if (k == 1)
        return -0.0f;
    if (k == 2)
        return 3e38f;
    if (k == 3)
        return (float)__builtin_nan("");
    if (k == 4)
        return 1e-40f;
    return ((float)(int32_t)s_rnd() / 2147483648.0f) * (k < 64 ? 2.0f : 500.0f);
}

/* Directed cases (external review review of Test150): a cube frustum [-10, 10]^3 and boxes built as the original
 * builds them (min = pos, every far corner = size[0]). The names say what each case aims at; the check is
 * translation == native, and each case's outcome is logged once. */
struct ScCase {
    float pos[3], size, sun[3];
    uint32_t on;
    const char *name;
};
static const ScCase k_sc_cases[] = {
    {{0, 0, 0}, 1, {0, 0, -1}, 1, "start inside"},
    {{10, 0, 0}, 10, {1, 0, 0}, 1, "tangent to the +x plane"},
    {{20, 0, 0}, 21, {1, 0, 0}, 1, "outside +x, swept back along -x"},
    {{20, 0, 0}, 21, {-1, 0, 0}, 1, "outside +x, swept away"},
    {{0, 20, 0}, 21, {0, 1, 0}, 1, "outside +y, swept along -y"},
    {{0, 20, 0}, 21, {0, -1, 0}, 1, "outside +y, swept away"},
    {{0, 0, 20}, 21, {0, 0, 1}, 1, "outside +z, swept along -z"},
    {{0, 0, 20}, 21, {0, 0, -1}, 1, "outside +z, swept away"},
    {{-30, -30, -30}, -29, {-1, -1, -1}, 1, "outside the -x-y-z corner, swept diagonally in"},
    {{-30, -30, -30}, -29, {1, 1, 1}, 1, "outside the -x-y-z corner, swept away"},
    {{15, 15, 15}, 15, {1, 1, 1}, 1, "zero extent (a point), swept in"},
    {{15, 15, 15}, 15, {-1, -1, -1}, 1, "zero extent (a point), swept away"},
    {{30, 0, 0}, 30.001f, {1, 0, 0}, 1, "small extent, swept in"},
    {{-5, 50, -5}, 5, {1, 0, 0}, 1, "size[0] far corners (inverted y), sweep parallel to four planes"},
    {{-5, 50, -5}, 5, {0, 1, 0}, 1, "size[0] far corners (inverted y), swept along -y"},
    {{1000, 1000, 1000}, 1001, {0, 0, 0}, 1, "far outside, zero sun direction"},
    {{1000, 1000, 1000}, 1001, {0.577f, 0.577f, 0.577f}, 1, "far outside, swept in"},
    {{0, 0, 0}, 1, {0, 0, -1}, 0, "scenery shadows off"},
};
static const int k_sc_ncases = (int)(sizeof k_sc_cases / sizeof k_sc_cases[0]);
static void sc_cube(void) {
    static const float n[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (uint32_t i = 0; i < 6; ++i) {
        wrf32(PLANES + 16 * i, n[i][0]);
        wrf32(PLANES + 16 * i + 4, n[i][1]);
        wrf32(PLANES + 16 * i + 8, n[i][2]);
        wrf32(PLANES + 16 * i + 12, 10.0f);
        /* corner mask: bit k picks the box minimum on axis k (the original's far-corner choice for a
         * positive normal is the maximum) */
        wr32(PLANES + 0x60 + 4 * i, (n[i][0] < 0 ? 1u : 0u) | (n[i][1] < 0 ? 2u : 0u) | (n[i][2] < 0 ? 4u : 0u));
    }
}

static void sc_selftest(X86 *c) {
    const uint32_t len = 0x20000;
    const uint32_t base = heap_alloc(len, true);
    if (!base) {
        fprintf(stderr, "native shadowcast self-test: no scratch block\n");
        return;
    }
    const uint32_t pos = base + 0x100, size = base + 0x120, stack = base + 0x10000;
    uint32_t ret = 0x00401001u;
    while (recomp_index_of(ret) >= 0 || recomp_module_lookup(ret) >= 0)
        ++ret;
    uint8_t saved_planes[0x78], saved_sun[12];
    uint32_t saved_on = rd32(SHADOWS_ON), saved_casters = rd32(CASTERS);
    memcpy(saved_planes, g_mem + PLANES, sizeof saved_planes);
    memcpy(saved_sun, g_mem + SUN_DIR, sizeof saved_sun);
    const uint16_t cw_saved = c->fpu_cw;
    const int exact_saved = g_sc_exact;
    for (int pass = 0; pass < 2; ++pass) {
    g_sc_exact = pass == 0; /* pass 0: the full sequence, every state bit; pass 1: early-out, all but FPU status */
    int bad = 0, runs = 0, sw_only = 0, outcomes[4] = {0, 0, 0, 0};
    int dbad = 0;
    for (int trial = -2 * k_sc_ncases; trial < 6000; ++trial) {
        const ScCase *dc = trial < 0 ? &k_sc_cases[(-trial - 1) >> 1] : nullptr;
        if (dc) {
            sc_cube();
            for (uint32_t i = 0; i < 3; ++i)
                wrf32(SUN_DIR + 4 * i, dc->sun[i]), wrf32(pos + 4 * i, dc->pos[i]);
            wrf32(size, dc->size);
            wr32(SHADOWS_ON, dc->on);
        } else {
        for (uint32_t i = 0; i < 24; ++i)
            wrf32(PLANES + 4 * i, s_rnd_float() * (trial % 5 == 0 ? 0.001f : 1.0f));
        for (uint32_t i = 0; i < 6; ++i)
            wr32(PLANES + 0x60 + 4 * i, s_rnd() & 7u);
        for (uint32_t i = 0; i < 3; ++i)
            wrf32(SUN_DIR + 4 * i, s_rnd_float());
        if (trial % 11 == 0)
            wrf32(SUN_DIR, 0.0f), wrf32(SUN_DIR + 4, 0.0f), wrf32(SUN_DIR + 8, 0.0f);
        wr32(SHADOWS_ON, trial % 17 == 0 ? 0u : 1u);
        for (uint32_t i = 0; i < 3; ++i)
            wrf32(pos + 4 * i, s_rnd_float());
        wrf32(size, s_rnd_float());
        }
        wr32(CASTERS, 1000u);
        X86 s = *c;
        s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0300u) | ((trial & 1) ? 0x0000u : 0x0200u));
        if (trial % 13 == 0)
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0c00u) | ((s_rnd() % 4u) << 10));
        if (dc) /* both precisions, nearest rounding */
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0f00u) | ((trial & 1) ? 0x0000u : 0x0200u));
        s.fpu_sw = 0;
        s.r[R_EAX] = s_rnd();
        s.r[R_EBX] = s_rnd();
        s.r[R_ESI] = s_rnd();
        s.r[R_EDI] = s_rnd();
        s.r[R_EBP] = s_rnd();
        const uint32_t esp = stack;
        wr32(esp, ret);
        wr32(esp + 4, pos);
        wr32(esp + 8, size);
        uint8_t *snapshot = (uint8_t *)malloc(len);
        memcpy(snapshot, g_mem + base, len);
        X86 a = s, b = s;
        a.r[R_ESP] = b.r[R_ESP] = esp;
        g_sc_native = 0;
        fn_006d7a40(&a);
        const uint32_t a_casters = rd32(CASTERS);
        memcpy(g_mem + base, snapshot, len);
        wr32(CASTERS, 1000u);
        g_sc_native = 1;
        const int outcome = caster_native(&b, rd32(esp + 4), rd32(esp + 8));
        b.eip = rd32(esp);
        b.r[R_ESP] = esp + 4;
        const uint32_t b_casters = rd32(CASTERS);
        memcpy(g_mem + base, snapshot, len);
        free(snapshot);
        outcomes[outcome] += 1;
        const int sw_diff = (a.fpu_sw & 0x4705) != (b.fpu_sw & 0x4705);
        int diff = (a.r[R_EAX] & 0xffu) != (b.r[R_EAX] & 0xffu) || a.r[R_ESP] != b.r[R_ESP] || a.eip != b.eip ||
                   a.fpu_top != b.fpu_top || (pass == 0 && sw_diff) || a_casters != b_casters ||
                   a.r[R_EBX] != b.r[R_EBX] || a.r[R_ESI] != b.r[R_ESI] || a.r[R_EDI] != b.r[R_EDI] ||
                   a.r[R_EBP] != b.r[R_EBP];
        sw_only += !diff && sw_diff;
        if (diff && bad < 20)
            fprintf(stderr,
                    "native self-test 006d7a40 #%d differs: eax %08x/%08x sw %04x/%04x casters %u/%u top %u/%u "
                    "outcome %d\n",
                    trial, a.r[R_EAX], b.r[R_EAX], a.fpu_sw, b.fpu_sw, a_casters, b_casters, a.fpu_top, b.fpu_top,
                    outcome);
        if (dc) {
            dbad += diff;
            if (pass == 1 && (trial & 1))
                fprintf(stderr, "native shadowcast directed: %-62s -> %s%s\n", dc->name,
                        outcome == 0 ? "shadows off" : outcome == 1 ? "in view" : outcome == 2 ? "swept hit" : "swept miss",
                        diff ? "  DIFFERS" : "");
            if (diff && bad < 20)
                fprintf(stderr, "native self-test 006d7a40 directed '%s' differs (pass %d)\n", dc->name, pass);
            bad += diff;
            continue;
        }
        bad += diff;
        ++runs;
    }
    fprintf(stderr, "native shadowcast self-test (%s): %d directed cases (x2 precisions), %d differ\n",
            pass == 0 ? "exact" : "early-out", k_sc_ncases, dbad);
    fprintf(stderr, "native shadowcast self-test (%s): %d runs, %d differ, %d differ only in FPU status bits (off %d, in view %d, hit %d, miss %d)\n",
            pass == 0 ? "exact" : "early-out", runs, bad, sw_only, outcomes[0], outcomes[1], outcomes[2], outcomes[3]);
    }
    g_sc_exact = exact_saved;
    c->fpu_cw = cw_saved;
    memcpy(g_mem + PLANES, saved_planes, sizeof saved_planes);
    memcpy(g_mem + SUN_DIR, saved_sun, sizeof saved_sun);
    wr32(SHADOWS_ON, saved_on);
    wr32(CASTERS, saved_casters);
    heap_free(base);
}
