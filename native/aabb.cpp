/* aabb.cpp - two of the world renderer's bounding-box routines, native (Test75).
 *
 * In a pursuit the main thread spends about 5% of its time in these two x87 routines (the
 * render list calls them for every candidate object and section):
 *   006be800  cdecl (matrix, min, max): transforms the box [min, max] by the matrix's 3x3 part
 *             and translation (Arvo's method), in place: min/max become the transformed box
 *   006cf2b0  thiscall (view; min, max, matrix or 0) -> 0 outside, 1 crossing, 2 inside: the box,
 *             optionally transformed first, against the six planes at [*view] + 0x140
 * Translated, every x87 instruction is a register-stack operation and every integer instruction
 * computes its flags. Here the same arithmetic runs on host doubles, in the same order and through
 * the same x86.h helpers (fx87, fto_float, fcom), so every stored float, every register the
 * originals leave and every status-word bit matches the translation. The guest stack below the
 * caller's ESP, which the originals use as scratch, is left alone; the one stack write above it
 * (006be800's scratch float in its first argument slot) is made as the original makes it.
 *
 * RECOMP_NATIVE=0 runs the translations instead (the same switch as culling.c and audio_mix.cpp).
 * RECOMP_NATIVE_SELFTEST=1 runs both on random input the first time one is called and reports any
 * difference on stderr. TEST-ONLY in-run A/B: RECOMP_AB_SWITCHES=aabb. */
#include "runtime/guest.h"
extern "C" {
#include "funcs.h"
}
#include "runtime/memory.h"
#include "platform/ab_phase.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void native_006be800(X86 *c);
extern "C" void native_006cf2b0(X86 *c);

static void aabb_selftest(X86 *c);
static int g_aabb_native = -1;

static int aabb_native_on(X86 *c) {
    if (__builtin_expect(g_aabb_native >= 0, 1))
        return g_aabb_native && !recomp_ab::old(recomp_ab::AABB);
    const char *e = getenv("RECOMP_NATIVE");
    int want = !(e && e[0] == '0');
    const char *t = getenv("RECOMP_NATIVE_SELFTEST");
    if (t && t[0] == '1')
        aabb_selftest(c);
    g_aabb_native = want;
    return g_aabb_native;
}

static const uint32_t K_ZERO = 0x00890968u; /* float 0.0 */
static const uint32_t K_HALF = 0x008933d4u; /* float 0.5 */

/* TEST AH,0x5 ; JP: taken when C0 and C2 are both clear or both set (even parity). */
static inline bool jp_c0c2(const X86 *c) {
    const uint32_t ah = (uint32_t)(fstsw(c) >> 8) & 0x5u;
    return ah == 0 || ah == 0x5u;
}

/* ---- 006be800 ---------------------------------------------------------------
 * Accumulators as the original keeps them on the FPU stack: the new minimum (a, b, cc) and the
 * new maximum (f, e, d) start from the translation row (matrix + 0x30). For each row i and axis k
 * the two products min[i]*m[i][k] and max[i]*m[i][k] are compared (the second after a round trip
 * through the scratch float) and the smaller goes to the minimum, the larger to the maximum.
 * Returns the low 16 bits the last FNSTSW left in AX. */
