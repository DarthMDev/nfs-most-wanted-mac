// clean-chain.hlsl
// Clean-room Module D.1 calibration on frozen Drop 2.2 A+B+C baseline
// Modules A, B and C are unchanged. D.1 changes only the clean finish calibration.
// One bounded black-box calibration round: dark-vignette shape + shadow-lift curve.
// NFSMW 2005 native Mac port
//
// Canonical source. Generated SM3 assembly / fx_2_0 are build artifacts.
// No console code, constants, instruction order, or private shader material used.
//
// Host conventions:
//   HostFrame.x = dt seconds, .y = reset flag, .z/.w = frame width/height.
//   DstTexelSize = (1/dstW, 1/dstH, dstW, dstH).
//   SrcTexelSizeN = (1/srcW, 1/srcH, srcW, srcH).
//   R32F samplers use point filtering.
//   Frame-writing passes preserve source alpha; host masks writes to RGB.
//
// History convention:
//   AdaptHistory.prev = last valid state or a negative invalid sentinel.
//   AdaptHistory.cur  = this frame's output.
//   On reset, a valid current measurement initializes immediately.
//   A black/invalid measurement does not advance adaptation.
//   If reset occurs on a black frame, -1 is kept until the first valid sample.

// fxgen technique manifests:
// technique clean_luma_log16:     vs=VS_Pass ps=PS_LumaLog16
// technique clean_luma_quartic16: vs=VS_Pass ps=PS_LumaQuartic16
// technique clean_reduce16:   vs=VS_Pass ps=PS_Reduce16
// technique clean_adapt:      vs=VS_Pass ps=PS_Adapt
// technique clean_exposure:       vs=VS_Pass ps=PS_Exposure
// technique clean_bright:         vs=VS_Pass ps=PS_Bright
// technique clean_blur_h:         vs=VS_Pass ps=PS_BlurH
// technique clean_blur_v:         vs=VS_Pass ps=PS_BlurV
// technique clean_exposure_bloom: vs=VS_Pass ps=PS_ExposureBloom
// technique clean_finish:         vs=VS_Pass ps=PS_Finish
// technique clean_dof_down:       vs=VS_Pass ps=PS_DofDown
// technique clean_dof_blur_h:     vs=VS_Pass ps=PS_DofBlurH
// technique clean_dof_blur_v:     vs=VS_Pass ps=PS_DofBlurV
// technique clean_motion_dof:     vs=VS_Pass ps=PS_MotionDof

float4 HostFrame       : register(c0);
float4 DstTexelSize    : register(c1);
float4 SrcTexelSize0   : register(c2);
float4 SrcTexelSize1   : register(c3);

// LumaParams:
//   x = epsilon used before log (default 0.0001)
//   yzw unused
float4 LumaParams      : register(c4);

// AdaptParams:
//   x = tau when measured scene luminance FALLS / scene gets darker (default 0.95 s)
//   y = tau when measured scene luminance RISES / scene gets brighter (default 0.42 s)
//   z = black / invalid measurement threshold (default 0.0005)
//   w = minimum positive adapted luminance (default 0.001)
float4 AdaptParams     : register(c5);

// ExposureModel (clean fit from six black-box target gains):
//   x = full-strength log-gain intercept
//   y = coefficient on adapted geometric/log-average luminance A
//   z = coefficient on instantaneous quartic mean Q4
//   w = setting strength 0..1 (default 0.50)
//
// At the default strength 0.50, the Drop 2.2 clean fit is:
//   gain = exp(0.5 * (1.27479958 -2.40261516*A -1.12322615*Q4))
// This keeps the same generic two-statistic model as Drop 2.1; only the
// coefficients are refit to the exact shader-side A/Q4 measurements.
// They are clean fitted parameters, not original/internal constants.
float4 ExposureModel   : register(c6);

// ExposureClamp:
//   x = minimum final gain (default 0.5)
//   y = maximum final gain (default 2.0)
//   z = safe minimum A for division (default 0.001)
//   w unused
float4 ExposureClamp   : register(c7);


// BloomParams (clean tunables; deliberately conservative first fit):
//   x = scene key used with adapted luminance (default 0.18)
//   y = bright-pass threshold after key/A scaling (default 0.80)
//   z = bloom composite strength (default 0.10)
//   w = minimum A used by the bright pass (default 0.02)
float4 BloomParams     : register(c8);

// BloomBlurParams:
//   x = reference quarter-resolution texel step in normalized frame-height units.
//       Default 1/270 gives one quarter-res pixel at 1080p and scales
//       proportionally with frame height at other resolutions.
//   yzw unused
float4 BloomBlurParams : register(c9);

// FinishParams (Module D dynamic inputs):
//   x = dark-vignette strength, 0..1 (default 0)
//   y = shadow-lift strength, 0..1 (default 0)
//   z = PC effect white-edge amount, 0..1 (default 0 when unused)
//   w unused
//
// The host supplies x/y from hybrid_vignette / hybrid_shadows and z from the
// PC game's effect-vignette request. The manifest carries neutral defaults.
float4 FinishParams     : register(c10);

