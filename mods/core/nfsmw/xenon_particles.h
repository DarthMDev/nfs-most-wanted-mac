/* Sparks and light trails (XenonEffects): the particle list of the NGEffect system - contrails and sparks - for
 * 'xbox_particles'. Pure: no guest or GPU access (the world collision query is a
 * callback), shared with native/tests/xenon_particle_tests.cpp.
 *
 * Behaviour from NFSMW_XenonEffects by xan1242 (Lovro Plese), MIT licence, commit b5070080 (the PC Carbon routines
 * it transplants into speed.exe 1.3): ParticleList::AgeParticles, BounceParticle, CalcCollisiontime. Reimplemented
 * in C from its x87 code; no code copied. Notice in NOTICE / docs/COMMUNITY_SOURCES.md.
 *
 * Coordinates: particles are in world space (x, y, z up). The collision query takes the game's UMath order,
 * (-y, z, x), and returns its normal in that order; the conversion is done here.
 *
 * One deliberate change: in AgeParticles a particle that bounces is kept in place in the original even when
 * particles before it in the list have died, so it is lost (and a stale one kept) after any death earlier in the
 * same pass. Here it is moved down with the rest, as every other surviving particle is. */
#ifndef XENON_PARTICLES_H
#define XENON_PARTICLES_H
#include <math.h>
#include <stdint.h>
#include <string.h>

/* NGParticle, 0x48 bytes (the original layout, kept for reference and for the sprite batch) */
typedef struct {
    float pos[3];            /* 0x00 initial position (moved to the impact point on a bounce) */
    uint32_t color;          /* 0x0c */
    float vel[3];            /* 0x10 */
    float gravity;           /* 0x1c z acceleration term: z(t) = z0 + vz t + gravity t^2 */
    float impact_normal[3];  /* 0x20 world space, set when a collision is found */
    float remaining_life;    /* 0x2c lifetime left after the next impact */
    float life;              /* 0x30 time until death, or until the impact when XP_IMPACT is set */
    float age;               /* 0x34 time since spawn or the last bounce */
    uint8_t elasticity;      /* 0x38 0..255 = 0..1 */
    uint8_t pad[3];
    uint8_t flags;           /* 0x3c XP_* */
    uint8_t rot[3];
    uint8_t size;
    uint8_t start[3];
    uint8_t uv[4];
} XenonParticle;
enum { XP_IMPACT = 2, XP_BOUNCED = 4 };

/* The world query (WCollisionMgr::CheckHitWorld, 007854b0, primitive mask 3) on the segment a -> b, each in UMath
 * order (-y, z, x, 1). Returns nonzero on a hit, with the hit point and normal in UMath order. */
typedef int (*XenonHitFn)(void *user, const float a[4], const float b[4], float hit_point[4], float hit_normal[4]);

/* CalcCollisiontime: lifts the particle 0.15 above its start, then looks along its path for the next `life`
 * seconds (the segment from 0.15 above that); on a hit, `life` becomes the time to the impact (the root of
 * dz + vz t + g t^2 = 0, dz = start height - hit height; the other root if the first is negative, 0 if none) and
 * XP_IMPACT and the impact normal are set. */
static inline void xp_collision_time(XenonParticle *p, XenonHitFn hit, void *user) {
    p->pos[2] += 0.15f;
    const float t = p->life;
    const float x1 = t * p->vel[0] + p->pos[0];
    const float y1 = t * p->vel[1] + p->pos[1];
    const float z1 = t * p->vel[2] + p->pos[2] + t * t * p->gravity;
    const float a[4] = {-p->pos[1], p->pos[2] + 0.15f, p->pos[0], 1.0f};
    const float b[4] = {-y1, z1, x1, 1.0f};
    float point[4] = {0, 0, 0, 0}, normal[4] = {0, 0, 0, 0};
    if (!hit || !hit(user, a, b, point, normal)) return;
    const float vz = p->vel[2];
    const float disc = vz * vz - (p->pos[2] - point[1]) * p->gravity * 4.0f;
    float when = 0.0f;
    if (disc > 0.0f) {
        const float s = sqrtf(disc), two_g = p->gravity + p->gravity;
        when = (s - vz) / two_g;
        if (when < 0.0f) when = (-vz - s) / two_g;   /* the later root only when the first is negative */
    }
    p->life = when;
    p->flags |= XP_IMPACT;
    p->impact_normal[0] = normal[2];
    p->impact_normal[1] = -normal[0];
    p->impact_normal[2] = normal[1];
}

/* BounceParticle: moves the particle to its impact point, reflects its velocity off the impact normal, keeps
 * elasticity/255 of its speed, restarts its age and its life from remaining_life, and looks for the next impact. */
