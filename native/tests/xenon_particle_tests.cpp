// The sparks and light trails particle list (xenon_particles.h). Physical checks of the ported XenonEffects
// (MIT) ageing, impact-time and bounce routines, and the routines' exact formulas on hand-worked cases.
#include "../../mods/core/nfsmw/xenon_particles.h"
#include <cmath>
#include <cstdio>
static int checks, failures;
#define CHECK(c) do { ++checks; if (!(c)) { ++failures; std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #c); } } while (0)
static bool close(float a, float b, float tol = 1e-4f) { return std::fabs(a - b) <= tol; }

// a flat ground at world height `ground`: the segment (UMath order -y, z, x) hits it when it crosses it
struct Ground { float height; int calls; };
static int ground_hit(void *user, const float a[4], const float b[4], float point[4], float normal[4]) {
    Ground *g = (Ground *)user;
    ++g->calls;
    if (!((a[1] - g->height) > 0.0f && (b[1] - g->height) <= 0.0f)) return 0;
    const float t = (a[1] - g->height) / (a[1] - b[1]);
    for (int i = 0; i < 3; ++i) point[i] = a[i] + t * (b[i] - a[i]);
    point[3] = 1.0f;
    normal[0] = 0.0f; normal[1] = 1.0f; normal[2] = 0.0f; normal[3] = 0.0f;   // world +z
    return 1;
}

