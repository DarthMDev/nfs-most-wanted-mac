// clean-pools.hlsl
// Clean-room Module F.1: point-light pools, extra additive pass for the PC
// WORLD (VS_Pools / PS_Pools) and WORLDREFLECT (VS_PoolsReflect /
// PS_PoolsReflect) effects. NFSMW 2005 native Mac port, 5 Oct 2026.
//
// Canonical source. Generated SM3 assembly is a build artifact.
// No console code, constants, instruction order, or private shader material used.
//
// Public basis only:
//   - inverse-square law for point lights;
//   - softened inverse square d0^2 / (d^2 + d0^2) (finite at d = 0, peak = intensity);
//   - smooth windowed cutoff saturate(1 - (d/r)^4)^2 (Karis, "Real Shading in
//     Unreal Engine 4", SIGGRAPH 2013 course notes), zero at the radius;
//   - Lambert diffuse with a small wrap term to soften the terminator;
//   - tangent-space normal mapping (standard TBN basis) for roads.
//
// Pass state (set by the host, contract section 10): additive ONE,ONE, RGB writes
// only, Z write off, ZFUNC LESSEQUAL, alpha test as the base pass, fog off.
// The output is albedo * sum(lights) * distance fade * strength, added to the
// frame the base pass already lit. Alpha = the diffuse texel's alpha, so
// alpha-tested holes stay holes.
//
// N = 8 lights per draw (CleanPools[16]). Lights beyond CleanPoolCount.x are
// masked out even if the host leaves stale data in their slots.
// No flow control: all 8 lights are evaluated with literal indices and masked.
// No tex2Dlod, no lerp, no dynamic constant indexing. Every normalisation is
// guarded so degenerate normals or tangents cannot produce NaN.

// ---- PC effect parameters (set by the game every draw; matched by name) ----
float4x4 WorldViewProj   : register(c0);   // object -> clip, D3DX row-vector convention (VS)
float4   TextureOffset   : register(c4);   // .xy added to TEXCOORD0 (VS)

// ---- PC effect parameter used in the pixel shader ----
float4   LocalEyePos     : register(c5);   // camera position in the draw's object space, metres

// ---- Host parameters for the extra pass (contract section 10) ----
// CleanPoolCount.x = number of lights for this draw (0..8).
float4   CleanPoolCount  : register(c6);

// CleanPoolParams (global tunables, same for every draw; values in lamp-classes.toml [params]):
//   x = distance-fade start, metres from the camera (40.0)
//   y = distance-fade end, metres; pools are zero beyond it (70.0)
//   z = global pool strength multiplier (1.0)
//   w = road normal-map weight in WORLDREFLECT, 0..1 (0.5; 0 = geometric normal only)
float4   CleanPoolParams : register(c7);

// CleanPools[2i]   = light position in the draw's object space (xyz), w = radius (m)
// CleanPools[2i+1] = colour x intensity x activation (rgb),
//                    w = falloff: softening distance d0 in metres (F.1 request; when the
//                    host leaves it 0, the shader uses d0 = 0.25 * radius)
float4   CleanPools[16]  : register(c8);

sampler2D diffuse_sampler : register(s0);
sampler2D normal_sampler  : register(s1);   // WORLDREFLECT only

// Design constants [T].
static const float kPoolWrap        = 0.15;   // Lambert wrap: softens the light/dark edge
static const float kDefaultD0Factor = 0.25;   // d0 = factor * radius when falloff is 0

struct VS_POOLS_IN
{
    float4 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
};

struct VS_POOLS_OUT
{
    float4 position : POSITION0;
    float2 uv       : TEXCOORD0;
    float3 objPos   : TEXCOORD1;
    float3 normal   : TEXCOORD2;
};

struct PS_POOLS_IN
{
    float2 uv       : TEXCOORD0;
    float3 objPos   : TEXCOORD1;
    float3 normal   : TEXCOORD2;
};

struct VS_POOLSR_IN
{
    float4 position : POSITION0;
    float3 normal   : NORMAL0;
    float2 uv       : TEXCOORD0;
    float4 tangent  : TANGENT0;
};

struct VS_POOLSR_OUT
{
    float4 position : POSITION0;
    float2 uv       : TEXCOORD0;
    float3 objPos   : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float4 tangent  : TEXCOORD3;
};

struct PS_POOLSR_IN
{
    float2 uv       : TEXCOORD0;
    float3 objPos   : TEXCOORD1;
    float3 normal   : TEXCOORD2;
    float4 tangent  : TEXCOORD3;
};

float3 PoolSafeNormalize(float3 v)
{
    // Never NaN for a finite input: a zero vector stays (near) zero.
    return v * rsqrt(max(dot(v, v), 1.0e-8));
}

float4 PoolClipPosition(float4 objectPosition)
{
    // Same object -> clip transform as the base pass (D3DX row-vector convention).
    return mul(float4(objectPosition.xyz, 1.0), WorldViewProj);
}