static inline int xp_bounce(XenonParticle *p, XenonHitFn hit, void *user) {
    const float t = p->life, tg = t * p->gravity;
    p->pos[0] += t * p->vel[0];
    p->pos[1] += t * p->vel[1];
    p->pos[2] = t * p->vel[2] + p->pos[2] + tg * t;
    float d[3] = {p->vel[0], p->vel[1], p->vel[2] + tg + tg};
    const float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (len != 0.0f) {
        const float inv = 1.0f / len;
        d[0] *= inv; d[1] *= inv; d[2] *= inv;
    }
    const float *n = p->impact_normal;
    const float k = (d[2] * n[2] + d[1] * n[1] + d[0] * n[0]) * 2.0f;
    const float e = len * (float)p->elasticity * (1.0f / 255.0f);
    p->vel[0] = (d[0] - k * n[0]) * e;
    p->vel[1] = (d[1] - k * n[1]) * e;
    p->vel[2] = (d[2] - k * n[2]) * e;
    p->age = 0.0f;
    p->flags = (uint8_t)((p->flags & 1u) | XP_BOUNCED);
    p->life = p->remaining_life;
    xp_collision_time(p, hit, user);
    return 1;
}

/* ParticleList::AgeParticles(dt): a particle ages by dt while age + dt <= life. Past it, a particle with a pending
 * impact spends that time from remaining_life and bounces if more than dt is left; any other dies. Survivors are
 * packed to the front; returns the new count. */
static inline uint32_t xp_age(XenonParticle *list, uint32_t count, float dt, XenonHitFn hit, void *user) {
    uint32_t w = 0;
    for (uint32_t i = 0; i < count; ++i) {
        XenonParticle *p = &list[i];
        const float na = dt + p->age;
        if (na <= p->life) {
            if (w != i) list[w] = *p;
            list[w].age += dt;
            ++w;
        } else if (p->flags & XP_IMPACT) {
            p->remaining_life -= na;
            if (p->remaining_life > dt && xp_bounce(p, hit, user)) {
                if (w != i) list[w] = *p;
                ++w;
            }
        }
    }
    return w;
}
/* bRandom(float range, uint *seed) (0045d9e0) and bRandom(int range, uint *seed) (0045d9a0): the game's xorshift-style
 * generator. The float form returns (seed % 0x7fffffff) x range / 2^31 (the seed before the step). */
static inline uint32_t xp_rand_step(uint32_t *seed) {
    const uint32_t v = *seed, x = v ^ 0x1d872b41u, y = (x >> 5) ^ x;
    *seed = (y << 27) ^ y ^ x;
    return v;
}
static inline float xp_rand_float(float range, uint32_t *seed) {
    const uint32_t v = xp_rand_step(seed);
    return (float)((double)(v % 0x7fffffffu) * (double)range * 4.656612873077393e-10);
}
static inline uint32_t xp_rand_int(uint32_t range, uint32_t *seed) {
    if (!range) return 0;
    return xp_rand_step(seed) % range;
}
/* __ftol2 into a byte, as the original stores al */
static inline uint8_t xp_ftob(float f) { return (uint8_t)(int32_t)f; }

/* Attrib::Gen::fuelcell_emitter in the PC Carbon layout the original's bridge builds from the MW one (MW offsets up
 * to 0x80, then Elasticity from its own table). */
typedef struct {
    float volume_center[4];     /* 0x00 */
    float velocity_delta[4];    /* 0x10 */
    float volume_extent[4];     /* 0x20 */
    float velocity_inherit[4];  /* 0x30 */
    float velocity_start[4];    /* 0x40 */
    float colour1[4];           /* 0x50 */
    uint32_t uv_class, uv_collection, unk;   /* 0x60 */
    float life;                 /* 0x6c */
    float num_particles_variance;
    float gravity_start;        /* 0x74 */
    float height_start;         /* 0x78 */
    float gravity_delta;        /* 0x7c */
    float elasticity;           /* 0x80 */
    float length_start;         /* 0x84 */
    float length_delta;         /* 0x88 */
    float life_variance;        /* 0x8c */
    float num_particles;        /* 0x90 */
    uint8_t debris_type;        /* 0x94 */
    uint8_t contrail;           /* 0x95 */
} XenonEmitterData;
/* Attrib::Gen::emitteruv as the original reads it: +0 EndV, +4 StartU, +8 EndU, +0xc StartV */
typedef struct { float end_v, start_u, end_u, start_v; } XenonEmitterUV;
typedef struct {
    const XenonEmitterData *d;
    const XenonEmitterUV *uv;
    float inherit[3];        /* CGEmitter +0x20: the velocity this emitter passes on (scaled by velocity_inherit) */
    float local_world[16];   /* CGEmitter +0x30: row-major, rows are x, y, z axes and the position */
    uint8_t rot[3];          /* debris spin, attributes 28638d89, d2603865, e2cc8106 (bytes; 0 when absent) */
} XenonEmitterIn;