int main() {
    // no collision callback: only the 0.15 lift; life unchanged; no impact
    {
        XenonParticle p{};
        p.pos[2] = 1.0f; p.life = 2.0f; p.vel[2] = 1.0f; p.gravity = -4.9f;
        xp_collision_time(&p, nullptr, nullptr);
        CHECK(close(p.pos[2], 1.15f) && p.life == 2.0f && !(p.flags & XP_IMPACT));
    }
    // a spark dropped 1.15 above flat ground (z = 0) with g = -4.9 (z = z0 + vz t + g t^2): the impact time solves
    // 1.15 + 0 t - 4.9 t^2 = 0 -> t = sqrt(1.15 / 4.9)
    {
        Ground g{0.0f, 0};
        XenonParticle p{};
        p.pos[2] = 1.0f; p.life = 3.0f; p.remaining_life = 3.0f; p.gravity = -4.9f; p.elasticity = 255;
        xp_collision_time(&p, ground_hit, &g);
        CHECK(g.calls == 1 && (p.flags & XP_IMPACT));
        CHECK(close(p.life, std::sqrt(1.15f / 4.9f)));
        CHECK(p.impact_normal[0] == 0.0f && p.impact_normal[1] == -0.0f && p.impact_normal[2] == 1.0f);   // world +z
        // the bounce: at the ground, the downward speed 2 g t reflected upward in full (elasticity 255)
        const float t = p.life, vz_hit = 2.0f * -4.9f * t;
        p.remaining_life = 3.0f - t;
        p.vel[0] = 0.0f;
        xp_bounce(&p, ground_hit, &g);
        CHECK(close(p.pos[2], 0.15f, 1e-3f));                      // impact point (z ~ 0), lifted 0.15 again
        CHECK(close(p.vel[2], -vz_hit, 1e-3f) && p.vel[2] > 0.0f);
        CHECK(p.age == 0.0f && (p.flags & XP_BOUNCED));
    }
    // elasticity halves the speed (128/255) and keeps the horizontal direction
    {
        XenonParticle p{};
        p.vel[0] = 3.0f; p.vel[2] = -4.0f; p.gravity = 0.0f; p.life = 0.0f; p.elasticity = 255;
        p.impact_normal[2] = 1.0f;
        xp_bounce(&p, nullptr, nullptr);
        CHECK(close(p.vel[0], 3.0f) && close(p.vel[2], 4.0f));
        p.vel[2] = -4.0f; p.elasticity = 0;
        xp_bounce(&p, nullptr, nullptr);
        CHECK(p.vel[0] == 0.0f && p.vel[2] == 0.0f);
    }
    // ageing: survivors age by dt and are packed; expired ones without an impact die
    {
        XenonParticle l[4]{};
        for (int i = 0; i < 4; ++i) { l[i].life = 1.0f; l[i].color = (uint32_t)i; }
        l[1].age = 0.95f;             // dies at dt 0.1
        l[3].age = 0.5f;
        const uint32_t n = xp_age(l, 4, 0.1f, nullptr, nullptr);
        CHECK(n == 3 && l[0].color == 0 && l[1].color == 2 && l[2].color == 3);
        CHECK(close(l[0].age, 0.1f) && close(l[2].age, 0.6f));
        // an expired particle with a pending impact and enough life left bounces and is kept (moved down too)
        XenonParticle m[3]{};
        m[0].life = 0.1f; m[0].age = 0.05f;                         // dies (no impact)
        m[1].life = 0.2f; m[1].age = 0.15f; m[1].flags = XP_IMPACT; m[1].remaining_life = 2.0f; m[1].elasticity = 200;
        m[1].impact_normal[2] = 1.0f; m[1].vel[2] = -1.0f; m[1].color = 7;
        m[2].life = 0.1f; m[2].age = 0.05f; m[2].flags = XP_IMPACT; m[2].remaining_life = 0.2f;   // 0.2 - 0.15 = 0.05 left: not more than dt, dies
        const uint32_t k = xp_age(m, 3, 0.1f, nullptr, nullptr);
        CHECK(k == 1 && m[0].color == 7 && (m[0].flags & XP_BOUNCED) && m[0].vel[2] > 0.0f);
        CHECK(close(m[0].life, 2.0f - 0.25f));                     // remaining life less the elapsed age + dt
    }
    // the game's generator (0045d9e0 / 0045d9a0): one step, and the float form's range
    {
        uint32_t seed = 12345u;
        const uint32_t v = xp_rand_step(&seed);
        const uint32_t x = 12345u ^ 0x1d872b41u, y = (x >> 5) ^ x;
        CHECK(v == 12345u && seed == ((y << 27) ^ y ^ x));
        uint32_t s2 = 0xdeadbeefu;
        for (int i = 0; i < 1000; ++i) { const float f = xp_rand_float(3.0f, &s2); CHECK(f >= 0.0f && f < 3.0f); }
        CHECK(xp_rand_int(0, &s2) == 0);
    }
    // spawning: n particles spread over dt, at the emitter's position, colour and life; the list limit holds
    {
        XenonEmitterData d{};
        d.colour1[0] = 1.0f; d.colour1[3] = 0.5f; d.life = 2.0f; d.life_variance = 0.25f; d.num_particles = 4.0f;
        d.length_start = 10.0f; d.height_start = 3.0f; d.elasticity = 128.0f; d.gravity_start = -9.8f;
        XenonEmitterUV uv{0.0f, 0.0f, 1.0f, 1.0f};
        XenonEmitterIn e{};
        e.d = &d; e.uv = &uv;
        e.local_world[0] = e.local_world[5] = e.local_world[10] = e.local_world[15] = 1.0f;
        e.local_world[12] = 5.0f;
        XenonParticle l[8]{};
        uint32_t n = 0, seed = 1;
        CHECK(xp_spawn(&e, 0.1f, 1.0f, 1, l, &n, 8, &seed, nullptr, nullptr) == 4 && n == 4);
        CHECK(l[0].pos[0] == 5.0f && l[0].age == 0.0f && std::fabs(l[3].age - 0.075f) < 1e-6f);
        CHECK(l[0].color == 0x7fff0000u && l[0].life == 1.5f && l[0].remaining_life == 1.5f && l[0].elasticity == 128);
        CHECK(l[0].start[0] == 10 && l[0].size == 3 && l[0].uv[2] == 255);
        CHECK(xp_spawn(&e, 0.1f, 1.0f, 1, l, &n, 8, &seed, nullptr, nullptr) == 4 && n == 8);
        CHECK(xp_spawn(&e, 0.1f, 1.0f, 1, l, &n, 8, &seed, nullptr, nullptr) == 0 && n == 8);   // full
        n = 0;
        CHECK(xp_spawn(&e, 0.1f, 0.5f, 1, l, &n, 8, &seed, nullptr, nullptr) == 4 && (l[0].color >> 24) == 21);   // alpha 42 x 0.5
        CHECK(xp_spawn(&e, 0.0f, 1.0f, 1, l, &n, 8, &seed, nullptr, nullptr) == 0);
        d.num_particles_variance = 0.002f; n = 0;                  // a fractional count stops at zero, not the limit
        CHECK(xp_spawn(&e, 0.1f, 1.0f, 1, l, &n, 8, &seed, nullptr, nullptr) == 4);
    }
    // the reference's contrail intensity: lerp(0.1, 0.75, (|v| - 44) / 44), step clamped to 0..1, all four components
    {
        const float a[4] = {0, 0, 0, 0}, b[4] = {55.0f, 0, 0, 0}, c[4] = {0, 66.0f, 0, 0}, e[4] = {0, 0, 100.0f, 0};
        const float d[4] = {44.0f, 0, 0, 30.0f};
        const auto lerp = [](float t) { return 0.1f + 0.65f * t; };
        CHECK(xp_contrail_intensity(a) == 0.1f);                                     // NOS at a standstill: the minimum
        CHECK(std::fabs(xp_contrail_intensity(b) - lerp(11.0f / 44.0f)) < 1e-6f);
        CHECK(std::fabs(xp_contrail_intensity(c) - lerp(0.5f)) < 1e-6f);
        CHECK(std::fabs(xp_contrail_intensity(e) - 0.75f) < 1e-6f);                 // capped
        CHECK(std::fabs(xp_contrail_intensity(d) - lerp((std::sqrt(44.0f * 44.0f + 900.0f) - 44.0f) / 44.0f)) < 1e-5f);   // w counts
    }
    std::printf("xenon_particle_tests: %d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
