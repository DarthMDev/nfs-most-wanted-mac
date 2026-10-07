/* Native equivalent of the format-1 RCMP YUV row converter, 007f91bd.
 * Verified against the pinned PC assembly: two PADDW operations (wrapping
 * signed 16-bit lanes), then PACKUSWB. Read the live lookup tables because
 * 007f94cc changes their channel order for different output formats.
 *
 * The final pair still executes the translation. This preserves its complete
 * MMX/register/flag/stack result, including caller-saved state, without guessing
 * which of those values the caller observes. No decoder clock or movie FPS is
 * changed. RECOMP_NATIVE_MOVIE=1 opts in during private validation; otherwise
 * retain the translated path. RECOMP_NATIVE=0 always disables the replacement.
 */
#include "funcs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int movie_mode = -1;
static int movie_check = -1;
static unsigned movie_check_stride;

static int range_ok(uint32_t address, uint32_t length) {
    return address < GUEST_SIZE && length <= GUEST_SIZE - address;
}

static int overlap(uint32_t a, uint32_t an, uint32_t b, uint32_t bn) {
    return (uint64_t)a < (uint64_t)b + bn && (uint64_t)b < (uint64_t)a + an;
}

static void movie_row(X86 *c) {
    const uint32_t stack = c->r[R_ESP];
    uint32_t arguments[5];
    for (unsigned i = 0; i < 5; ++i)
        arguments[i] = rd32(stack + 4 + 4 * i);
    const uint32_t y = arguments[0], u = arguments[1], v = arguments[2];
    const uint32_t dst = arguments[3], end = arguments[4];
    const uint32_t bytes = end - dst, width = bytes / 4;
    if (end <= dst || (bytes & 7) || width > 8192 || !range_ok(dst, bytes) ||
        !range_ok(y, width) || !range_ok(u, width / 2) || !range_ok(v, width / 2) ||
        overlap(dst, bytes, y, width) || overlap(dst, bytes, u, width / 2) ||
        overlap(dst, bytes, v, width / 2) || overlap(dst, bytes, 0x00908648u, 0x1800) ||
        overlap(dst, bytes, stack - 16, 40) || overlap(y, width, stack - 16, 40) ||
        overlap(u, width / 2, stack - 16, 40) || overlap(v, width / 2, stack - 16, 40)) {
        fn_007f91bd(c);
        return;
    }
    for (uint32_t x = 0; x + 2 < width; x += 2) {
        const uint32_t ut = 0x00908e48u + 8u * rd8(u + x / 2);
        const uint32_t vt = 0x00909648u + 8u * rd8(v + x / 2);
        for (uint32_t p = 0; p < 2; ++p) {
            const uint32_t yt = 0x00908648u + 8u * rd8(y + x + p);
            for (uint32_t lane = 0; lane < 4; ++lane) {
                uint16_t bits = (uint16_t)(rd16(yt + lane * 2) + rd16(ut + lane * 2) +
                                           rd16(vt + lane * 2));
                /* PACKUSWB interprets the wrapped sum as signed before clamping. */
                int value = bits < 0x8000u ? (int)bits : (int)bits - 0x10000;
                wr8(dst + (x + p) * 4 + lane, value < 0 ? 0 : value > 255 ? 255 : (uint8_t)value);
            }
        }
    }
    const uint32_t last = width - 2;
    wr32(stack + 4, y + last);
    wr32(stack + 8, u + last / 2);
    wr32(stack + 12, v + last / 2);
    wr32(stack + 16, dst + last * 4);
    fn_007f91bd(c);
    for (unsigned i = 0; i < 5; ++i)
        wr32(stack + 4 + 4 * i, arguments[i]);
}

/* Compare actual row pixels, all CPU bytes, the argument slots and saved
 * registers below ESP. Restore the input before running each implementation.
 * Sample 720 rows across 97 frames, including changing movie content rather
 * than only the initial frame. Production defaults do not run this work. */
static int check_row(X86 *c) {
    uint32_t stack = c->r[R_ESP], dst = rd32(stack + 16), end = rd32(stack + 20);
    if (end <= dst || end - dst > 32768 || !range_ok(dst, end - dst))
        return 0;
    uint32_t bytes = end - dst;
    uint8_t *before = malloc(bytes), *expected = malloc(bytes);
    if (!before || !expected) {
        free(before);
        free(expected);
        return 1;
    }
    uint8_t stack_before[40], stack_after[40];
    memcpy(stack_before, g_mem + stack - 16, sizeof stack_before);
    memcpy(before, g_mem + dst, bytes);
    X86 translated = *c, native = *c;
    fn_007f91bd(&translated);
    memcpy(expected, g_mem + dst, bytes);
    memcpy(stack_after, g_mem + stack - 16, sizeof stack_after);
    memcpy(g_mem + dst, before, bytes);
    memcpy(g_mem + stack - 16, stack_before, sizeof stack_before);
    movie_row(&native);
    int differs = memcmp(expected, g_mem + dst, bytes) != 0 ||
                  memcmp(&translated, &native, sizeof translated) != 0 ||
                  memcmp(stack_after, g_mem + stack - 16, sizeof stack_after) != 0;
    memcpy(g_mem + dst, before, bytes);
    memcpy(g_mem + stack - 16, stack_before, sizeof stack_before);
    free(before);
    free(expected);
    return differs;
}

void native_007f91bd(X86 *c) {
    if (movie_mode < 0) {
        const char *all = getenv("RECOMP_NATIVE"), *mode = getenv("RECOMP_NATIVE_MOVIE");
        const char *check = getenv("RECOMP_NATIVE_MOVIE_SELFTEST");
        movie_mode = !(all && all[0] == '0') && mode && mode[0] == '1';
        movie_check = check && check[0] == '1' ? 720 : 0;
        if (movie_mode)
            fprintf(stderr, "native movie: exact LUT row converter enabled (experimental)\n");
    }
    if (!movie_mode) {
        fn_007f91bd(c);
        return;
    }
    if (movie_check > 0 && movie_check_stride++ % 97 == 0) {
        if (check_row(c)) {
            fprintf(stderr, "native movie self-test: mismatch, disabling replacement\n");
            movie_mode = movie_check = 0;
            fn_007f91bd(c);
            return;
        }
        if (!--movie_check)
            fprintf(stderr, "native movie self-test: 720 rows, pixels/CPU/stack identical\n");
    }
    movie_row(c);
}