// FinishShape (original clean D white-edge shape; retained unchanged in D.1):
//   x = squared radial distance where the PC white edge begins (0.50)
//   y = squared radial distance where the PC white edge reaches full strength (1.60)
//   z = retained original D value (0.18; unused by D.1)
//   w = retained original D value (0.45; unused by D.1)
// Keeping this separate means D.1 does not change the PC-requested white-edge path.
float4 FinishShape       : register(c11);

// FinishDarkShape (D.1 clean black-box fit; not console-derived):
//   x = squared radial distance where dark-vignette shaping begins (0.25)
//   y = squared radial distance where it reaches full strength (2.00)
//   z = maximum dark-vignette attenuation at 100% setting strength (0.42)
//   w unused
float4 FinishDarkShape   : register(c12);

// FinishShadowShape (D.1 clean black-box fit; not console-derived):
//   x = k in f(x)=x+k*x*(1-x)^2/(x+b) at 100% setting strength (0.123)
//   y = b denominator offset (0.009)
//   zw unused
// These values are fitted only to the reported clean-room black-box centre-tone
// curves. They are not recovered or copied game/console constants.
float4 FinishShadowShape : register(c13);

sampler2D SourceSampler  : register(s0);
sampler2D HistorySampler : register(s1);
sampler2D BloomSampler   : register(s2);
sampler2D QuarticSampler : register(s3);

struct VS_IN
{
    float4 position : POSITION0;
    float2 uv       : TEXCOORD0;
};

struct VS_OUT
{
    float4 position : POSITION0;
    float2 uv       : TEXCOORD0;
};

VS_OUT VS_Pass(VS_IN input)
{
    VS_OUT output;
    output.position = input.position;
    output.uv = input.uv;
    return output;
}

float GameFrameLuma(float3 rgb)
{
    // Rec.709 luma weights applied directly to the game's normalized frame colour.
    // No sRGB-to-linear conversion: this follows the clean-room host/spec contract.
    return dot(rgb, float3(0.2126, 0.7152, 0.0722));
}

float LogLumaAt(float2 uv)
{
    float3 rgb = tex2D(SourceSampler, uv).rgb;
    float y = max(GameFrameLuma(rgb), LumaParams.x);
    return log(y);
}

float QuarticLumaAt(float2 uv)
{
    float3 rgb = tex2D(SourceSampler, uv).rgb;
    float y = max(GameFrameLuma(rgb), 0.0);
    float y2 = y * y;
    return y2 * y2;
}

float ScalarAt(float2 uv)
{
    return tex2D(SourceSampler, uv).r;
}

float2 GridOffset(float x, float y)
{
    // Four stratified positions per destination-pixel footprint:
    // {-3/8, -1/8, +1/8, +3/8} in each dimension.
    return float2(x, y) * DstTexelSize.xy;
}

float4 PS_LumaLog16(VS_OUT input) : COLOR0
{
    // Area-sample the frame footprint represented by this destination pixel.
    // For the first frame -> frame/4 pass these points coincide with the
    // centres of the corresponding 4x4 source pixels.
    float sum = 0.0;

    sum += LogLumaAt(input.uv + GridOffset(-0.375, -0.375));
    sum += LogLumaAt(input.uv + GridOffset(-0.125, -0.375));
    sum += LogLumaAt(input.uv + GridOffset( 0.125, -0.375));
    sum += LogLumaAt(input.uv + GridOffset( 0.375, -0.375));

    sum += LogLumaAt(input.uv + GridOffset(-0.375, -0.125));
    sum += LogLumaAt(input.uv + GridOffset(-0.125, -0.125));
    sum += LogLumaAt(input.uv + GridOffset( 0.125, -0.125));
    sum += LogLumaAt(input.uv + GridOffset( 0.375, -0.125));

    sum += LogLumaAt(input.uv + GridOffset(-0.375,  0.125));
    sum += LogLumaAt(input.uv + GridOffset(-0.125,  0.125));
    sum += LogLumaAt(input.uv + GridOffset( 0.125,  0.125));
    sum += LogLumaAt(input.uv + GridOffset( 0.375,  0.125));

    sum += LogLumaAt(input.uv + GridOffset(-0.375,  0.375));
    sum += LogLumaAt(input.uv + GridOffset(-0.125,  0.375));
    sum += LogLumaAt(input.uv + GridOffset( 0.125,  0.375));
    sum += LogLumaAt(input.uv + GridOffset( 0.375,  0.375));

    float meanLog = sum * (1.0 / 16.0);
    return float4(meanLog, 0.0, 0.0, 1.0);
}

float4 PS_LumaQuartic16(VS_OUT input) : COLOR0
{
    // Pixelwise fourth moment, area-averaged over the same 4x4 footprint used
    // by the log-luminance path. The final fourth root is taken only in C.
    float sum = 0.0;

    sum += QuarticLumaAt(input.uv + GridOffset(-0.375, -0.375));
    sum += QuarticLumaAt(input.uv + GridOffset(-0.125, -0.375));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.125, -0.375));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.375, -0.375));

    sum += QuarticLumaAt(input.uv + GridOffset(-0.375, -0.125));
    sum += QuarticLumaAt(input.uv + GridOffset(-0.125, -0.125));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.125, -0.125));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.375, -0.125));

    sum += QuarticLumaAt(input.uv + GridOffset(-0.375,  0.125));
    sum += QuarticLumaAt(input.uv + GridOffset(-0.125,  0.125));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.125,  0.125));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.375,  0.125));

    sum += QuarticLumaAt(input.uv + GridOffset(-0.375,  0.375));
    sum += QuarticLumaAt(input.uv + GridOffset(-0.125,  0.375));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.125,  0.375));
    sum += QuarticLumaAt(input.uv + GridOffset( 0.375,  0.375));

    return float4(sum * (1.0 / 16.0), 0.0, 0.0, 1.0);
}

