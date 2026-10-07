/* audio_mix.cpp - two of the EA sound mixer's inner loops, native (Test73).
 *
 * The game's sound thread (started at 00820bd7) mixes every voice in software.
 * The guest runs one thread at a time, so while it mixes, the main thread waits:
 * in a pursuit (sirens, radio, more engines) the profile shows the main thread
 * waiting for it about a tenth of the time. Its two hottest leaves are plain
 * x87 loops:
 *   008251f0  cdecl (count, gain, src, dst): dst[i] += src[i] * gain
 *   00827790  cdecl (count, src, dst, &index, &frac, step, step_frac): linear
 *             interpolation resampler, 32.32 fixed-point position
 * Translated, each x87 instruction is a register-stack operation and every
 * flag of every integer instruction is computed. Here the same arithmetic
 * runs on host doubles, in the same order and through the same x86.h helpers
 * (fx87, fto_float), with the same guest memory reads and writes in the same
 * order, so every stored float and status-word bit matches the translation.
 * Registers are left as the originals leave them (both save and restore every
 * register they use); the guest stack below the caller's ESP, which the
 * originals use as scratch, is left alone.
 *
 * RECOMP_NATIVE=0 runs the translations instead (shared with culling.c's
 * switch through the same environment variable). RECOMP_NATIVE_SELFTEST=1
 * runs both on random input the first time one is called and reports any
 * difference on stderr. */
#include "runtime/guest.h"
extern "C" {
#include "funcs.h"
}
#include "runtime/memory.h"
#include "platform/ab_phase.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" void native_008251f0(X86 *c);
extern "C" void native_00827790(X86 *c);

static void audio_selftest(X86 *c);
static int g_audio_native = -1;

static int audio_native_on(X86 *c) {
    if (__builtin_expect(g_audio_native >= 0, 1))
        return g_audio_native && !recomp_ab::old(recomp_ab::AUDIO); /* TEST ONLY in-run A/B */
    const char *e = getenv("RECOMP_NATIVE");
    int want = !(e && e[0] == '0');
    const char *t = getenv("RECOMP_NATIVE_SELFTEST");
    if (t && t[0] == '1')
        audio_selftest(c);
    g_audio_native = want;
    return g_audio_native;
}

/* ---- 008251f0: dst[i] += src[i] * gain, i = count-1 down to 0 ----------------
 * Odd elements one at a time from the top until the count is a multiple of
 * four, then blocks of four (all four src reads, all four dst reads, then the
 * stores in the order -4, -3, -1, -2). A count of zero still runs one block,
 * at indices -4..-1, exactly as the original does. */
extern "C" void native_008251f0(X86 *c) {
    if (!audio_native_on(c)) {
        fn_008251f0(c);
        return;
    }
    const uint32_t esp = c->r[R_ESP];
    uint32_t n = rd32(esp + 4);
    const double g = (double)rdf32(esp + 8);
    const uint32_t src = rd32(esp + 12), dst = rd32(esp + 16);
    while (n & 3u) {
        const uint32_t o = n * 4u - 4u;
        const double p = fx87(c, (double)rdf32(src + o) * g);
        const double s = fx87(c, p + (double)rdf32(dst + o));
        wrf32(dst + o, fto_float(c, s));
        n -= 1u;
        if (n == 0)
            goto done;
    }
    for (;;) {
        const uint32_t o = n * 4u;
        const double a0 = fx87(c, (double)rdf32(src + o - 16u) * g);
        const double a1 = fx87(c, (double)rdf32(src + o - 12u) * g);
        const double a2 = fx87(c, (double)rdf32(src + o - 8u) * g);
        const double a3 = fx87(c, (double)rdf32(src + o - 4u) * g);
        const double b0 = fx87(c, a0 + (double)rdf32(dst + o - 16u));
        const double b1 = fx87(c, a1 + (double)rdf32(dst + o - 12u));
        const double b2 = fx87(c, a2 + (double)rdf32(dst + o - 8u));
        const double b3 = fx87(c, a3 + (double)rdf32(dst + o - 4u));
        wrf32(dst + o - 16u, fto_float(c, b0));
        wrf32(dst + o - 12u, fto_float(c, b1));
        wrf32(dst + o - 4u, fto_float(c, b3));
        wrf32(dst + o - 8u, fto_float(c, b2));
        const int64_t next = (int64_t)(int32_t)n - 4; /* SUB ECX,4; JG: signed, overflow-aware */
        n -= 4u;
        if (next <= 0)
            break;
    }
done:
    c->eip = rd32(esp);
    c->r[R_ESP] = esp + 4u;
}

