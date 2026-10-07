/* Test113 module F, host side: the clean-room light pools' data and per-draw light selection (contract v0.2
 * section 10). PC data only: clean-pools.bin (test113 pools_bin.py: the player's own PC lamp scenery with the
 * clean side's class table) in the mod folder. The shading is the clean side's (clean-world.fx /
 * clean-worldreflect.fx, an added pass on copies of the PC game's own effects). Owner setting clean_pools.
 * Pure part (no guest access) below; the draw hooks are in nfsmw.c next to the Test76 pass they share. */
#ifndef CLEAN_POOLS_H
#define CLEAN_POOLS_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CPL_MAX 8u            /* lights per draw (clean side: N = 8, CleanPools[16]) */
#define CPL_CELL 32.0f        /* grid cell, metres */

typedef struct { float p[3], radius, col[3], d0; uint32_t section, kind; } CplLight;
typedef struct {
    CplLight *l;
    uint32_t n;
    float params[4];
    float maxr, x0, y0;
    uint32_t gw, gh;
    uint32_t *cell_first, *cell_items;   /* CSR grid: lights whose sphere touches the cell */
} Cpl;

static void cpl_free(Cpl *s) {
    free(s->l); free(s->cell_first); free(s->cell_items);
    memset(s, 0, sizeof *s);
}

static int cpl_parse(Cpl *s, const uint8_t *b, size_t n) {
    memset(s, 0, sizeof *s);
    if (n < 24 || memcmp(b, "CPL1", 4)) return 0;
    uint32_t cnt; memcpy(&cnt, b + 4, 4); memcpy(s->params, b + 8, 16);
    if (cnt == 0 || cnt > 100000 || n != 24 + (size_t)cnt * sizeof(CplLight)) return 0;
    s->l = (CplLight *)malloc(cnt * sizeof(CplLight));
    if (!s->l) return 0;
    memcpy(s->l, b + 24, cnt * sizeof(CplLight));
    s->n = cnt;
    float x0 = 1e30f, y0 = 1e30f, x1 = -1e30f, y1 = -1e30f;
    for (uint32_t i = 0; i < cnt; ++i) {
        const CplLight *L = &s->l[i];
        if (!(L->radius > 0.0f && L->radius < 1000.0f) || !isfinite(L->p[0]) || !isfinite(L->p[1]) || !isfinite(L->p[2])) { cpl_free(s); return 0; }
        if (L->radius > s->maxr) s->maxr = L->radius;
        if (L->p[0] - L->radius < x0) x0 = L->p[0] - L->radius;
        if (L->p[1] - L->radius < y0) y0 = L->p[1] - L->radius;
        if (L->p[0] + L->radius > x1) x1 = L->p[0] + L->radius;
        if (L->p[1] + L->radius > y1) y1 = L->p[1] + L->radius;
    }
    s->x0 = x0; s->y0 = y0;
    s->gw = (uint32_t)((x1 - x0) / CPL_CELL) + 1; s->gh = (uint32_t)((y1 - y0) / CPL_CELL) + 1;
    if ((uint64_t)s->gw * s->gh > 4u * 1024 * 1024) { cpl_free(s); return 0; }
    const uint32_t cells = s->gw * s->gh;
    s->cell_first = (uint32_t *)calloc(cells + 1, sizeof(uint32_t));
    if (!s->cell_first) { cpl_free(s); return 0; }
    /* two passes: count, then fill */
    for (int pass = 0; pass < 2; ++pass) {
        uint32_t *fill = NULL;
        if (pass == 1) {
            for (uint32_t c = 0; c < cells; ++c) s->cell_first[c + 1] += s->cell_first[c];
            s->cell_items = (uint32_t *)malloc((s->cell_first[cells] ? s->cell_first[cells] : 1) * sizeof(uint32_t));
            fill = (uint32_t *)calloc(cells, sizeof(uint32_t));
            if (!s->cell_items || !fill) { free(fill); cpl_free(s); return 0; }
        }
        for (uint32_t i = 0; i < cnt; ++i) {
            const CplLight *L = &s->l[i];
            const uint32_t cx0 = (uint32_t)((L->p[0] - L->radius - x0) / CPL_CELL), cx1 = (uint32_t)((L->p[0] + L->radius - x0) / CPL_CELL);
            const uint32_t cy0 = (uint32_t)((L->p[1] - L->radius - y0) / CPL_CELL), cy1 = (uint32_t)((L->p[1] + L->radius - y0) / CPL_CELL);
            for (uint32_t cy = cy0; cy <= cy1 && cy < s->gh; ++cy)
                for (uint32_t cx = cx0; cx <= cx1 && cx < s->gw; ++cx) {
                    const uint32_t c = cy * s->gw + cx;
                    if (pass == 0) ++s->cell_first[c + 1];
                    else s->cell_items[s->cell_first[c] + fill[c]++] = i;
                }
        }
        free(fill);
    }
    return 1;
}