float4 PS_Reduce16(VS_OUT input) : COLOR0
{
    // Generic 4x4 stratified area reduction for an R32F source.
    // Because offsets are relative to the destination footprint, this also
    // handles the deliberately non-power-of-two frame/256 -> 4x4 step.
    float sum = 0.0;

    sum += ScalarAt(input.uv + GridOffset(-0.375, -0.375));
    sum += ScalarAt(input.uv + GridOffset(-0.125, -0.375));
    sum += ScalarAt(input.uv + GridOffset( 0.125, -0.375));
    sum += ScalarAt(input.uv + GridOffset( 0.375, -0.375));

    sum += ScalarAt(input.uv + GridOffset(-0.375, -0.125));
    sum += ScalarAt(input.uv + GridOffset(-0.125, -0.125));
    sum += ScalarAt(input.uv + GridOffset( 0.125, -0.125));
    sum += ScalarAt(input.uv + GridOffset( 0.375, -0.125));

    sum += ScalarAt(input.uv + GridOffset(-0.375,  0.125));
    sum += ScalarAt(input.uv + GridOffset(-0.125,  0.125));
    sum += ScalarAt(input.uv + GridOffset( 0.125,  0.125));
    sum += ScalarAt(input.uv + GridOffset( 0.375,  0.125));

    sum += ScalarAt(input.uv + GridOffset(-0.375,  0.375));
    sum += ScalarAt(input.uv + GridOffset(-0.125,  0.375));
    sum += ScalarAt(input.uv + GridOffset( 0.125,  0.375));
    sum += ScalarAt(input.uv + GridOffset( 0.375,  0.375));

    return float4(sum * (1.0 / 16.0), 0.0, 0.0, 1.0);
}

float4 PS_Adapt(VS_OUT input) : COLOR0
{
    // s0: 1x1 log-average luminance
    // s1: previous 1x1 adaptation history
    float logMean = tex2D(SourceSampler, input.uv).r;
    float measured = exp(logMean);
    float previous = tex2D(HistorySampler, input.uv).r;

    float measurementValid = (measured > AdaptParams.z) ? 1.0 : 0.0;
    float previousValid = (previous > 0.0) ? 1.0 : 0.0;
    float resetNow = (HostFrame.y > 0.5) ? 1.0 : 0.0;

    // Choose the time constant by the direction of measured scene luminance.
    // A decrease in measured luminance means the scene got darker.
    float sceneGotBrighter = (measured >= previous) ? 1.0 : 0.0;
    float tau = AdaptParams.x + sceneGotBrighter * (AdaptParams.y - AdaptParams.x);
    tau = max(tau, 0.0001);

    float dt = max(HostFrame.x, 0.0);
    float blend = 1.0 - exp(-dt / tau);
    float evolved = previous + (measured - previous) * blend;

    // A valid measurement initializes immediately after reset or invalid history.
    float initRequired = max(resetNow, 1.0 - previousValid);
    float validResult = evolved + initRequired * (measured - evolved);
    validResult = max(validResult, AdaptParams.w);

    // Invalid/black measurement: never adapt toward black.
    // After reset (or with no valid history), keep a negative sentinel so the
    // first subsequent valid measurement initializes immediately.
    float safePrevious = (previousValid > 0.5) ? previous : -1.0;
    float invalidResult = (resetNow > 0.5) ? -1.0 : safePrevious;

    float result = (measurementValid > 0.5) ? validResult : invalidResult;
    return float4(result, 0.0, 0.0, 1.0);
}

float CleanExposureGain(float adapted, float quarticMoment)
{
    float validA = (adapted > 0.0) ? 1.0 : 0.0;

    // Q4 = fourth root of the pixelwise fourth moment.
    float q4 = pow(max(quarticMoment, 0.0), 0.25);

    // Clean two-statistic predictor. The quartic mean acts as a smooth,
    // GPU-friendly upper-tail measure; unlike a percentile it needs no
    // histogram, sort, or CPU readback.
    float logFullGain =
        ExposureModel.x +
        ExposureModel.y * adapted +
        ExposureModel.z * q4;

    float gain = exp(ExposureModel.w * logFullGain);
    gain = min(max(gain, ExposureClamp.x), ExposureClamp.y);

    // Neutral until adaptation history is valid.
    return 1.0 + validA * (gain - 1.0);
}

float4 PS_Exposure(VS_OUT input) : COLOR0
{
    // s0: untouched chain-entry frame
    // s1: current adapted luminance
    // s3: current-frame quartic luminance moment
    float4 src = tex2D(SourceSampler, input.uv);
    float adapted = tex2D(HistorySampler, float2(0.5, 0.5)).r;
    float quarticMoment = tex2D(QuarticSampler, float2(0.5, 0.5)).r;

    float gain = CleanExposureGain(adapted, quarticMoment);
    return float4(src.rgb * gain, src.a);
}

