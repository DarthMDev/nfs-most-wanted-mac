ps_3_0
dcl_texcoord0 v0
def c28 = 0.00000000e+00, 1.38461542e+00, 1.38461542e+00, 0.00000000e+00
def c29 = 2.27027029e-01, 2.27027029e-01, 2.27027029e-01, 2.27027029e-01
def c30 = 3.16216230e-01, 3.16216230e-01, 3.16216230e-01, 3.16216230e-01
def c31 = 1.38461542e+00, 1.38461542e+00, 3.23076916e+00, 3.23076916e+00
def c32 = 3.16216230e-01, 3.16216230e-01, 3.16216230e-01, 3.16216230e-01
def c33 = 7.02702701e-02, 7.02702701e-02, 7.02702701e-02, 7.02702701e-02
def c34 = 3.23076916e+00, 3.23076916e+00, 0.00000000e+00, 0.00000000e+00
def c35 = 7.02702701e-02, 7.02702701e-02, 7.02702701e-02, 7.02702701e-02
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c9.xyzw
mov r2.xyzw, c2.xyzw
mov r0.z, r2.xxwx
mov r0.w, r1.x
mul r0.z, r0.z, r0.w
mov r0.w, r2.xxxy
mul r0.z, r0.w, r0.z
mov r0.w, c28.x
mov r1.x, r0.w
mov r1.y, r0.z
mov r0.zw, r1.xxxy
texld r1.xyzw, r0.xyxx, s0.xyzw
mov r2.xyzw, c29.xyzw
mul r1.xyzw, r1.xyzw, r2.xyzw
mov r2.x, r1.x
mov r3.x, r1.yxxx
mov r2.y, r3.x
mov r3.x, r1.zxxx
mov r2.z, r3.x
mov r1.x, r1.wxxx
mov r2.w, r1.x
mov r1.xy, c28.yzxx
mul r1.xy, r0.zwxx, r1.xyxx
add r1.xy, r0.xyxx, r1.xyxx
texld r1.xyzw, r1.xyxx, s0.xyzw
mov r3.xyzw, c30.xyzw
mul r1.xyzw, r1.xyzw, r3.xyzw
mov r2.xyzw, r2.xyzw
add r1.xyzw, r2.xyzw, r1.xyzw
mov r2.xy, c31.xyxx
mul r2.xy, r0.zwxx, r2.xyxx
mov r2.xy, -r2.xyxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r3.xyzw, c32.xyzw
mul r2.xyzw, r2.xyzw, r3.xyzw
add r1.xyzw, r1.xyzw, r2.xyzw
mov r2.xy, c31.zwxx
mul r2.xy, r0.zwxx, r2.xyxx
add r2.xy, r0.xyxx, r2.xyxx
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r3.xyzw, c33.xyzw
mul r2.xyzw, r2.xyzw, r3.xyzw
add r1.xyzw, r1.xyzw, r2.xyzw
mov r2.xy, c34.xyxx
mul r0.zw, r0.xxzw, r2.xxxy
mov r0.zw, -r0.xxzw
add r0.xy, r0.xyxx, r0.zwxx
texld r0.xyzw, r0.xyxx, s0.xyzw
mov r2.xyzw, c35.xyzw
mul r0.xyzw, r0.xyzw, r2.xyzw
add r0.xyzw, r1.xyzw, r0.xyzw
mov oC0.xyzw, r0.xyzw