/* ---- 00827790: linear-interpolation resampler --------------------------------
 * out = (src[i+1] - src[i]) * frac64 * scale + src[i], with frac64 the qword at
 * 0090c8c0 (its low half is the running 32-bit fraction, stored before every
 * sample; scale at 0090c8c8 is 2^-32), then frac += step_frac and
 * i += step + carry. count >= 2 makes count samples, otherwise count & 1. */
extern "C" void native_00827790(X86 *c) {
    if (!audio_native_on(c)) {
        fn_00827790(c);
        return;
    }
    const uint32_t esp = c->r[R_ESP];
    const int32_t n = (int32_t)rd32(esp + 4);
    const uint32_t src = rd32(esp + 8);
    uint32_t dst = rd32(esp + 12);
    const uint32_t p_index = rd32(esp + 16), p_frac = rd32(esp + 20);
    const uint32_t step = rd32(esp + 24), step_frac = rd32(esp + 28);
    uint32_t frac = rd32(p_frac);
    wr32(0x0090c8c0u, frac);
    uint32_t index = rd32(p_index);
    const uint32_t count = n >= 2 ? (uint32_t)n : ((uint32_t)n & 1u);
    for (uint32_t k = 0; k < count; ++k) {
        const uint32_t at = src + index * 4u;
        const double d = fx87(c, (double)rdf32(at + 4u) - (double)rdf32(at));
        const double q = (double)(int64_t)rd64(0x0090c8c0u);
        double v = fx87(c, d * q);
        v = fx87(c, v * (double)rdf32(0x0090c8c8u));
        const uint64_t sum = (uint64_t)frac + step_frac;
        frac = (uint32_t)sum;
        v = fx87(c, v + (double)rdf32(at));
        index = index + step + (uint32_t)(sum >> 32);
        dst += 4u;
        wr32(0x0090c8c0u, frac);
        wrf32(dst - 4u, fto_float(c, v));
    }
    wr32(p_frac, frac);
    wr32(p_index, index);
    c->eip = rd32(esp);
    c->r[R_ESP] = esp + 4u;
}

/* ---- self-test -------------------------------------------------------------
 * Both forms on the same random buffers, counts, gains, steps and precision
 * control, in scratch memory below the current stack. Compared: every byte of
 * the scratch region, the 0090c8c0 qword, ESP/EIP, the saved registers, the
 * FPU top and the status word's condition/exception bits. */
static uint32_t a_rnd_state = 0x9e3779b9u;
static uint32_t a_rnd(void) {
    a_rnd_state = a_rnd_state * 1664525u + 1013904223u;
    return a_rnd_state;
}
static float a_rnd_float(void) {
    uint32_t k = a_rnd() % 128;
    if (k == 0)
        return 0.0f;
    if (k == 1)
        return -0.0f;
    if (k == 2)
        return 3e38f;
    if (k == 3)
        return (float)__builtin_nan("");
    if (k == 4)
        return 1e-40f; /* denormal */
    return ((float)(int32_t)a_rnd() / 2147483648.0f) * (k < 64 ? 1.0f : 40000.0f);
}

typedef void (*AudioFn)(X86 *);