float4 QuarterFrameAverage(float2 uv)
{
    // The bright target is frame/4. Four bilinear taps at quadrant centres
    // form a compact downsample of the corresponding source footprint.
    float2 q = DstTexelSize.xy * 0.25;

    float4 c0 = tex2D(SourceSampler, uv + float2(-q.x, -q.y));
    float4 c1 = tex2D(SourceSampler, uv + float2( q.x, -q.y));
    float4 c2 = tex2D(SourceSampler, uv + float2(-q.x,  q.y));
    float4 c3 = tex2D(SourceSampler, uv + float2( q.x,  q.y));

    return (c0 + c1 + c2 + c3) * 0.25;
}

float4 PS_Bright(VS_OUT input) : COLOR0
{
    // s0: untouched chain-entry frame
    // s1: current adapted luminance
    float4 src = QuarterFrameAverage(input.uv);
    float adapted = tex2D(HistorySampler, float2(0.5, 0.5)).r;

    float validA = (adapted > 0.0) ? 1.0 : 0.0;
    float safeA = max(adapted, BloomParams.w);

    // Public-technique bright pass: scene-key scaling by adapted luminance,
    // then threshold and clamp. All constants here are clean tunables.
    float scale = BloomParams.x / safeA;
    float3 scaled = src.rgb * scale;
    float3 bright = max(scaled - BloomParams.y, 0.0);

    bright *= validA;
    return float4(bright, 1.0);
}

float2 BloomStepX()
{
    // Keep blur radius proportional to frame height instead of target pixels.
    float pixelScale = SrcTexelSize0.w * BloomBlurParams.x;
    return float2(SrcTexelSize0.x * pixelScale, 0.0);
}

float2 BloomStepY()
{
    float pixelScale = SrcTexelSize0.w * BloomBlurParams.x;
    return float2(0.0, SrcTexelSize0.y * pixelScale);
}

float4 BlurFiveTap(float2 uv, float2 stepUV)
{
    // Bilinear-optimised Gaussian approximation. Radius is controlled separately.
    const float w0 = 0.2270270270;
    const float w1 = 0.3162162162;
    const float w2 = 0.0702702703;
    const float o1 = 1.3846153846;
    const float o2 = 3.2307692308;

    float4 result = tex2D(SourceSampler, uv) * w0;
    result += tex2D(SourceSampler, uv + stepUV * o1) * w1;
    result += tex2D(SourceSampler, uv - stepUV * o1) * w1;
    result += tex2D(SourceSampler, uv + stepUV * o2) * w2;
    result += tex2D(SourceSampler, uv - stepUV * o2) * w2;
    return result;
}

float4 PS_BlurH(VS_OUT input) : COLOR0
{
    return BlurFiveTap(input.uv, BloomStepX());
}

float4 PS_BlurV(VS_OUT input) : COLOR0
{
    return BlurFiveTap(input.uv, BloomStepY());
}

float4 PS_ExposureBloom(VS_OUT input) : COLOR0
{
    // s0: untouched chain-entry frame
    // s1: current adapted luminance
    // s2: vertically blurred quarter-resolution bloom
    // s3: current-frame quartic luminance moment
    float4 src = tex2D(SourceSampler, input.uv);
    float adapted = tex2D(HistorySampler, float2(0.5, 0.5)).r;
    float3 bloom = tex2D(BloomSampler, input.uv).rgb;
    float quarticMoment = tex2D(QuarticSampler, float2(0.5, 0.5)).r;

    float gain = CleanExposureGain(adapted, quarticMoment);
    float3 rgb = src.rgb * gain + bloom * BloomParams.z;

    return float4(min(max(rgb, 0.0), 1.0), src.a);
}

// -----------------------------------------------------------------------------
// Module D: finish. This is intentionally a separate full-resolution pass so
// the frozen A+B+C composite remains unchanged. At neutral settings it is an
// identity transform (apart from the host's ordinary ARGB8 store/load).
// -----------------------------------------------------------------------------

float FinishDarkEdgeMask(float2 uv)
{
    // D.1 dark vignette: clean black-box-fitted radial falloff.
    // Squared radius avoids a square root; smooth cubic keeps zero slope at ends.
    float2 p = (uv - float2(0.5, 0.5)) * 2.0;
    float r2 = dot(p, p);
    float width = max(FinishDarkShape.y - FinishDarkShape.x, 0.0001);
    float t = saturate((r2 - FinishDarkShape.x) / width);
    return t * t * (3.0 - 2.0 * t);
}

float FinishWhiteEdgeMask(float2 uv)
{
    // Preserve Module D's original clean white-edge path exactly in D.1.
    float2 p = (uv - float2(0.5, 0.5)) * 2.0;
    float r2 = dot(p, p);
    float width = max(FinishShape.y - FinishShape.x, 0.0001);
    float t = saturate((r2 - FinishShape.x) / width);
    return t * t * (3.0 - 2.0 * t);
}