/* Squared distance from point p to the box [lo, hi]. */
static float cpl_box_d2(const float *p, const float *lo, const float *hi) {
    float d2 = 0.0f;
    for (int k = 0; k < 3; ++k) {
        const float v = p[k] < lo[k] ? lo[k] - p[k] : p[k] > hi[k] ? p[k] - hi[k] : 0.0f;
        d2 += v * v;
    }
    return d2;
}

/* Lights whose sphere intersects the world box, nearest to `eye` first (eye NULL: nearest to the box centre),
 * at most CPL_MAX. Returns the count; idx receives the light numbers. */
static uint32_t cpl_select(const Cpl *s, const float *lo, const float *hi, const float *eye, uint32_t *idx) {
    if (!s->n || !(hi[0] >= lo[0] && hi[1] >= lo[1] && hi[2] >= lo[2])) return 0;
    float centre[3] = {(lo[0] + hi[0]) * 0.5f, (lo[1] + hi[1]) * 0.5f, (lo[2] + hi[2]) * 0.5f};
    const float *ref = eye ? eye : centre;
    float keyv[CPL_MAX];
    uint32_t m = 0;
    if (lo[0] - s->x0 > (float)s->gw * CPL_CELL || hi[0] < s->x0 || lo[1] - s->y0 > (float)s->gh * CPL_CELL || hi[1] < s->y0) return 0;
    const int cx0 = (int)((lo[0] - s->x0) / CPL_CELL), cx1 = (int)((hi[0] - s->x0) / CPL_CELL);
    const int cy0 = (int)((lo[1] - s->y0) / CPL_CELL), cy1 = (int)((hi[1] - s->y0) / CPL_CELL);
    for (int cy = cy0 < 0 ? 0 : cy0; cy <= cy1 && cy < (int)s->gh; ++cy)
        for (int cx = cx0 < 0 ? 0 : cx0; cx <= cx1 && cx < (int)s->gw; ++cx) {
            const uint32_t c = (uint32_t)cy * s->gw + (uint32_t)cx;
            for (uint32_t k = s->cell_first[c]; k < s->cell_first[c + 1]; ++k) {
                const uint32_t i = s->cell_items[k];
                const CplLight *L = &s->l[i];
                if (cpl_box_d2(L->p, lo, hi) >= L->radius * L->radius) continue;
                int dup = 0;
                for (uint32_t j = 0; j < m; ++j) if (idx[j] == i) { dup = 1; break; }
                if (dup) continue;
                const float dx = L->p[0] - ref[0], dy = L->p[1] - ref[1], dz = L->p[2] - ref[2];
                const float key = dx * dx + dy * dy + dz * dz;
                /* insertion into the sorted top-CPL_MAX list */
                uint32_t at = m;
                while (at > 0 && keyv[at - 1] > key) --at;
                if (at >= CPL_MAX) continue;
                const uint32_t last = m < CPL_MAX ? m : CPL_MAX - 1;
                for (uint32_t j = last; j > at; --j) { idx[j] = idx[j - 1]; keyv[j] = keyv[j - 1]; }
                idx[at] = i; keyv[at] = key;
                if (m < CPL_MAX) ++m;
            }
        }
    return m;
}

/* The CleanPools block for one draw (world space; the caller moves positions into the draw's space):
 * [2i] = position, radius; [2i+1] = colour x intensity x activation, d0. Unused slots are zero. */
static void cpl_block(const Cpl *s, const uint32_t *idx, uint32_t m, float out[2 * CPL_MAX][4]) {
    memset(out, 0, sizeof(float) * 8 * CPL_MAX);
    for (uint32_t j = 0; j < m; ++j) {
        const CplLight *L = &s->l[idx[j]];
        out[2 * j][0] = L->p[0]; out[2 * j][1] = L->p[1]; out[2 * j][2] = L->p[2]; out[2 * j][3] = L->radius;
        out[2 * j + 1][0] = L->col[0]; out[2 * j + 1][1] = L->col[1]; out[2 * j + 1][2] = L->col[2]; out[2 * j + 1][3] = L->d0;
    }
}
#endif