static uint16_t box_transform(X86 *c, uint32_t esp, uint32_t m, uint32_t pmin, uint32_t pmax) {
    double a = rdf32(m + 0x30), b = rdf32(m + 0x34), cc = rdf32(m + 0x38);
    double d = rdf32(m + 0x38), e = rdf32(m + 0x34), f = rdf32(m + 0x30);
    const uint32_t scratch = esp + 4; /* [ESP + 0x20] inside the original: its first argument slot */
    uint16_t sw = 0;
    for (uint32_t i = 0; i < 3; ++i) {
        const uint32_t row = m + 16u * i;
        const double lo = rdf32(pmin + 4u * i);
        const double hi_in = rdf32(pmax + 4u * i);
        /* x: row[0] */
        double t1 = fx87(c, lo * (double)rdf32(row));
        double t2 = fx87(c, hi_in * (double)rdf32(row));
        float s = fto_float(c, t2);
        wrf32(scratch, s);
        fcom(c, t1, (double)s);
        if (!jp_c0c2(c)) { /* t1 < s */
            a = fx87(c, a + t1);
            f = fx87(c, f + (double)s);
        } else {
            a = fx87(c, a + (double)s);
            f = fx87(c, f + t1);
        }
        /* y: row[1] */
        double u1 = fx87(c, lo * (double)rdf32(row + 4));
        double u2 = fx87(c, hi_in * (double)rdf32(row + 4));
        float s2 = fto_float(c, u2);
        wrf32(scratch, s2);
        fcom(c, u1, (double)s2);
        if (!jp_c0c2(c)) {
            b = fx87(c, b + u1);
            e = fx87(c, e + (double)s2);
        } else {
            b = fx87(c, b + (double)s2);
            e = fx87(c, e + u1);
        }
        /* z: row[2] */
        double w1 = fx87(c, lo * (double)rdf32(row + 8));
        double w2 = fx87(c, hi_in * (double)rdf32(row + 8));
        float s3 = fto_float(c, w2);
        wrf32(scratch, s3);
        fcom(c, w1, (double)s3);
        /* The FNSTSW here runs with seven values on the stack. */
        {
            const uint32_t top = c->fpu_top;
            c->fpu_top = (top - 7u) & 7u;
            sw = fstsw(c);
            c->fpu_top = top;
        }
        if (!jp_c0c2(c)) {
            cc = fx87(c, cc + w1);
            d = fx87(c, d + (double)s3);
        } else {
            cc = fx87(c, cc + (double)s3);
            d = fx87(c, d + w1);
        }
    }
    wrf32(pmin, fto_float(c, a));
    wrf32(pmin + 4, fto_float(c, b));
    wrf32(pmin + 8, fto_float(c, cc));
    wrf32(pmax, fto_float(c, f));
    wrf32(pmax + 4, fto_float(c, e));
    wrf32(pmax + 8, fto_float(c, d));
    return sw;
}

extern "C" void native_006be800(X86 *c) {
    if (!aabb_native_on(c)) {
        fn_006be800(c);
        return;
    }
    const uint32_t esp = c->r[R_ESP];
    const uint32_t m = rd32(esp + 4), pmin = rd32(esp + 8), pmax = rd32(esp + 12);
    const uint16_t sw = box_transform(c, esp, m, pmin, pmax);
    c->r[R_EAX] = (m & 0xffff0000u) | sw;
    c->r[R_ECX] = pmin + 12u;
    c->r[R_EDX] = m + 0x38u;
    c->eip = rd32(esp);
    c->r[R_ESP] = esp + 4;
}

/* ---- 006cf2b0 ---------------------------------------------------------------
 * The box is copied (and transformed when a matrix is given), its centre and half-extent are
 * rounded to floats as the original stores them, and each plane (nx, ny, nz, dist) is tested:
 * radius = |ny|*ey + |nz|*ez + |nx|*ex, distance = cy*ny + cz*nz + cx*nx + dist. distance + radius
 * below zero: outside (0). distance - radius below zero on any plane: crossing (1). Else inside (2). */