static int audio_compare(X86 *c, AudioFn translated, AudioFn native, uint32_t base, uint32_t len, uint32_t esp,
                         const char *name, int trial) {
    uint8_t *snapshot = (uint8_t *)malloc(len), *after = (uint8_t *)malloc(len);
    uint8_t q_before[8], q_after[8];
    memcpy(snapshot, g_mem + base, len);
    memcpy(q_before, g_mem + 0x0090c8c0u, 8);
    X86 a = *c, b = *c;
    a.r[R_ESP] = b.r[R_ESP] = esp;
    g_audio_native = 0;
    translated(&a);
    memcpy(after, g_mem + base, len);
    memcpy(q_after, g_mem + 0x0090c8c0u, 8);
    memcpy(g_mem + base, snapshot, len);
    memcpy(g_mem + 0x0090c8c0u, q_before, 8);
    g_audio_native = 1;
    native(&b);
    /* The originals push their saved registers below ESP; that scratch is nobody's. */
    const uint32_t scratch_lo = esp - 64u - base, scratch_hi = esp - base;
    memset(after + scratch_lo, 0, scratch_hi - scratch_lo);
    memset(g_mem + base + scratch_lo, 0, scratch_hi - scratch_lo);
    int bad = memcmp(after, g_mem + base, len) != 0 || memcmp(q_after, g_mem + 0x0090c8c0u, 8) != 0 ||
              a.r[R_ESP] != b.r[R_ESP] || a.eip != b.eip || a.fpu_top != b.fpu_top ||
              (a.fpu_sw & 0x4705) != (b.fpu_sw & 0x4705);
    for (int r = 0; r < 8; ++r)
        bad |= a.r[r] != b.r[r];
    if (bad) {
        uint32_t first = 0;
        while (first < len && after[first] == g_mem[base + first])
            ++first;
        fprintf(stderr, "native self-test %s #%d differs: esp %08x/%08x eip %08x/%08x sw %04x/%04x first byte +%u\n",
                name, trial, a.r[R_ESP], b.r[R_ESP], a.eip, b.eip, a.fpu_sw, b.fpu_sw, first);
    }
    memcpy(g_mem + base, snapshot, len);
    memcpy(g_mem + 0x0090c8c0u, q_before, 8);
    free(snapshot);
    free(after);
    return bad;
}

static void audio_selftest(X86 *c) {
    const uint32_t len = 0x20000;
    const uint32_t base = heap_alloc(len, true);
    if (!base) {
        fprintf(stderr, "native audio self-test: no scratch block\n");
        return;
    }
    const uint32_t src = base + 0x100, dst = base + 0x6000, cells = base + 0xc000, stack = base + 0x10000;
    /* A return address the translated RET treats as a plain return: not a function entry
     * (recomp_return would tail-call it), not a shim, not the callback sentinel. */
    uint32_t ret = 0x00401001u;
    while (recomp_index_of(ret) >= 0 || recomp_module_lookup(ret) >= 0)
        ++ret;
    uint8_t q_saved[8];
    memcpy(q_saved, g_mem + 0x0090c8c0u, 8);
    const uint16_t cw_saved = c->fpu_cw;
    int bad = 0, runs = 0;
    for (int trial = 0; trial < 2000; ++trial) {
        for (uint32_t i = 0; i < 0x1700; ++i) /* src and dst windows, incl. below dst */
            wrf32(src + 4u * i, a_rnd_float());
        X86 s = *c;
        if (trial & 1) /* precision control: single (as Direct3D leaves it) or the default */
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0300u) | 0x0000u);
        else
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0300u) | 0x0200u);
        if (trial % 13 == 0) /* a directed rounding mode */
            s.fpu_cw = (uint16_t)((s.fpu_cw & ~0x0c00u) | ((a_rnd() % 4u) << 10));
        s.fpu_sw = 0;
        /* 008251f0(count, gain, src, dst + 64) */
        int32_t count = trial % 17 == 0 ? -(int32_t)(a_rnd() % 7) : (int32_t)(a_rnd() % 300);
        uint32_t esp = stack;
        wr32(esp, ret);
        wr32(esp + 4, (uint32_t)count);
        wrf32(esp + 8, a_rnd_float());
        wr32(esp + 12, src + 64);
        wr32(esp + 16, dst + 64);
        bad += audio_compare(&s, fn_008251f0, native_008251f0, base, len, esp, "008251f0", trial);
        /* 00827790(count, src, dst, &index, &frac, step, step_frac) */
        count = trial % 11 == 0 ? (int32_t)(a_rnd() % 5) - 2 : (int32_t)(a_rnd() % 400);
        wr32(cells, a_rnd() % 64);     /* index */
        wr32(cells + 4, a_rnd());      /* fraction */
        wr32(0x0090c8c4u, trial % 29 == 0 ? a_rnd() % 3 : 0); /* the qword's high half */
        wr32(esp, ret);
        wr32(esp + 4, (uint32_t)count);
        wr32(esp + 8, src);
        wr32(esp + 12, dst);
        wr32(esp + 16, cells);
        wr32(esp + 20, cells + 4);
        wr32(esp + 24, a_rnd() % 3);
        wr32(esp + 28, a_rnd());
        bad += audio_compare(&s, fn_00827790, native_00827790, base, len, esp, "00827790", trial);
        runs += 2;
    }
    memcpy(g_mem + 0x0090c8c0u, q_saved, 8);
    c->fpu_cw = cw_saved;
    heap_free(base);
    fprintf(stderr, "native audio self-test: %d runs, %d differ\n", runs, bad);
}