float3 FinishShadowLift(float3 rgb, float strengthSetting)
{
    // D.1 clean black-box fit:
    //   f(x) = x + k*x*(1-x)^2/(x+b)
    // with k scaled by the setting. The fitted full-strength values are
    // k=0.123 and b=0.009. The curve preserves f(0)=0 and f(1)=1 and is
    // monotonic on [0,1] for this fitted parameter range.
    float k = saturate(strengthSetting) * max(FinishShadowShape.x, 0.0);
    float b = max(FinishShadowShape.y, 0.000001);
    float3 x = saturate(rgb);
    float3 oneMinus = 1.0 - x;
    float3 lifted = x + k * x * oneMinus * oneMinus / (x + b);
    return saturate(lifted);
}

float4 PS_Finish(VS_OUT input) : COLOR0
{
    // s0: frozen A+B+C full-resolution result.
    float4 src = tex2D(SourceSampler, input.uv);
    float3 rgb = FinishShadowLift(src.rgb, FinishParams.y);

    float darkEdge = FinishDarkEdgeMask(input.uv);

    // Dark vignette: D.1 calibrated clean-side shape.
    float darkAmount = saturate(FinishParams.x) * saturate(FinishDarkShape.z) * darkEdge;
    rgb *= 1.0 - darkAmount;

    // PC-requested effect vignette: unchanged from Module D. It retains the
    // original clean D radial mask and still blends the edge toward white.
    float whiteEdge = FinishWhiteEdgeMask(input.uv);
    float whiteAmount = saturate(FinishParams.z) * whiteEdge;
    rgb += (1.0 - rgb) * whiteAmount;

    return float4(saturate(rgb), src.a);
}

// -----------------------------------------------------------------------------
// Module E.3: camera motion blur and cinematic depth of field.
// Appended to the frozen D.1 drop. Every line above is unchanged; the only
// other addition is the four technique-manifest comment lines in the header.
// E.2 was the calibration round after E.1; E.3 is the in-car follow-up after
// the E.2 eye check ([O]).
//
// Public basis only:
//   - GPU Gems 3 ch. 27 (post-process motion blur): per-pixel screen velocity
//     reconstructed from scene depth and camera motion, then a gather of
//     samples along that velocity in one full-screen pass.
//   - GPU Gems ch. 23 (depth-of-field survey): blend between the sharp frame
//     and a blurred reduced-size copy by a per-pixel blur amount from depth.
//   - D3D9 projection transform documentation (left-handed perspective; view
//     z from device depth: z = n*f / (f - d*(f - n)), and its inverse
//     d = f/(f-n) * (1 - n/z)).
//   - Separable Gaussian blur (reuses the frozen BlurFiveTap helper).
//   - Interleaved gradient noise (Jimenez, SIGGRAPH 2014) to dither sample
//     positions, so long blurs show fine noise instead of banding.
// No console code, constants, instruction order or private shader material.
//
// E.2 changes against E.1 (kept): about half strength at 100%, clear centre
// with blur toward the edges (EMotionEdge), full coverage only at 3 px, 6 taps.
//
// E.3 changes against E.2 (in-car views only, cameraFlags.y = 0; the chase
// views use exactly the E.2 values and arithmetic):
//   - [O] clear area about 50% larger: clear radius 0.4 -> 0.6.
//   - [O] much more gradual clear-to-blurred change: the ramp runs from r = 0.6
//     to r = 1.8 (E.2: 0.4 to 1.1) and uses an ease-in profile
//     smoothstep(t) * t, so the blur fades in from nothing with no visible
//     ring edge.
//   - [O] hood fully sharp: near-geometry suppression in the in-car views
//     reaches to 3.5 m (full) / 5.5 m (none) instead of 1.2 / 2.0 m. Hood
//     pixels get zero coverage (they keep D.1's picture exactly) and are
//     down-weighted as blur samples, so the road does not smear onto them.
//   - [O] subtle: an in-car strength multiplier (0.85).
//   The in-car values are per-pass constants on the composite pass
//   (EMotionEdgeCar, ERigCar), because all 16 global constants are in use.
//   No hood/bumper signal is needed: the depth test keeps the hood sharp when
//   there is one and does nothing when there is not (bumper view).
//
// Chain placement (unchanged from E.1):
//   E reads FinishInput (the frozen A+B+C result, the same input D.1 reads)
//   and is the LAST pass, alpha-blended over D.1's frame (blend=alpha,
//   write=rgb). Its RGB is D.1's finish applied to E's blurred colour. Its
//   ALPHA is the E coverage, not the source alpha: this is the technique's
//   explicit alpha behaviour; the host's RGB write mask keeps the frame alpha.
//   When E is inactive the coverage is exactly 0.0, so the frame stays D.1's
//   output bit-for-bit (dst*1 + src*0). E never writes an A-D target.
//
// E keeps no temporal state (no history target), so Reset needs nothing.
// HostFrame.x (dt) is deliberately unused: the blur uses a fixed exposure
// time so its length does not change with frame rate.
// No flow control, no tex2Dlod, no lerp: plain tex2D, cmp-style selects.
// -----------------------------------------------------------------------------

