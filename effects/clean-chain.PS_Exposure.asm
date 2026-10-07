ps_3_0
dcl_texcoord0 v0
def c28 = 5.00000000e-01, 5.00000000e-01, 5.00000000e-01, 5.00000000e-01
def c29 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c30 = 0.00000000e+00, 0.00000000e+00, 2.50000000e-01, 1.44269502e+00
def c31 = 1.00000000e+00, -1.00000000e+00, 0.00000000e+00, 0.00000000e+00
dcl_2d s1
dcl_2d s3
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c7.xyzw
mov r2.xyzw, c6.xyzw
texld r0.xyzw, r0.xyxx, s0.xyzw
mov r3.x, r0.x
mov r4.x, r0.yxxx
mov r3.y, r4.x
mov r4.x, r0.zxxx
mov r3.z, r4.x
mov r4.x, r0.wxxx
mov r3.w, r4.x
mov r4.xy, c28.xyxx
texld r4.xyzw, r4.xyxx, s1.xyzw
mov r5.xy, c28.zwxx
texld r5.xyzw, r5.xyxx, s3.xyzw
mov r6.x, r4.x
mov r6.y, c29.x
mov r6.x, -r6.x
add r6.x, r6.y, r6.x
mov r6.y, c29.y
mov r6.z, c29.z
cmp r6.x, r6.x, r6.y, r6.z
mov r6.y, c29.w
mov r6.z, c30.x
mov r6.x, -r6.x
cmp r6.x, r6.x, r6.z, r6.y
mov r5.y, c30.y
max r5.x, r5.x, r5.y
mov r5.y, c30.z
log r5.x, r5.x
mul r5.x, r5.y, r5.x
exp r5.x, r5.x
mov r5.y, r2.x
mov r5.z, r2.xxyx
mul r4.x, r5.z, r4.x
add r4.x, r5.y, r4.x
mov r4.y, r2.xzxx
mul r4.y, r4.y, r5.x
add r4.x, r4.x, r4.y
mov r2.x, r2.wxxx
mul r2.x, r2.x, r4.x
mov r2.y, c30.w
mul r2.x, r2.y, r2.x
exp r2.x, r2.x
mov r2.y, r1.x
max r2.x, r2.x, r2.y
mov r1.x, r1.yxxx
min r1.x, r2.x, r1.x
mov r1.y, c31.x
mov r1.z, c31.y
add r1.x, r1.x, r1.z
mul r1.x, r6.x, r1.x
add r1.x, r1.y, r1.x
mov r2.xyzw, r3.xyzw
mov r1.yzw, r2.xxyz
mov r2.xyz, r1.x
mul r1.xyz, r1.yzwx, r2.xyzx
mov r1.w, r1.x
mov r2.x, r1.w
mov r1.w, r1.xxxy
mov r2.y, r1.w
mov r1.x, r1.zxxx
mov r2.z, r1.x
mov r0.x, r0.wxxx
mov r2.w, r0.x
mov r0.xyzw, r2.xyzw
mov oC0.xyzw, r0.xyzw