/* CGEmitter::SpawnParticles(dt, intensity, no_collision) (the original's 0073f... routine): spawns
 *   n = max(intensity, 1) x num_particles x (1 - 100 x num_particles_variance)
 * particles spread evenly over dt (particle k starts k dt / n into the step, already moved that far), each with
 *   size  = length_start + rand(length_delta), at most 255 (a negative size ends the spawn)
 *   vel   = (local_world 3x3 x velocity_start + velocity_inherit x inherit) x (1 - velocity_delta + 2 rand(velocity_delta))
 *   g     = gravity_start - gravity_delta + 2 rand(gravity_delta)
 *   pos   = local_world x (volume_center - volume_extent / 2 + rand(volume_extent), 1)
 *   life  = life x (1 - life_variance)    colour = ARGB of colour1 x 255, alpha min(42 intensity, 42) unless intensity is 1
 * then, unless no_collision, its first impact (xp_collision_time). Debris emitters (debris_type > 0) instead carry
 * the debris type, the u/v start cells, random start angles and the spin bytes. Two deliberate changes: the spawn
 * stops when n reaches zero or below (the original loops while n != 0, so a fractional n would run to the list's
 * limit), and a negative size releases the slot it took. Returns the number spawned. */
static inline uint32_t xp_spawn(const XenonEmitterIn *e, float dt, float intensity, int no_collision, XenonParticle *list,
                                uint32_t *count, uint32_t max, uint32_t *seed, XenonHitFn hit, void *user) {
    if (!(intensity > 0.0f) || !(dt > 0.0f)) return 0;
    const XenonEmitterData *d = e->d;
    const float *M = e->local_world;
    const float life = d->life - d->life * d->life_variance;
    const uint32_t r = (uint32_t)(int32_t)(d->colour1[0] * 255.0f), g = (uint32_t)(int32_t)(d->colour1[1] * 255.0f),
                   b = (uint32_t)(int32_t)(d->colour1[2] * 255.0f);
    uint32_t a = (uint32_t)(int32_t)(d->colour1[3] * 255.0f);
    if (intensity != 1.0f) {
        float x = intensity * 42.0f;
        if (!(42.0f > x)) x = 42.0f;
        a = (uint32_t)(int32_t)x;
    }
    const uint32_t colour = ((((a << 8) | r) << 8) | g) << 8 | b;
    const float c = intensity > 1.0f ? intensity : 1.0f;
    float n = c * d->num_particles - c * d->num_particles * d->num_particles_variance * 100.0f;
    uint8_t debris = 0, debris_type = d->debris_type;
    if (debris_type) { --debris_type; debris = 1; }
    const float spacing = dt / n;
    float t = 0.0f;
    uint32_t spawned = 0;
    while (n > 0.0f) {
        n -= 1.0f;
        if (*count >= max) break;
        XenonParticle *p = &list[(*count)++];
        float size = xp_rand_float(d->length_delta, seed) + d->length_start;
        if (size < 0.0f) { --*count; break; }
        if (!(size < 255.0f)) size = 255.0f;
        const float f0 = 1.0f - (d->velocity_delta[0] - (xp_rand_float(d->velocity_delta[0], seed) * 2.0f));
        const float f1 = 1.0f - (d->velocity_delta[1] - (xp_rand_float(d->velocity_delta[1], seed) * 2.0f));
        const float f2 = 1.0f - (d->velocity_delta[2] - (xp_rand_float(d->velocity_delta[2], seed) * 2.0f));
        const float *vs = d->velocity_start;
        const float ox = vs[0] * M[0] + vs[1] * M[4] + vs[2] * M[8];
        const float oy = vs[0] * M[1] + vs[1] * M[5] + vs[2] * M[9];
        const float oz = vs[0] * M[2] + vs[1] * M[6] + vs[2] * M[10];
        const float vx = (ox + d->velocity_inherit[0] * e->inherit[0]) * f0;
        const float vy = (oy + d->velocity_inherit[1] * e->inherit[1]) * f1;
        const float vz = (oz + d->velocity_inherit[2] * e->inherit[2]) * f2;
        const float grav = xp_rand_float(d->gravity_delta, seed) * 2.0f + (d->gravity_start - d->gravity_delta);
        float l[3];
        for (int i = 0; i < 3; ++i)
            l[i] = (xp_rand_float(d->volume_extent[i], seed) - d->volume_extent[i] * 0.5f) + d->volume_center[i];
        const float px = l[0] * M[0] + l[1] * M[4] + l[2] * M[8] + M[12];
        const float py = l[0] * M[1] + l[1] * M[5] + l[2] * M[9] + M[13];
        const float pz = l[0] * M[2] + l[1] * M[6] + l[2] * M[10] + M[14];
        p->pos[0] = vx * t + px;
        p->pos[1] = vy * t + py;
        p->pos[2] = (grav * t * t + vz * t) + pz;
        p->vel[0] = vx; p->vel[1] = vy; p->vel[2] = vz;
        p->remaining_life = life;
        p->life = life;
        p->age = t;
        p->gravity = grav;
        p->elasticity = xp_ftob(d->elasticity);
        p->size = xp_ftob(d->height_start);
        p->color = colour;
        p->flags = debris;
        if (debris) {
            p->uv[0] = debris_type;
            p->uv[1] = xp_ftob(e->uv->start_u);
            p->uv[2] = xp_ftob(e->uv->start_v);
            p->start[0] = (uint8_t)xp_rand_int(255, seed);
            p->start[1] = (uint8_t)xp_rand_int(255, seed);
            p->start[2] = (uint8_t)xp_rand_int(255, seed);
            p->rot[0] = e->rot[0];
            p->rot[1] = e->rot[1];
            p->rot[2] = e->rot[2];
        } else {
            p->uv[0] = xp_ftob(e->uv->start_u * 255.0f);
            p->uv[1] = xp_ftob(e->uv->start_v * 255.0f);
            p->uv[2] = xp_ftob(e->uv->end_u * 255.0f);
            p->uv[3] = xp_ftob(e->uv->end_v * 255.0f);
            p->start[0] = xp_ftob(size);
        }
        t += spacing;
        if (!(p->flags & XP_BOUNCED) && !no_collision) xp_collision_time(p, hit, user);
        ++spawned;
    }
    return spawned;
}