// Module E host inputs (contract section 9; set by the runner every frame).
//   DepthInfo.x        = 1 when the depth copy (s1 in the E composite) is valid
//   nearFar.xy         = near, far plane (view-space units = metres)
//   cameraVelocityVS   = camera velocity in view space (m/s), w = speed
//   cameraFlags.x      = scripted cut-scene camera, .y = chase view
//   CameraFocus.xy     = PC focal distance, PC depth-of-field value
//   MotionSettings.xy  = motion-blur strength, DoF strength (0..1)
float4 DepthInfo        : register(c14);
float4 nearFar          : register(c15);
float4 cameraVelocityVS : register(c16);
float4 cameraFlags      : register(c17);
float4 CameraFocus      : register(c18);
float4 MotionSettings   : register(c19);

// EMotionParams (clean tunables):
//   x = camera speed where motion blur starts, m/s (8.0)
//   y = camera speed where blur strength saturates, m/s (60.0)
//   z = exposure time at 100% setting strength, seconds (E.2: 0.025)
//   w = maximum blur length in frame heights, independent of strength (E.2: 0.02)
float4 EMotionParams    : register(c20);

// EMotionShape (clean tunables):
//   x = blur length in pixels at which E coverage becomes full (E.2: 3.0)
//   y = vertical projection scale P22 = 1/tan(fovY/2) (1.7320508 = 60 deg);
//       P11 is derived as P22 * H / W
//   z = assumed view depth in metres when depth is unavailable (16.0)
//   w = motion-blur multiplier during scripted cut-scene cameras (0.0 = off)
float4 EMotionShape     : register(c21);

// ERigDepth (suppression of geometry that moves with the camera):
//   x,y = chase views: full suppression nearer than x m, none beyond y m (1.2, 2.0)
//         (E.3: the in-car views use ERigCar.xy instead)
//   z,w = chase view inside ERigRegion: full nearer than z, none beyond w (7.0, 10.0)
float4 ERigDepth        : register(c22);

// ERigRegion (chase-view player-car screen region, ellipse in uv):
//   xy = centre (0.50, 0.65), zw = radii (0.32, 0.38)
float4 ERigRegion       : register(c23);

// EDofParams (clean tunables):
//   x = in-focus half-width as a multiple of the PC DoF value (0.5)
//   y = blur transition length as a multiple of the PC DoF value (1.0)
//   z = unit scale applied to CameraFocus.xy to obtain metres (1.0)
//   w = blurred-copy step per blur pass, in frame heights (0.0055555556)
float4 EDofParams       : register(c24);

// EMotionEdge (E.2 [O] clear-centre weight; chase views since E.3):
//   Distance r is measured in half-frame-heights from the view point
//   (0.5, z) in uv, aspect-corrected (so the mid-left/right edges at 16:9 are
//   r = 1.78 and the mid-top/bottom edges are about r = 1).
//   x = r^2 inside which motion blur is zero (0.16, r = 0.4)
//   y = r^2 at which motion blur reaches full weight (1.21, r = 1.1)
//   z = view-point uv y (0.48, slightly above centre, toward the road ahead)
//   w = residual weight kept inside the clear zone, 0..1 (0.0; all views)
float4 EMotionEdge      : register(c25);

// EMotionEdgeCar (E.3 [O]; per-pass constant on clean_motion_dof; in-car
// views, cameraFlags.y = 0; same distance measure as EMotionEdge):
//   x = r^2 inside which motion blur is zero (0.36, r = 0.6: 50% larger)
//   y = r^2 at which motion blur reaches full weight (3.24, r = 1.8)
//   z = view-point uv y (0.48)
//   w = ease-in amount 0..1 (1.0): profile = smoothstep(t) * (1 - w + w*t)
float4 EMotionEdgeCar   : register(c26);

// ERigCar (E.3 [O]; per-pass constant on clean_motion_dof; in-car views):
//   x,y = near suppression: full nearer than x m, none beyond y m (3.5, 5.5);
//         replaces ERigDepth.xy in the in-car views so the hood stays sharp
//   z = in-car motion-blur strength multiplier, 0..1 (0.85)
//   w unused
float4 ERigCar          : register(c27);

float ESmoothStep01(float t)
{
    return t * t * (3.0 - 2.0 * t);
}

float ELinearStep(float x, float a, float b)
{
    return saturate((x - a) / max(b - a, 0.0001));
}

// Uniform-derived per-pixel constants for depth reconstruction:
//   x = near*far, y = far, z = far - near, w = fallback depth.
float4 EDepthConstants()
{
    float n = max(nearFar.x, 0.001);
    float f = max(nearFar.y, n + 0.001);
    return float4(n * f, f, f - n, max(EMotionShape.z, n));
}

// Rig-mask thresholds converted once per pixel from metres to device depth:
//   x = near start, y = 1/near width, z = chase start, w = 1/chase width
// (all in device-depth units, so each tap needs no divide).
// rz = the metre thresholds in effect for this view (ERigDepth layout).
float4 ERigDeviceConstants(float4 dk, float4 rz)
{
    float f = dk.y;
    float n = dk.y - dk.z;
    float4 zt = max(rz, n);
    float4 dt = (f / dk.z) * (1.0 - n / zt);
    return float4(dt.x, 1.0 / max(dt.y - dt.x, 0.000001),
                  dt.z, 1.0 / max(dt.w - dt.z, 0.000001));
}