extern "C" void native_006cf2b0(X86 *c) {
    if (!aabb_native_on(c)) {
        fn_006cf2b0(c);
        return;
    }
    const uint32_t esp = c->r[R_ESP];
    const uint32_t view = c->r[R_ECX];
    const uint32_t pmin = rd32(esp + 4), pmax = rd32(esp + 8), mat = rd32(esp + 12);
    float mn[3] = {rdf32(pmin), rdf32(pmin + 4), rdf32(pmin + 8)};
    float mx[3] = {rdf32(pmax), rdf32(pmax + 4), rdf32(pmax + 8)};
    if (mat) {
        /* The original passes copies in its own frame; the transform's scratch argument slot is in
         * that frame too, below the caller's stack. Do the same in a frame below ours. */
        const uint32_t frame = ((esp - 4u) & 0xfffffff0u) - 0x3cu - 4u - 0x40u;
        const uint32_t cmin = frame + 0x10u, cmax = frame + 0x20u;
        for (int k = 0; k < 3; ++k) {
            wrf32(cmin + 4u * (uint32_t)k, mn[k]);
            wrf32(cmax + 4u * (uint32_t)k, mx[k]);
        }
        box_transform(c, frame, mat, cmin, cmax);
        for (int k = 0; k < 3; ++k) {
            mn[k] = rdf32(cmin + 4u * (uint32_t)k);
            mx[k] = rdf32(cmax + 4u * (uint32_t)k);
        }
    }
    const double half = rdf32(K_HALF), zero = rdf32(K_ZERO);
    float cent[3];
    for (int k = 0; k < 3; ++k) {
        const double sum = fx87(c, (double)mx[k] + (double)mn[k]);
        const double mid = fx87(c, sum * half);
        cent[k] = fto_float(c, mid);
    }
    const float cx = cent[0], cy = cent[1], cz = cent[2];
    const float ex = fto_float(c, fx87(c, (double)mx[0] - (double)cx));
    const float ey = fto_float(c, fx87(c, (double)mx[1] - (double)cy));
    const float ez = fto_float(c, fx87(c, (double)mx[2] - (double)cz));
    uint32_t p = rd32(view) + 0x144u; /* each plane's ny */
    uint32_t crossing = 0;
    uint32_t edx = 1;
    for (;;) {
        const double ny = rdf32(p), nz = rdf32(p + 4), nx = rdf32(p - 4), nd = rdf32(p + 8);
        /* One helper call per statement: the order of the calls is the original's. */
        const double ra = fx87(c, fabs(ny) * (double)ey);
        const double rb = fx87(c, fabs(nz) * (double)ez);
        double r = fx87(c, ra + rb);
        const double anx = fabs(nx);
        (void)fto_float(c, anx); /* FST to scratch: the store's status effects */
        const double rd = fx87(c, anx * (double)ex);
        r = fx87(c, r + rd);
        const double da = fx87(c, (double)cy * ny);
        const double db = fx87(c, (double)cz * nz);
        double dist = fx87(c, da + db);
        const double dc = fx87(c, (double)cx * nx);
        dist = fx87(c, dist + dc);
        dist = fx87(c, dist + nd);
        const double outer = fx87(c, dist + r);
        fcom(c, outer, zero);
        if (!jp_c0c2(c)) { /* outside this plane */
            c->r[R_EAX] = 0;
            c->r[R_ECX] = p;
            c->r[R_EDX] = edx;
            c->eip = rd32(esp);
            c->r[R_ESP] = esp + 16;
            return;
        }
        const double inner = fx87(c, dist - r);
        fcom(c, inner, zero);
        if (!jp_c0c2(c))
            crossing = 1;
        ++edx;
        p += 16;
        if ((int32_t)edx > 6)
            break;
    }
    c->r[R_EAX] = crossing ? 1u : 2u;
    c->r[R_ECX] = p;
    c->r[R_EDX] = edx;
    c->eip = rd32(esp);
    c->r[R_ESP] = esp + 16;
}

/* ---- self-test --------------------------------------------------------------
 * The translation and the replacement on the same random boxes, matrices and planes, in a heap
 * block: every byte outside the 64-byte scratch below ESP (and 006cf2b0's own frame below that),
 * all eight registers, ESP/EIP, the FPU top and the condition/IE status bits must agree. */
static uint32_t b_rnd_state = 0x2468ace1u;
static uint32_t b_rnd(void) {
    b_rnd_state = b_rnd_state * 1664525u + 1013904223u;
    return b_rnd_state;
}
static float b_rnd_float(void) {
    uint32_t k = b_rnd() % 128;
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
    return ((float)(int32_t)b_rnd() / 2147483648.0f) * (k < 64 ? 2.0f : 5000.0f);
}

typedef void (*BoxFn)(X86 *);