// One light. lp = position (xyz) + radius (w); lc = colour x intensity (rgb) + d0 (w).
float3 PoolLight(float4 lp, float4 lc, float3 p, float3 n, float index)
{
    float3 v = lp.xyz - p;
    float d2 = max(dot(v, v), 0.0001);
    float r = max(lp.w, 0.01);

    // Smooth windowed cutoff: 1 near the light, exactly 0 at and beyond the radius.
    float x = d2 / (r * r);
    float win = saturate(1.0 - x * x);
    win *= win;

    // Softened inverse square: peak = intensity at d = 0, half at d = d0.
    float d0 = (lc.w > 0.0) ? lc.w : kDefaultD0Factor * r;
    float d02 = d0 * d0;
    float soft = d02 / (d2 + d02);

    // Lambert with a small wrap.
    float ndl = dot(n, v) * rsqrt(d2);
    float diffuse = saturate((ndl + kPoolWrap) / (1.0 + kPoolWrap));

    float active = (CleanPoolCount.x > index + 0.5) ? 1.0 : 0.0;
    return max(lc.rgb, 0.0) * (win * soft * diffuse * active);
}

float3 PoolsShade(float3 p, float3 n, float3 albedo)
{
    float3 sum = PoolLight(CleanPools[0],  CleanPools[1],  p, n, 0.0);
    sum += PoolLight(CleanPools[2],  CleanPools[3],  p, n, 1.0);
    sum += PoolLight(CleanPools[4],  CleanPools[5],  p, n, 2.0);
    sum += PoolLight(CleanPools[6],  CleanPools[7],  p, n, 3.0);
    sum += PoolLight(CleanPools[8],  CleanPools[9],  p, n, 4.0);
    sum += PoolLight(CleanPools[10], CleanPools[11], p, n, 5.0);
    sum += PoolLight(CleanPools[12], CleanPools[13], p, n, 6.0);
    sum += PoolLight(CleanPools[14], CleanPools[15], p, n, 7.0);

    // Distance fade from the camera (object-space metres), smooth cubic.
    float3 e = LocalEyePos.xyz - p;
    float e2 = max(dot(e, e), 0.000001);
    float dist = e2 * rsqrt(e2);
    float fadeStart = CleanPoolParams.x;
    float fadeWidth = max(CleanPoolParams.y - CleanPoolParams.x, 0.01);
    float t = saturate((dist - fadeStart) / fadeWidth);
    float fade = 1.0 - t * t * (3.0 - 2.0 * t);

    return albedo * sum * (fade * max(CleanPoolParams.z, 0.0));
}

// ---------------------------------------------------------------- WORLD ----

VS_POOLS_OUT VS_Pools(VS_POOLS_IN input)
{
    VS_POOLS_OUT output;
    output.position = PoolClipPosition(input.position);
    output.uv = input.uv + TextureOffset.xy;
    output.objPos = input.position.xyz;
    output.normal = input.normal;
    return output;
}

float4 PS_Pools(PS_POOLS_IN input) : COLOR0
{
    float4 albedo = tex2D(diffuse_sampler, input.uv);
    float3 n = PoolSafeNormalize(input.normal);
    float3 rgb = PoolsShade(input.objPos, n, albedo.rgb);
    return float4(rgb, albedo.a);
}

// ---------------------------------------------------------- WORLDREFLECT ----

VS_POOLSR_OUT VS_PoolsReflect(VS_POOLSR_IN input)
{
    VS_POOLSR_OUT output;
    output.position = PoolClipPosition(input.position);
    output.uv = input.uv + TextureOffset.xy;
    output.objPos = input.position.xyz;
    output.normal = input.normal;
    output.tangent = input.tangent;
    return output;
}

float4 PS_PoolsReflect(PS_POOLSR_IN input) : COLOR0
{
    float4 albedo = tex2D(diffuse_sampler, input.uv);
    float3 ng = PoolSafeNormalize(input.normal);

    // Tangent-space road normal map (standard rgb*2-1 decoding), blended toward
    // the geometric normal by CleanPoolParams.w. Bitangent = cross(N, T) * sign.
    float3 nm = tex2D(normal_sampler, input.uv).xyz * 2.0 - 1.0;
    float3 tng = input.tangent.xyz;
    float3 btg = cross(ng, tng) * input.tangent.w;
    float3 nb = PoolSafeNormalize(tng * nm.x + btg * nm.y + ng * nm.z);
    float k = saturate(CleanPoolParams.w);
    float3 nBlend = PoolSafeNormalize(ng + k * (nb - ng));
    float3 n = (k > 0.0) ? nBlend : ng;

    float3 rgb = PoolsShade(input.objPos, n, albedo.rgb);
    return float4(rgb, albedo.a);
}