float ERigMaskAt(float2 uv, float d, float2 flags, float4 rd, float2 regionInv)
{
    // 1 = this pixel belongs to geometry that moves with the camera (player
    // car in chase view, hood in the hood view): it must not be blurred and
    // must not bleed into blurred neighbours.
    // flags.x = depth valid, flags.y = chase view. d = device depth (1 when
    // depth is invalid, so the depth terms vanish and only the region acts).
    float nearAny = (1.0 - saturate((d - rd.x) * rd.y)) * flags.x;
    float nearChase = 1.0 - saturate((d - rd.z) * rd.w);
    nearChase = nearChase * flags.x + (1.0 - flags.x);
    float2 e = (uv - ERigRegion.xy) * regionInv;
    float region = saturate((1.0 - dot(e, e)) * 2.5);
    return max(nearAny, flags.y * region * nearChase);
}

float4 EMotionTap(float2 uv, float2 blurUV, float t, float2 flags, float4 rd, float2 regionInv)
{
    float2 tuv = uv + blurUV * t;
    float3 c = tex2D(SourceSampler, tuv).rgb;
    float raw = tex2D(HistorySampler, tuv).r;
    float d = (flags.x > 0.5) ? raw : 1.0;
    float w = 1.0 - ERigMaskAt(tuv, d, flags, rd, regionInv);
    return float4(c * w, w);
}

float3 EApplyFinish(float3 rgbIn, float2 uv)
{
    // Same finish as PS_Finish (D.1), applied to E's blurred colour so the
    // blended result carries D's shadow lift and vignettes.
    float3 rgb = FinishShadowLift(rgbIn, FinishParams.y);
    float darkEdge = FinishDarkEdgeMask(uv);
    float darkAmount = saturate(FinishParams.x) * saturate(FinishDarkShape.z) * darkEdge;
    rgb *= 1.0 - darkAmount;
    float whiteEdge = FinishWhiteEdgeMask(uv);
    float whiteAmount = saturate(FinishParams.z) * whiteEdge;
    rgb += (1.0 - rgb) * whiteAmount;
    return saturate(rgb);
}

float4 PS_DofDown(VS_OUT input) : COLOR0
{
    // s0: FinishInput (frozen A+B+C result), linear. Destination: frame/4.
    // Reuses the frozen 16-pixel box downsample.
    return float4(QuarterFrameAverage(input.uv).rgb, 1.0);
}

float4 PS_DofBlurH(VS_OUT input) : COLOR0
{
    // Step is a fixed fraction of frame height, so the look is resolution-independent.
    float pixelScale = SrcTexelSize0.w * EDofParams.w;
    return BlurFiveTap(input.uv, float2(SrcTexelSize0.x * pixelScale, 0.0));
}

float4 PS_DofBlurV(VS_OUT input) : COLOR0
{
    float pixelScale = SrcTexelSize0.w * EDofParams.w;
    return BlurFiveTap(input.uv, float2(0.0, SrcTexelSize0.y * pixelScale));
}