static int box_compare(X86 *c, BoxFn translated, BoxFn native, uint32_t base, uint32_t len, uint32_t esp,
                       uint32_t scratch_below, const char *name, int trial) {
    uint8_t *snapshot = (uint8_t *)malloc(len), *after = (uint8_t *)malloc(len);
    memcpy(snapshot, g_mem + base, len);
    X86 a = *c, b = *c;
    a.r[R_ESP] = b.r[R_ESP] = esp;
    g_aabb_native = 0;
    translated(&a);
    memcpy(after, g_mem + base, len);
    memcpy(g_mem + base, snapshot, len);
    g_aabb_native = 1;
    native(&b);
    const uint32_t scratch_lo = esp - scratch_below - base, scratch_hi = esp - base;
    memset(after + scratch_lo, 0, scratch_hi - scratch_lo);
    memset(g_mem + base + scratch_lo, 0, scratch_hi - scratch_lo);
    int bad = memcmp(after, g_mem + base, len) != 0 || a.r[R_ESP] != b.r[R_ESP] || a.eip != b.eip ||
              a.fpu_top != b.fpu_top || (a.fpu_sw & 0x4705) != (b.fpu_sw & 0x4705);
    for (int r = 0; r < 8; ++r)
        bad |= a.r[r] != b.r[r];
    if (bad) {
        uint32_t first = 0;
        while (first < len && after[first] == g_mem[base + first])
            ++first;
        fprintf(stderr,
                "native self-test %s #%d differs: eax %08x/%08x ecx %08x/%08x edx %08x/%08x esp %08x/%08x "
                "sw %04x/%04x first byte +%u\n",
                name, trial, a.r[R_EAX], b.r[R_EAX], a.r[R_ECX], b.r[R_ECX], a.r[R_EDX], b.r[R_EDX], a.r[R_ESP],
                b.r[R_ESP], a.fpu_sw, b.fpu_sw, first);
    }
    memcpy(g_mem + base, snapshot, len);
    free(snapshot);
    free(after);
    return bad;
}

static void aabb_selftest(X86 *c) {
    const uint32_t len = 0x20000;
    const uint32_t base = heap_alloc(len, true);
    if (!base) {
        fprintf(stderr, "native aabb self-test: no scratch block\n");
        return;
    }
    const uint32_t mat = base + 0x100, bmin = base + 0x200, bmax = base + 0x220, view = base + 0x300,
                   frustum = base + 0x400, stack = base + 0x10000;
    uint32_t ret = 0x00401001u;
    while (recomp_index_of(ret) >= 0 || recomp_module_lookup(ret) >= 0)
        ++ret;
    const uint16_t cw_saved = c->fpu_cw;
    int bad = 0, runs = 0;
    for (int trial = 0; trial < 3000; ++trial) {
        for (uint32_t i = 0; i < 16; ++i)
            wrf32(mat + 4 * i, b_rnd_float());
        for (uint32_t i = 0; i < 3; ++i) {
            float lo = b_rnd_float(), hi = b_rnd_float();
            if (trial % 3 && lo > hi) { /* mostly ordered boxes, sometimes not */
                float t = lo;
                lo = hi;
                hi = t;
            }
            wrf32(bmin + 4 * i, lo);
            wrf32(bmax + 4 * i, hi);
        }
        for (uint32_t i = 0; i < 6 * 4; ++i)
            wrf32(frustum + 0x140 + 4 * i, b_rnd_float() * (trial % 5 == 0 ? 0.001f : 1.0f));
        wr32(view, frustum);
        X86 s = *c;
        if (trial & 1)
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0300u) | 0x0000u);
        else
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0300u) | 0x0200u);
        if (trial % 13 == 0)
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0c00u) | ((b_rnd() % 4u) << 10));
        s.fpu_sw = 0;
        s.r[R_EAX] = b_rnd();
        s.r[R_EBX] = b_rnd();
        s.r[R_ESI] = b_rnd();
        s.r[R_EDI] = b_rnd();
        s.r[R_EBP] = b_rnd();
        uint32_t esp = stack;
        /* 006be800(mat, bmin, bmax) */
        wr32(esp, ret);
        wr32(esp + 4, mat);
        wr32(esp + 8, bmin);
        wr32(esp + 12, bmax);
        bad += box_compare(&s, fn_006be800, native_006be800, base, len, esp, 64, "006be800", trial);
        /* 006cf2b0(view; bmin, bmax, mat or 0) */
        s.r[R_ECX] = view;
        wr32(esp, ret);
        wr32(esp + 4, bmin);
        wr32(esp + 8, bmax);
        wr32(esp + 12, trial % 4 == 0 ? 0u : mat);
        bad += box_compare(&s, fn_006cf2b0, native_006cf2b0, base, len, esp, 0x200, "006cf2b0", trial);
        runs += 2;
    }
    c->fpu_cw = cw_saved;
    heap_free(base);
    fprintf(stderr, "native aabb self-test: %d runs, %d differ\n", runs, bad);
}
