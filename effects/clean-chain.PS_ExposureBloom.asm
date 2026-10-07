ps_3_0
dcl_texcoord0 v0
def c28 = 5.00000000e-01, 5.00000000e-01, 5.00000000e-01, 5.00000000e-01
def c29 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c30 = 0.00000000e+00, 0.00000000e+00, 2.50000000e-01, 1.44269502e+00
def c31 = 1.00000000e+00, -1.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c32 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c33 = 1.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
dcl_2d s2
dcl_2d s1
dcl_2d s3
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c8.xyzw
mov r2.xyzw, c7.xyzw
mov r3.xyzw, c6.xyzw
texld r4.xyzw, r0.xyxx, s0.xyzw
mov r0.z, r4.x
mov r5.x, r0.z
mov r0.z, r4.xxyx
mov r5.y, r0.z
mov r0.z, r4.xxzx
mov r5.z, r0.z
mov r0.z, r4.xxwx
mov r5.w, r0.z
mov r0.zw, c28.xxxy
texld r6.xyzw, r0.zwxx, s1.xyzw
texld r0.xyzw, r0.xyxx, s2.xyzw
mov r7.x, r0.x
mov r7.w, r0.xxxy
mov r7.y, r7.w
mov r0.x, r0.zxxx
mov r7.z, r0.x
mov r0.xy, c28.zwxx
texld r0.xyzw, r0.xyxx, s3.xyzw
mov r7.w, r6.x
mov r7.w, r7.w
mov r8.x, c29.x
mov r7.w, r7.w
mov r7.w, -r7.w
add r7.w, r8.x, r7.w
mov r8.x, c29.y
mov r8.y, c29.z
cmp r7.w, r7.w, r8.x, r8.y
mov r7.w, r7.w
mov r8.x, c29.w
mov r8.y, c30.x
mov r7.w, r7.w
mov r7.w, -r7.w
cmp r7.w, r7.w, r8.y, r8.x
mov r0.y, c30.y
max r0.x, r0.x, r0.y
mov r0.y, c30.z
log r0.x, r0.x
mul r0.x, r0.y, r0.x
exp r0.x, r0.x
mov r0.y, r3.x
mov r0.z, r3.xxyx
mov r0.w, r6.x
mul r0.z, r0.z, r0.w
add r0.y, r0.y, r0.z
mov r0.z, r3.xxzx
mul r0.x, r0.z, r0.x
add r0.x, r0.y, r0.x
mov r0.y, r3.xwxx
mul r0.x, r0.y, r0.x
mov r0.y, c30.w
mul r0.x, r0.y, r0.x
exp r0.x, r0.x
mov r0.y, r2.x
max r0.x, r0.x, r0.y
mov r0.y, r2.xyxx
min r0.x, r0.x, r0.y
mov r0.y, c31.x
mov r0.z, c31.y
add r0.x, r0.x, r0.z
mul r0.x, r7.w, r0.x
add r0.x, r0.y, r0.x
mov r2.xyzw, r5.xyzw
mov r0.yzw, r2.xxyz
mov r2.xyz, r0.x
mul r0.xyz, r0.yzwx, r2.xyzx
mov r2.xyz, r7.xyzx
mov r1.xyz, r1.zzzx
mul r1.xyz, r2.xyzx, r1.xyzx
add r0.xyz, r0.xyzx, r1.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.xyz, r1.xyzx
mov r1.xyz, c32.xyzx
max r0.xyz, r0.xyzx, r1.xyzx
mov r1.xyz, c33.xyzx
min r0.xyz, r0.xyzx, r1.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.x, r4.wxxx
mov r1.w, r0.x
mov r0.xyzw, r1.xyzw
mov oC0.xyzw, r0.xyzw