float4 PS_MotionDof(VS_OUT input) : COLOR0
{
    // s0: FinishInput (frozen A+B+C result), linear
    // s1: depth (R32F non-linear device depth), point
    // s2: blurred quarter-size copy of FinishInput (EDofPing), linear
    // Output: rgb = finish(E colour), a = E coverage (blend=alpha, write=rgb).
    float2 uv = input.uv;
    float depthValid = (DepthInfo.x > 0.5) ? 1.0 : 0.0;
    float chase = (cameraFlags.y > 0.5) ? 1.0 : 0.0;
    float inCar = 1.0 - chase;
    float2 flags = float2(depthValid, chase);
    float4 dk = EDepthConstants();

    // E.3: per-view parameters. With chase = 1 every blend term below is an
    // exact zero, so the chase views use the E.2 values unchanged.
    float4 rz = ERigDepth + inCar * (float4(ERigCar.xy, ERigDepth.zw) - ERigDepth);
    float4 rd = ERigDeviceConstants(dk, rz);
    float2 regionInv = 1.0 / max(ERigRegion.zw, 0.0001);

    // Centre pixel: device depth (select before arithmetic, so undefined depth
    // contents never reach the maths), view depth and rig mask.
    float3 centre = tex2D(SourceSampler, uv).rgb;
    float raw0 = tex2D(HistorySampler, uv).r;
    float d0 = (depthValid > 0.5) ? raw0 : 1.0;
    d0 = min(max(d0, 0.0), 1.0);
    float zDepth = dk.x / (dk.y - d0 * dk.z);   // denominator >= near > 0
    float z0 = (depthValid > 0.5) ? zDepth : dk.w;
    float rig0 = ERigMaskAt(uv, d0, flags, rd, regionInv);

    // ---- Motion blur: screen velocity of a static point under camera translation.
    // Strength rises smoothly above s0 and the effective speed saturates at s1.
    float speed = max(cameraVelocityVS.w, 0.0);
    float s0 = EMotionParams.x;
    float s1 = max(EMotionParams.y, s0 + 0.001);
    float ramp = ESmoothStep01(ELinearStep(speed, s0, s1));
    float speedGain = ramp * min(1.0, s1 / max(speed, 0.001));
    float3 v = cameraVelocityVS.xyz * speedGain;

    // d(ndc)/dt = (ndc * vz - P * vxy) / z  (camera moving +v, point static).
    float p22 = EMotionShape.y;
    float p11 = p22 * DstTexelSize.w * DstTexelSize.x;
    float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
    float2 ndcVel = float2(ndc.x * v.z - p11 * v.x, ndc.y * v.z - p22 * v.y) / z0;

    // [O] clear centre: edge weight from the aspect-corrected distance to the
    // driver's view point. Zero in the central disc, rising toward the edges.
    // Chase: E.2 disc and smoothstep ramp. In-car (E.3): larger disc, wider
    // ramp and an ease-in profile so no ring edge is visible.
    float3 edgeP = EMotionEdge.xyz + inCar * (EMotionEdgeCar.xyz - EMotionEdge.xyz);
    float ease = inCar * saturate(EMotionEdgeCar.w);
    float aspect = DstTexelSize.z * DstTexelSize.y;
    float2 q = float2((uv.x - 0.5) * aspect, uv.y - edgeP.z) * 2.0;
    float edgeLin = ELinearStep(dot(q, q), edgeP.x, edgeP.y);
    float edgeT = ESmoothStep01(edgeLin) * (1.0 - ease + ease * edgeLin);
    float edgeFloor = saturate(EMotionEdge.w);
    float edge = edgeFloor + (1.0 - edgeFloor) * edgeT;

    float cutMul = (cameraFlags.x > 0.5) ? EMotionShape.w : 1.0;
    float viewMul = 1.0 + inCar * (saturate(ERigCar.z) - 1.0);
    float strength = saturate(MotionSettings.x) * saturate(cutMul) * viewMul * edge;
    float2 blurUV = float2(0.5 * ndcVel.x, -0.5 * ndcVel.y) * (EMotionParams.z * strength);

    // Clamp the blur length in frame-height units (no sqrt of zero anywhere).
    float2 hVec = float2(blurUV.x * aspect, blurUV.y);
    float len2 = dot(hVec, hVec);
    float clampScale = min(1.0, max(EMotionParams.w, 0.0) * rsqrt(max(len2, 1.0e-12)));
    blurUV *= clampScale;
    len2 *= clampScale * clampScale;

    // Coverage: 0 for zero-length blur (exact), full once the blur reaches
    // EMotionShape.x pixels; removed on camera-rig pixels.
    float pxFull = max(EMotionShape.x, 0.001);
    float lenPx2 = len2 * DstTexelSize.w * DstTexelSize.w;
    float aMb = saturate(lenPx2 / (pxFull * pxFull)) * (1.0 - rig0);

    // Centred gather: centre tap (weight 1) + 6 dithered box-shutter taps,
    // each down-weighted when it lands on camera-rig geometry.
    float2 pix = uv * DstTexelSize.zw;
    float noise = frac(52.9829189 * frac(dot(pix, float2(0.06711056, 0.00583715))));
    float tb = noise * (1.0 / 6.0) - 0.5;

    float4 acc = float4(centre, 1.0);
    acc += EMotionTap(uv, blurUV, tb,             flags, rd, regionInv);
    acc += EMotionTap(uv, blurUV, tb + 0.1666667, flags, rd, regionInv);
    acc += EMotionTap(uv, blurUV, tb + 0.3333333, flags, rd, regionInv);
    acc += EMotionTap(uv, blurUV, tb + 0.5,       flags, rd, regionInv);
    acc += EMotionTap(uv, blurUV, tb + 0.6666667, flags, rd, regionInv);
    acc += EMotionTap(uv, blurUV, tb + 0.8333333, flags, rd, regionInv);
    float3 mb = acc.rgb / acc.a;   // acc.a >= 1

    // ---- Cinematic DoF (unchanged from E.1): active only when both PC focus
    // values are non-zero and depth is valid. Blur rises outside focus +/- halfWidth.
    float3 dofCopy = tex2D(BloomSampler, uv).rgb;
    float focusOn = (abs(CameraFocus.x) > 0.000001) ? 1.0 : 0.0;
    float rangeOn = (abs(CameraFocus.y) > 0.000001) ? 1.0 : 0.0;
    float unitScale = max(EDofParams.z, 0.0);
    float zf = abs(CameraFocus.x) * unitScale;
    float range = abs(CameraFocus.y) * unitScale;
    float halfWidth = max(EDofParams.x, 0.0) * range;
    float transition = max(EDofParams.y * range, 0.001);
    float outside = max(abs(z0 - zf) - halfWidth, 0.0);
    float blurAmount = ESmoothStep01(saturate(outside / transition));
    float aDof = focusOn * rangeOn * depthValid * saturate(MotionSettings.y) * blurAmount;

    // ---- Combine: lerp(lerp(sharp, mb, aMb), dof, aDof) written as one
    // coverage a over the sharp picture, with colour C = weighted mb/dof.
    float wMb = aMb * (1.0 - aDof);
    float wSum = wMb + aDof;
    float3 blended = (mb * wMb + dofCopy * aDof) / max(wSum, 0.00001);
    float3 rgb = EApplyFinish(blended, uv);
    return float4(rgb, saturate(wSum));
}
