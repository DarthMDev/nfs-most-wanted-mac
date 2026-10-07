ps_3_0
dcl_texcoord0 v0
def c28 = 2.50000000e-01, 2.50000000e-01, 1.00000000e+00, 0.00000000e+00
def c29 = 2.50000000e-01, 2.50000000e-01, 2.50000000e-01, 2.50000000e-01
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c1.xyzw
mov r0.zw, r1.xxxy
mov r1.xy, c28.xyxx
mul r0.zw, r0.xxzw, r1.xxxy
mov r1.x, r0.zxxx
mov r1.x, -r1.x
mov r1.y, r0.xwxx
mov r1.y, -r1.y
mov r1.z, r1.y
mov r1.xy, r1.xzxx
add r1.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r1.xyxx, s0.xyzw
mov r2.x, r1.x
mov r3.x, r1.yxxx
mov r2.y, r3.x
mov r3.x, r1.zxxx
mov r2.z, r3.x
mov r1.x, r1.wxxx
mov r2.w, r1.x
mov r1.x, r0.wxxx
mov r1.x, -r1.x
mov r1.y, r0.xzxx
mov r1.y, r1.y
mov r1.z, r1.x
mov r1.xy, r1.yzxx
add r1.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r1.xyxx, s0.xyzw
mov r3.x, r1.x
mov r4.x, r1.yxxx
mov r3.y, r4.x
mov r4.x, r1.zxxx
mov r3.z, r4.x
mov r1.x, r1.wxxx
mov r3.w, r1.x
mov r1.x, r0.zxxx
mov r1.x, -r1.x
mov r1.z, r0.xxwx
mov r1.y, r1.z
mov r1.xy, r1.xyxx
add r1.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r1.xyxx, s0.xyzw
mov r4.x, r1.x
mov r5.x, r1.yxxx
mov r4.y, r5.x
mov r5.x, r1.zxxx
mov r4.z, r5.x
mov r1.x, r1.wxxx
mov r4.w, r1.x
mov r1.x, r0.zxxx
mov r0.z, r0.xxwx
mov r1.y, r0.z
mov r0.zw, r1.xxxy
add r0.xy, r0.xyxx, r0.zwxx
texld r0.xyzw, r0.xyxx, s0.xyzw
mov r1.x, r0.x
mov r5.x, r0.yxxx
mov r1.y, r5.x
mov r5.x, r0.zxxx
mov r1.z, r5.x
mov r0.x, r0.wxxx
mov r1.w, r0.x
mov r0.xyzw, r2.xyzw
mov r2.xyzw, r3.xyzw
add r0.xyzw, r0.xyzw, r2.xyzw
mov r2.xyzw, r4.xyzw
add r0.xyzw, r0.xyzw, r2.xyzw
mov r1.xyzw, r1.xyzw
add r0.xyzw, r0.xyzw, r1.xyzw
mov r1.xyzw, c29.xyzw
mul r0.xyzw, r0.xyzw, r1.xyzw
mov r1.x, r0.x
mov r2.x, r0.yxxx
mov r1.y, r2.x
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.x, c28.z
mov r1.w, r0.x
mov r0.xyzw, r1.xyzw
mov oC0.xyzw, r0.xyzw