/* One sprite vertex as the original's vertex buffer holds it (stride 0x18): position, ARGB colour, u, v. */
typedef struct { float x, y, z; uint32_t color; float u, v; } XenonVertex;

/* XSpriteManager::AddParticle: each particle becomes a quad - a vertical ribbon of height size / 2048 from where the
 * particle is at its age to where it will be start[0] / 2048 seconds later (z(t) = z0 + vz t + g t^2), so a fast
 * spark draws as a streak along its path. Vertices: (now, u0 v1), (now + height, u2 v1), (later + height, u2 v3),
 * (later, u0 v3), uv bytes / 255; the caller indexes each quad as two triangles (0 1 2, 0 2 3 in the game's index
 * buffer). At most `cap` quads; returns the number written. */
static inline uint32_t xp_build_quads(const XenonParticle *list, uint32_t count, XenonVertex *out, uint32_t cap) {
    uint32_t q = 0;
    for (uint32_t i = 0; i < count && q < cap; ++i) {
        const XenonParticle *p = &list[i];
        const float a = p->age;
        const float x0 = a * p->vel[0] + p->pos[0], y0 = a * p->vel[1] + p->pos[1];
        const float z0 = a * p->vel[2] + p->pos[2] + a * a * p->gravity;
        const float h = (float)p->size * 0.00048828125f;
        const float b = (float)p->start[0] * 0.00048828125f + a;
        const float x1 = b * p->vel[0] + p->pos[0], y1 = b * p->vel[1] + p->pos[1];
        const float z1 = b * p->vel[2] + p->pos[2] + b * p->gravity * b;
        const float u0 = (float)p->uv[0] * 0.0039215689f, v1 = (float)p->uv[1] * 0.0039215689f;
        const float u2 = (float)p->uv[2] * 0.0039215689f, v3 = (float)p->uv[3] * 0.0039215689f;
        XenonVertex *v = &out[4u * q++];
        v[0] = (XenonVertex){x0, y0, z0, p->color, u0, v1};
        v[1] = (XenonVertex){x0, y0, z0 + h, p->color, u2, v1};
        v[2] = (XenonVertex){x1, y1, z1 + h, p->color, u2, v3};
        v[3] = (XenonVertex){x1, y1, z1, p->color, u0, v3};
    }
    return q;
}

/* The effect intensity for an effect without a piggyback (contrails), as the reference computes it:
 * lerp(0.1, 0.75, (|v| - 44) / 44) over the effect's velocity (all four components), the step clamped to 0..1;
 * such effects spawn without collisions. Effects with a piggyback (sparks) use 1.0 and collide. */
static inline float xp_contrail_intensity(const float v[4]) {
    float t = (sqrtf(v[0] * v[0] + v[1] * v[1] + v[2] * v[2] + v[3] * v[3]) - 44.0f) * (1.0f / 44.0f);
    if (!(t > 0.0f)) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return 0.1f + (0.75f - 0.1f) * t;
}

#endif
