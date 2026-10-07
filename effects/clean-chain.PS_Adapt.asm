ps_3_0
dcl_texcoord0 v0
def c28 = 1.44269502e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c29 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
def c30 = 1.00000000e+00, 0.00000000e+00, 5.00000000e-01, 0.00000000e+00
def c31 = 1.00000000e+00, 1.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c32 = 1.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
def c33 = 9.99999975e-05, 0.00000000e+00, 1.00000000e+00, 1.44269502e+00
def c34 = 1.00000000e+00, 5.00000000e-01, 0.00000000e+00, 1.00000000e+00
def c35 = -1.00000000e+00, 5.00000000e-01, 0.00000000e+00, 1.00000000e+00
def c36 = -1.00000000e+00, 5.00000000e-01, 0.00000000e+00, 1.00000000e+00
def c37 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 0.00000000e+00
dcl_2d s1
dcl_2d s0
mov r0.xy, v0.xyxx
mov r1.xyzw, c5.xyzw
mov r2.xyzw, c0.xyzw
texld r3.xyzw, r0.xyxx, s0.xyzw
mov r0.z, r3.x
mov r0.w, c28.x
mul r0.z, r0.w, r0.z
exp r0.z, r0.z
texld r3.xyzw, r0.xyxx, s1.xyzw
mov r0.x, r1.zxxx
mov r0.y, r0.z
mov r0.y, r0.y
mov r0.y, -r0.y
add r0.x, r0.x, r0.y
mov r0.y, c28.y
mov r0.w, c28.z
cmp r0.x, r0.x, r0.y, r0.w
mov r0.y, c28.w
mov r0.w, c29.x
mov r0.x, -r0.x
cmp r0.x, r0.x, r0.w, r0.y
mov r0.y, r3.x
mov r0.y, r0.y
mov r0.w, c29.y
mov r0.y, r0.y
mov r0.y, -r0.y
add r0.y, r0.w, r0.y
mov r0.w, c29.z
mov r4.x, c29.w
cmp r0.y, r0.y, r0.w, r4.x
mov r0.y, r0.y
mov r0.w, c30.x
mov r4.x, c30.y
mov r0.y, r0.y
mov r0.y, -r0.y
cmp r0.y, r0.y, r4.x, r0.w
mov r0.w, r2.xxxy
mov r0.w, r0.w
mov r4.x, c30.z
mov r0.w, r0.w
mov r0.w, -r0.w
add r0.w, r4.x, r0.w
mov r4.x, c30.w
mov r4.y, c31.x
cmp r0.w, r0.w, r4.x, r4.y
mov r0.w, r0.w
mov r4.x, c31.y
mov r4.y, c31.z
mov r0.w, r0.w
mov r0.w, -r0.w
cmp r0.w, r0.w, r4.y, r4.x
mov r4.x, r3.x
mov r4.y, r0.z
mov r4.y, r4.y
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c31.w
mov r4.z, c32.x
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c32.y
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c32.z
mov r4.z, c32.w
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.y, r1.x
mov r4.z, r1.xxyx
mov r4.w, r1.x
mov r4.w, -r4.w
add r4.z, r4.z, r4.w
mul r4.x, r4.x, r4.z
add r4.x, r4.y, r4.x
mov r4.y, c33.x
max r4.x, r4.x, r4.y
mov r2.y, c33.y
max r2.x, r2.x, r2.y
mov r2.y, c33.z
mov r2.x, -r2.x
rcp r2.z, r4.x
mul r2.x, r2.x, r2.z
mov r2.z, c33.w
mul r2.x, r2.z, r2.x
exp r2.x, r2.x
mov r2.x, -r2.x
add r2.x, r2.y, r2.x
mov r2.y, r3.x
mov r2.z, r3.x
mov r2.z, -r2.z
add r2.z, r0.z, r2.z
mul r2.x, r2.z, r2.x
add r2.x, r2.y, r2.x
mov r2.y, c34.x
mov r2.z, -r0.y
add r2.y, r2.y, r2.z
max r2.y, r0.w, r2.y
mov r2.z, -r2.x
add r0.z, r0.z, r2.z
mul r0.z, r2.y, r0.z
add r0.z, r2.x, r0.z
mov r1.x, r1.wxxx
max r0.z, r0.z, r1.x
mov r0.y, r0.y
mov r1.x, c34.y
mov r0.y, r0.y
mov r0.y, -r0.y
add r0.y, r1.x, r0.y
mov r1.x, c34.z
mov r1.y, c34.w
cmp r0.y, r0.y, r1.x, r1.y
mov r0.y, r0.y
mov r1.x, r3.x
mov r1.y, c35.x
mov r0.y, r0.y
mov r0.y, -r0.y
cmp r0.y, r0.y, r1.y, r1.x
mov r0.w, r0.w
mov r1.x, c35.y
mov r0.w, r0.w
mov r0.w, -r0.w
add r0.w, r1.x, r0.w
mov r1.x, c35.z
mov r1.y, c35.w
cmp r0.w, r0.w, r1.x, r1.y
mov r0.w, r0.w
mov r1.x, c36.x
mov r0.w, r0.w
mov r0.w, -r0.w
cmp r0.y, r0.w, r0.y, r1.x
mov r0.w, c36.y
mov r0.x, -r0.x
add r0.x, r0.w, r0.x
mov r0.w, c36.z
mov r1.x, c36.w
cmp r0.x, r0.x, r0.w, r1.x
mov r0.x, -r0.x
cmp r0.x, r0.x, r0.y, r0.z
mov r1.x, c37.x
mov r0.y, r1.x
mov r1.x, c37.y
mov r0.z, r1.x
mov r1.x, c37.z
mov r0.w, r1.x
mov r0.xyzw, r0.xyzw
mov oC0.xyzw, r0.xyzw
