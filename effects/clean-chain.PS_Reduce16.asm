ps_3_0
dcl_texcoord0 v0
def c28 = -3.75000000e-01, -3.75000000e-01, -1.25000000e-01, -3.75000000e-01
def c29 = 1.25000000e-01, -3.75000000e-01, 3.75000000e-01, -3.75000000e-01
def c30 = -3.75000000e-01, -1.25000000e-01, -1.25000000e-01, -1.25000000e-01
def c31 = 1.25000000e-01, -1.25000000e-01, 3.75000000e-01, -1.25000000e-01
def c32 = -3.75000000e-01, 1.25000000e-01, -1.25000000e-01, 1.25000000e-01
def c33 = 1.25000000e-01, 1.25000000e-01, 3.75000000e-01, 1.25000000e-01
def c34 = -3.75000000e-01, 3.75000000e-01, -1.25000000e-01, 3.75000000e-01
def c35 = 1.25000000e-01, 3.75000000e-01, 3.75000000e-01, 3.75000000e-01
def c36 = 6.25000000e-02, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c1.xyzw
mov r0.zw, c28.xxxy
mov r2.xy, r1.xyxx
mul r0.zw, r0.xxzw, r2.xxxy
add r0.zw, r0.xxxy, r0.xxzw
texld r2.xyzw, r0.zwxx, s0.xyzw
mov r0.z, r2.x
mov r2.xy, c28.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c29.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c29.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c30.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c30.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c31.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c31.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c32.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c32.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c33.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c33.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c34.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c34.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c35.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
add r0.z, r0.z, r0.w
mov r2.xy, c35.zwxx
mov r1.xy, r1.xyxx
mul r1.xy, r2.xyxx, r1.xyxx
add r0.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r0.xyxx, s0.xyzw
mov r0.x, r1.x
add r0.x, r0.z, r0.x
mov r0.y, c36.x
mul r0.x, r0.x, r0.y
mov r1.x, c36.y
mov r0.y, r1.x
mov r1.x, c36.z
mov r0.z, r1.x
mov r1.x, c36.w
mov r0.w, r1.x
mov r0.xyzw, r0.xyzw
mov oC0.xyzw, r0.xyzw
