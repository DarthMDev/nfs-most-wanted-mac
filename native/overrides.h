/* overrides.h - native replacements for hot game functions.
 *
 * Included by the generated funcs.h (game.toml [translate] native): each
 * FN_<addr> below sends every direct call to that address to the C function
 * named. The translated fn_<addr> still exists; each replacement falls back to
 * it when RECOMP_NATIVE=0, and RECOMP_NATIVE_SELFTEST=1 checks the two
 * against each other on random input at start-up. */
#ifndef NFSMW_NATIVE_OVERRIDES_H
#define NFSMW_NATIVE_OVERRIDES_H

#define FN_006c9440 native_006c9440 /* clip a segment against one plane */
#define FN_006c9510 native_006c9510 /* sphere inside all six frustum planes */
#define FN_006c9570 native_006c9570 /* swept sphere against the frustum */

/* The sound mixer thread's two hottest loops (Test73, native/audio_mix.cpp). */
#define FN_008251f0 native_008251f0 /* mix: dst[i] += src[i] * gain */
#define FN_00827790 native_00827790 /* linear-interpolation resampler */

/* The world renderer's bounding boxes (Test75, native/aabb.cpp). */
#define FN_006be800 native_006be800 /* transform a box by a matrix (in place) */
#define FN_006cf2b0 native_006cf2b0 /* box against the six view planes */

/* The scenery shadow-caster test and its two box routines (Test150, native/shadow_cast.cpp). */
#define FN_006d7a40 native_006d7a40 /* does an instance outside the view cast a shadow into it */

#endif
