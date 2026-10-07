ps_3_0
dcl_texcoord0 v0
def c28 = 2.50000000e-01, 2.50000000e-01, 5.00000000e-01, 5.00000000e-01
def c29 = 2.50000000e-01, 2.50000000e-01, 2.50000000e-01, 2.50000000e-01
def c30 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c31 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c32 = 1.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00
dcl_2d s1
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c8.xyzw
mov r2.xyzw, c1.xyzw
mov r0.zw, r2.xxxy
mov r2.xy, c28.xyxx
mul r0.zw, r0.xxzw, r2.xxxy
mov r2.x, r0.zxxx
mov r2.x, -r2.x
mov r2.y, r0.xwxx
mov r2.y, -r2.y
mov r2.z, r2.y
mov r2.xy, r2.xzxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r3.x, r2.x
mov r4.x, r2.yxxx
mov r3.y, r4.x
mov r4.x, r2.zxxx
mov r3.z, r4.x
mov r2.x, r2.wxxx
mov r3.w, r2.x
mov r2.x, r0.wxxx
mov r2.x, -r2.x
mov r2.y, r0.xzxx
mov r2.y, r2.y
mov r2.z, r2.x
mov r2.xy, r2.yzxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r4.x, r2.x
mov r5.x, r2.yxxx
mov r4.y, r5.x
mov r5.x, r2.zxxx
mov r4.z, r5.x
mov r2.x, r2.wxxx
mov r4.w, r2.x
mov r2.x, r0.zxxx
mov r2.x, -r2.x
mov r2.z, r0.xxwx
mov r2.y, r2.z
mov r2.xy, r2.xyxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r5.x, r2.x
mov r6.x, r2.yxxx
mov r5.y, r6.x
mov r6.x, r2.zxxx
mov r5.z, r6.x
mov r2.x, r2.wxxx
mov r5.w, r2.x
mov r2.x, r0.zxxx
mov r0.z, r0.xxwx
mov r2.y, r0.z
mov r0.zw, r2.xxxy
add r0.xy, r0.xyxx, r0.zwxx
texld r0.xyzw, r0.xyxx, s0.xyzw
mov r2.x, r0.x
mov r6.x, r0.yxxx
mov r2.y, r6.x
mov r6.x, r0.zxxx
mov r2.z, r6.x
mov r0.x, r0.wxxx
mov r2.w, r0.x
mov r0.xyzw, r3.xyzw
mov r3.xyzw, r4.xyzw
add r0.xyzw, r0.xyzw, r3.xyzw
mov r3.xyzw, r5.xyzw
add r0.xyzw, r0.xyzw, r3.xyzw
mov r2.xyzw, r2.xyzw
add r0.xyzw, r0.xyzw, r2.xyzw
mov r2.xyzw, c29.xyzw
mul r0.xyzw, r0.xyzw, r2.xyzw
mov r2.x, r0.x
mov r3.x, r0.yxxx
mov r2.y, r3.x
mov r3.x, r0.zxxx
mov r2.z, r3.x
mov r0.x, r0.wxxx
mov r2.w, r0.x
mov r0.xy, c28.zwxx
texld r0.xyzw, r0.xyxx, s1.xyzw
mov r3.x, r0.x
mov r3.y, c30.x
mov r3.x, -r3.x
add r3.x, r3.y, r3.x
mov r3.y, c30.y
mov r3.z, c30.z
cmp r3.x, r3.x, r3.y, r3.z
mov r3.y, c30.w
mov r3.z, c31.x
mov r3.x, -r3.x
cmp r3.x, r3.x, r3.z, r3.y
mov r0.y, r1.xwxx
max r0.x, r0.x, r0.y
mov r0.y, r1.x
rcp r0.x, r0.x
mul r0.x, r0.y, r0.x
mov r2.xyzw, r2.xyzw
mov r0.yzw, r2.xxyz
mov r2.xyz, r0.x
mul r0.xyz, r0.yzwx, r2.xyzx
mov r0.w, r0.x
mov r2.x, r0.w
mov r0.w, r0.xxxy
mov r2.y, r0.w
mov r0.x, r0.zxxx
mov r2.z, r0.x
mov r0.xyz, r2.xyzx
mov r0.w, r1.xxxy
mov r0.w, -r0.w
mov r1.xyz, r0.wwwx
add r0.xyz, r0.xyzx, r1.xyzx
mov r1.xyz, c31.yzwx
max r0.xyz, r0.xyzx, r1.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.xyz, r1.xyzx
mov r1.xyz, r3.x
mul r0.xyz, r0.xyzx, r1.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.x, c32.x
mov r1.w, r0.x
mov r0.xyzw, r1.xyzw
mov oC0.xyzw, r0.xyzw
