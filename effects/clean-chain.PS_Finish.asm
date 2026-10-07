ps_3_0
dcl_texcoord0 v0
def c28 = 0.00000000e+00, 9.99999997e-07, -5.00000000e-01, -5.00000000e-01
def c29 = 1.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
def c30 = 2.00000000e+00, 2.00000000e+00, 9.99999975e-05, 3.00000000e+00
def c31 = 2.00000000e+00, 1.00000000e+00, -5.00000000e-01, -5.00000000e-01
def c32 = 2.00000000e+00, 2.00000000e+00, 0.00000000e+00, 9.99999975e-05
def c33 = 3.00000000e+00, 2.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c34 = 1.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c13.xyzw
mov r2.xyzw, c12.xyzw
mov r3.xyzw, c11.xyzw
mov r4.xyzw, c10.xyzw
texld r5.xyzw, r0.xyxx, s0.xyzw
mov r0.z, r5.x
mov r6.x, r0.z
mov r0.z, r5.xxyx
mov r6.y, r0.z
mov r0.z, r5.xxzx
mov r6.z, r0.z
mov r0.z, r5.xxwx
mov r6.w, r0.z
mov r6.xyzw, r6.xyzw
mov r0.z, r4.xxyx
mov_sat r0.z, r0.z
mov r0.w, r1.x
mov r7.x, c28.x
max r0.w, r0.w, r7.x
mul r0.z, r0.z, r0.w
mov r0.w, r1.xxxy
mov r1.x, c28.y
max r0.w, r0.w, r1.x
mov r1.xyz, r6.xyzx
mov_sat r1.xyz, r1.xyzx
mov r1.w, r1.x
mov r6.x, r1.w
mov r1.w, r1.xxxy
mov r6.y, r1.w
mov r1.x, r1.zxxx
mov r6.z, r1.x
mov r1.xyz, r6.xyzx
mov r1.xyz, -r1.xyzx
mov r7.xyz, c29.xyzx
add r1.xyz, r7.xyzx, r1.xyzx
mov r1.w, r1.x
mov r7.x, r1.w
mov r1.w, r1.xxxy
mov r7.y, r1.w
mov r1.x, r1.zxxx
mov r7.z, r1.x
mov r1.xyz, r6.xyzx
mov r8.xyz, r6.xyzx
mov r9.xyz, r0.zzzx
mul r8.xyz, r9.xyzx, r8.xyzx
mov r9.xyz, r7.xyzx
mul r8.xyz, r8.xyzx, r9.xyzx
mov r7.xyz, r7.xyzx
mul r7.xyz, r8.xyzx, r7.xyzx
mov r6.xyz, r6.xyzx
mov r8.xyz, r0.wwwx
add r6.xyz, r6.xyzx, r8.xyzx
rcp r6.x, r6.x
rcp r6.y, r6.y
rcp r6.z, r6.z
mul r6.xyz, r7.xyzx, r6.xyzx
add r1.xyz, r1.xyzx, r6.xyzx
mov r0.z, r1.x
mov r6.x, r0.z
mov r0.z, r1.xxyx
mov r6.y, r0.z
mov r0.z, r1.xxzx
mov r6.z, r0.z
mov r1.xyz, r6.xyzx
mov_sat r1.xyz, r1.xyzx
mov r0.z, r1.x
mov r6.x, r0.z
mov r0.z, r1.xxyx
mov r6.y, r0.z
mov r0.z, r1.xxzx
mov r6.z, r0.z
mov r0.zw, c28.xxzw
add r0.zw, r0.xxxy, r0.xxzw
mov r1.xy, c30.xyxx
mul r0.zw, r0.xxzw, r1.xxxy
mov r1.x, r0.zxxx
mov r0.z, r0.xxwx
mov r1.y, r0.z
mov r0.zw, r1.xxxy
mov r1.xy, r1.xyxx
mov r1.z, c29.w
dp2add r0.z, r0.zwxx, r1.xyxx, r1.z
mov r0.w, r2.xxxy
mov r1.x, r2.x
mov r1.x, -r1.x
add r0.w, r0.w, r1.x
mov r1.x, c30.z
max r0.w, r0.w, r1.x
mov r1.x, r2.x
mov r1.x, -r1.x
add r0.z, r0.z, r1.x
rcp r0.w, r0.w
mul r0.z, r0.z, r0.w
mov_sat r0.z, r0.z
mul r0.w, r0.z, r0.z
mov r1.x, c30.w
mov r1.y, c31.x
mul r0.z, r1.y, r0.z
mov r0.z, -r0.z
add r0.z, r1.x, r0.z
mul r0.z, r0.w, r0.z
mov r0.w, r4.x
mov_sat r0.w, r0.w
mov r1.x, r2.zxxx
mov_sat r1.x, r1.x
mul r0.w, r0.w, r1.x
mul r0.z, r0.w, r0.z
mov r0.w, c31.y
mov r0.z, -r0.z
add r0.z, r0.w, r0.z
mov r1.xyz, r6.xyzx
mov r2.xyz, r0.zzzx
mul r1.xyz, r1.xyzx, r2.xyzx
mov r0.zw, c31.xxzw
add r0.xy, r0.xyxx, r0.zwxx
mov r0.zw, c32.xxxy
mul r0.xy, r0.xyxx, r0.zwxx
mov r0.z, r0.x
mov r0.z, r0.z
mov r0.x, r0.yxxx
mov r0.w, r0.x
mov r0.xy, r0.zwxx
mov r0.zw, r0.xxzw
mov r1.w, c32.z
dp2add r0.x, r0.xyxx, r0.zwxx, r1.w
mov r0.y, r3.xyxx
mov r0.z, r3.x
mov r0.z, -r0.z
add r0.y, r0.y, r0.z
mov r0.z, c32.w
max r0.y, r0.y, r0.z
mov r0.z, r3.x
mov r0.z, -r0.z
add r0.x, r0.x, r0.z
rcp r0.y, r0.y
mul r0.x, r0.x, r0.y
mov_sat r0.x, r0.x
mul r0.y, r0.x, r0.x
mov r0.z, c33.x
mov r0.w, c33.y
mul r0.x, r0.w, r0.x
mov r0.x, -r0.x
add r0.x, r0.z, r0.x
mul r0.x, r0.y, r0.x
mov r0.y, r4.xzxx
mov_sat r0.y, r0.y
mul r0.x, r0.y, r0.x
mov r0.yzw, -r1.xxyz
mov r2.xyz, c34.xyzx
add r0.yzw, r2.xxyz, r0.xyzw
mov r2.xyz, r0.x
mul r0.xyz, r0.yzwx, r2.xyzx
add r0.xyz, r1.xyzx, r0.xyzx
mov_sat r0.xyz, r0.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.x, r5.wxxx
mov r1.w, r0.x
mov r0.xyzw, r1.xyzw
mov oC0.xyzw, r0.xyzw
