ps_3_0
dcl_texcoord0 v0
def c28 = -3.75000000e-01, -3.75000000e-01, 0.00000000e+00, 0.00000000e+00
def c29 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c30 = -1.25000000e-01, -3.75000000e-01, 1.25000000e-01, -3.75000000e-01
def c31 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c32 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c33 = 3.75000000e-01, -3.75000000e-01, -3.75000000e-01, -1.25000000e-01
def c34 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c35 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c36 = -1.25000000e-01, -1.25000000e-01, 1.25000000e-01, -1.25000000e-01
def c37 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c38 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c39 = 3.75000000e-01, -1.25000000e-01, -3.75000000e-01, 1.25000000e-01
def c40 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c41 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c42 = -1.25000000e-01, 1.25000000e-01, 1.25000000e-01, 1.25000000e-01
def c43 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c44 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c45 = 3.75000000e-01, 1.25000000e-01, -3.75000000e-01, 3.75000000e-01
def c46 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c47 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c48 = -1.25000000e-01, 3.75000000e-01, 1.25000000e-01, 3.75000000e-01
def c49 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 0.00000000e+00
def c50 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 6.25000000e-02
def c51 = 3.75000000e-01, 3.75000000e-01, 0.00000000e+00, 0.00000000e+00
def c52 = 2.12599993e-01, 7.15200007e-01, 7.22000003e-02, 1.00000000e+00
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c1.xyzw
mov r0.zw, c28.xxxy
mov r2.xy, r1.xyxx
mul r0.zw, r0.xxzw, r2.xxxy
add r0.zw, r0.xxxy, r0.xxzw
texld r2.xyzw, r0.zwxx, s0.xyzw
mov r0.z, r2.x
mov r3.x, r0.z
mov r0.z, r2.xxyx
mov r3.y, r0.z
mov r0.z, r2.xxzx
mov r3.z, r0.z
mov r2.xyz, r3.xyzx
mov r4.xyz, c29.xyzx
dp3 r0.z, r2.xyzx, r4.xyzx
mov r0.w, c28.z
max r0.z, r0.z, r0.w
mul r0.z, r0.z, r0.z
mul r0.z, r0.z, r0.z
mov r2.xy, c30.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c31.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c28.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c30.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c32.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c29.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c33.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c34.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c31.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c33.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c35.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c32.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c36.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c37.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c34.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c36.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c38.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c35.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c39.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c40.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c37.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c39.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c41.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c38.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c42.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c43.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c40.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c42.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c44.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c41.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c45.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c46.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c43.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c45.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c47.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c44.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c48.xyxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c49.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c46.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c48.zwxx
mov r2.zw, r1.xxxy
mul r2.xy, r2.xyxx, r2.zwxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r3.x, r0.w
mov r0.w, r2.xxxy
mov r3.y, r0.w
mov r0.w, r2.xxxz
mov r3.z, r0.w
mov r2.xyz, r3.xyzx
mov r4.xyz, c50.xyzx
dp3 r0.w, r2.xyzx, r4.xyzx
mov r2.x, c47.w
max r0.w, r0.w, r2.x
mul r0.w, r0.w, r0.w
mul r0.w, r0.w, r0.w
add r0.z, r0.z, r0.w
mov r2.xy, c51.xyxx
mov r1.xy, r1.xyxx
mul r1.xy, r2.xyxx, r1.xyxx
add r0.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r0.xyxx, s0.xyzw
mov r0.x, r1.x
mov r3.x, r0.x
mov r0.x, r1.yxxx
mov r3.y, r0.x
mov r0.x, r1.zxxx
mov r3.z, r0.x
mov r0.xyw, r3.xyxz
mov r1.xyz, c52.xyzx
dp3 r0.x, r0.xywx, r1.xyzx
mov r0.y, c49.w
max r0.x, r0.x, r0.y
mul r0.x, r0.x, r0.x
mul r0.x, r0.x, r0.x
add r0.x, r0.z, r0.x
mov r0.y, c50.w
mul r0.x, r0.x, r0.y
mov r1.x, c51.z
mov r0.y, r1.x
mov r1.x, c51.w
mov r0.z, r1.x
mov r1.x, c52.w
mov r0.w, r1.x
mov r0.xyzw, r0.xyzw
mov oC0.xyzw, r0.xyzw
