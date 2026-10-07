ps_3_0
dcl_texcoord0 v0
dcl_texcoord1 v1
dcl_texcoord2 v2
def c24 = 9.99999994e-09, 9.99999975e-05, 9.99999978e-03, 1.00000000e+00
def c25 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 2.50000000e-01
def c26 = 1.50000006e-01, 8.69565248e-01, 5.00000000e-01, 0.00000000e+00
def c27 = 1.00000000e+00, 1.00000000e+00, 0.00000000e+00, 9.99999975e-05
def c28 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 9.99999978e-03
def c29 = 1.00000000e+00, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
def c30 = 2.50000000e-01, 1.50000006e-01, 8.69565248e-01, 1.50000000e+00
def c31 = 0.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
def c32 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 9.99999975e-05
def c33 = 9.99999978e-03, 1.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c34 = 1.00000000e+00, 2.50000000e-01, 1.50000006e-01, 8.69565248e-01
def c35 = 2.50000000e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c36 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c37 = 9.99999975e-05, 9.99999978e-03, 1.00000000e+00, 0.00000000e+00
def c38 = 0.00000000e+00, 1.00000000e+00, 2.50000000e-01, 1.50000006e-01
def c39 = 8.69565248e-01, 3.50000000e+00, 0.00000000e+00, 1.00000000e+00
def c40 = 1.00000000e+00, 0.00000000e+00, 9.99999975e-05, 9.99999978e-03
def c41 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
def c42 = 0.00000000e+00, 0.00000000e+00, 1.00000000e+00, 2.50000000e-01
def c43 = 1.50000006e-01, 8.69565248e-01, 4.50000000e+00, 0.00000000e+00
def c44 = 1.00000000e+00, 1.00000000e+00, 0.00000000e+00, 9.99999975e-05
def c45 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 9.99999978e-03
def c46 = 1.00000000e+00, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
def c47 = 2.50000000e-01, 1.50000006e-01, 8.69565248e-01, 5.50000000e+00
def c48 = 0.00000000e+00, 1.00000000e+00, 1.00000000e+00, 0.00000000e+00
def c49 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 9.99999975e-05
def c50 = 9.99999978e-03, 1.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c51 = 1.00000000e+00, 2.50000000e-01, 1.50000006e-01, 8.69565248e-01
def c52 = 6.50000000e+00, 0.00000000e+00, 1.00000000e+00, 1.00000000e+00
def c53 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 0.00000000e+00
def c54 = 9.99999975e-05, 9.99999978e-03, 1.00000000e+00, 0.00000000e+00
def c55 = 0.00000000e+00, 1.00000000e+00, 2.50000000e-01, 1.50000006e-01
def c56 = 8.69565248e-01, 7.50000000e+00, 0.00000000e+00, 1.00000000e+00
def c57 = 1.00000000e+00, 0.00000000e+00, 9.99999997e-07, 9.99999978e-03
def c58 = 0.00000000e+00, 0.00000000e+00, 0.00000000e+00, 1.00000000e+00
def c59 = 3.00000000e+00, 2.00000000e+00, 0.00000000e+00, 0.00000000e+00
dcl_2d s0
mov r0.xyz, v2.xyzx
mov r1.xyz, v1.xyzx
mov r2.xy, v0.xyxx
mov r3.xyzw, c8.xyzw
mov r4.xyzw, c9.xyzw
mov r5.xyzw, c10.xyzw
mov r6.xyzw, c11.xyzw
mov r7.xyzw, c12.xyzw
mov r8.xyzw, c13.xyzw
mov r9.xyzw, c14.xyzw
mov r10.xyzw, c15.xyzw
mov r11.xyzw, c16.xyzw
mov r12.xyzw, c17.xyzw
mov r13.xyzw, c18.xyzw
mov r14.xyzw, c19.xyzw
mov r15.xyzw, c20.xyzw
mov r16.xyzw, c21.xyzw
mov r17.xyzw, c22.xyzw
mov r18.xyzw, c23.xyzw
mov r19.xyzw, c7.xyzw
mov r20.xyzw, c6.xyzw
mov r21.xyzw, c5.xyzw
texld r2.xyzw, r2.xyxx, s0.xyzw
mov r0.w, r2.x
mov r22.x, r0.w
mov r0.w, r2.xxxy
mov r22.y, r0.w
mov r0.w, r2.xxxz
mov r22.z, r0.w
mov r0.w, r2.xxxw
mov r22.w, r0.w
dp3 r0.w, r0.xyzx, r0.xyzx
mov r1.w, c24.x
max r0.w, r0.w, r1.w
rsq r0.w, r0.w
mov r23.xyz, r0.wwwx
mul r0.xyz, r0.xyzx, r23.xyzx
mov r0.w, r0.x
mov r23.x, r0.w
mov r0.w, r0.xxxy
mov r23.y, r0.w
mov r0.x, r0.zxxx
mov r23.z, r0.x
mov r0.xyz, r23.xyzx
mov r22.xyzw, r22.xyzw
mov r23.xyz, r3.xyzx
mov r24.xyz, -r1.xyzx
add r23.xyz, r23.xyzx, r24.xyzx
mov r0.w, r23.x
mov r24.x, r0.w
mov r0.w, r23.xxxy
mov r24.y, r0.w
mov r0.w, r23.xxxz
mov r24.z, r0.w
mov r23.xyz, r24.xyzx
mov r25.xyz, r24.xyzx
dp3 r0.w, r23.xyzx, r25.xyzx
mov r1.w, c24.y
max r0.w, r0.w, r1.w
mov r1.w, r3.xxxw
mov r3.x, c24.z
max r1.w, r1.w, r3.x
mul r3.x, r1.w, r1.w
rcp r3.x, r3.x
mul r3.x, r0.w, r3.x
mov r3.y, c24.w
mul r3.x, r3.x, r3.x
mov r3.x, -r3.x
add r3.x, r3.y, r3.x
mov_sat r3.x, r3.x
mul r3.x, r3.x, r3.x
mov r3.y, r4.xwxx
mov r3.y, r3.y
mov r3.z, c25.x
mov r3.y, r3.y
mov r3.y, -r3.y
add r3.y, r3.z, r3.y
mov r3.z, c25.y
mov r3.w, c25.z
cmp r3.y, r3.y, r3.z, r3.w
mov r3.y, r3.y
mov r3.z, r4.xxwx
mov r3.w, c25.w
mul r1.w, r3.w, r1.w
mov r3.y, r3.y
mov r3.y, -r3.y
cmp r1.w, r3.y, r1.w, r3.z
mul r1.w, r1.w, r1.w
add r3.y, r0.w, r1.w
rcp r3.y, r3.y
mul r1.w, r1.w, r3.y
mov r3.yzw, r24.xxyz
dp3 r3.y, r0.xyzx, r3.yzwx
rsq r0.w, r0.w
mul r0.w, r3.y, r0.w
mov r3.y, c26.x
add r0.w, r0.w, r3.y
mov r3.y, c26.y
mul r0.w, r0.w, r3.y
mov_sat r0.w, r0.w
mov r3.y, r20.x
mov r3.y, r3.y
mov r3.z, c26.z
mov r3.y, r3.y
mov r3.y, -r3.y
add r3.y, r3.z, r3.y
mov r3.z, c26.w
mov r3.w, c27.x
cmp r3.y, r3.y, r3.z, r3.w
mov r3.y, r3.y
mov r3.z, c27.y
mov r3.w, c27.z
mov r3.y, r3.y
mov r3.y, -r3.y
cmp r3.y, r3.y, r3.w, r3.z
mov r4.xyz, r4.xyzx
mov r23.xyz, c28.xyzx
max r4.xyz, r4.xyzx, r23.xyzx
mul r1.w, r3.x, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r3.y
mov r3.xyz, r0.wwwx
mul r3.xyz, r4.xyzx, r3.xyzx
mov r0.w, r3.x
mov r4.x, r0.w
mov r0.w, r3.xxxy
mov r4.y, r0.w
mov r0.w, r3.xxxz
mov r4.z, r0.w
mov r3.xyz, r5.xyzx
mov r23.xyz, -r1.xyzx
add r3.xyz, r3.xyzx, r23.xyzx
mov r0.w, r3.x
mov r24.x, r0.w
mov r0.w, r3.xxxy
mov r24.y, r0.w
mov r0.w, r3.xxxz
mov r24.z, r0.w
mov r3.xyz, r24.xyzx
mov r23.xyz, r24.xyzx
dp3 r0.w, r3.xyzx, r23.xyzx
mov r1.w, c27.w
max r0.w, r0.w, r1.w
mov r1.w, r5.xxxw
mov r3.x, c28.w
max r1.w, r1.w, r3.x
mul r3.x, r1.w, r1.w
rcp r3.x, r3.x
mul r3.x, r0.w, r3.x
mov r3.y, c29.x
mul r3.x, r3.x, r3.x
mov r3.x, -r3.x
add r3.x, r3.y, r3.x
mov_sat r3.x, r3.x
mul r3.x, r3.x, r3.x
mov r3.y, r6.xwxx
mov r3.y, r3.y
mov r3.z, c29.y
mov r3.y, r3.y
mov r3.y, -r3.y
add r3.y, r3.z, r3.y
mov r3.z, c29.z
mov r3.w, c29.w
cmp r3.y, r3.y, r3.z, r3.w
mov r3.y, r3.y
mov r3.z, r6.xxwx
mov r3.w, c30.x
mul r1.w, r3.w, r1.w
mov r3.y, r3.y
mov r3.y, -r3.y
cmp r1.w, r3.y, r1.w, r3.z
mul r1.w, r1.w, r1.w
add r3.y, r0.w, r1.w
rcp r3.y, r3.y
mul r1.w, r1.w, r3.y
mov r3.yzw, r24.xxyz
dp3 r3.y, r0.xyzx, r3.yzwx
rsq r0.w, r0.w
mul r0.w, r3.y, r0.w
mov r3.y, c30.y
add r0.w, r0.w, r3.y
mov r3.y, c30.z
mul r0.w, r0.w, r3.y
mov_sat r0.w, r0.w
mov r3.y, r20.x
mov r3.y, r3.y
mov r3.z, c30.w
mov r3.y, r3.y
mov r3.y, -r3.y
add r3.y, r3.z, r3.y
mov r3.z, c31.x
mov r3.w, c31.y
cmp r3.y, r3.y, r3.z, r3.w
mov r3.y, r3.y
mov r3.z, c31.z
mov r3.w, c31.w
mov r3.y, r3.y
mov r3.y, -r3.y
cmp r3.y, r3.y, r3.w, r3.z
mov r5.xyz, r6.xyzx
mov r6.xyz, c32.xyzx
max r5.xyz, r5.xyzx, r6.xyzx
mul r1.w, r3.x, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r3.y
mov r3.xyz, r0.wwwx
mul r3.xyz, r5.xyzx, r3.xyzx
mov r4.xyz, r4.xyzx
add r3.xyz, r4.xyzx, r3.xyzx
mov r4.xyz, r7.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c32.w
max r0.w, r0.w, r1.w
mov r1.w, r7.xxxw
mov r3.w, c33.x
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c33.y
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r8.wxxx
mov r4.y, c33.z
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c33.w
mov r4.z, c34.x
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r8.xwxx
mov r4.z, c34.y
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r4.x, r0.xyzx, r4.xyzx
rsq r0.w, r0.w
mul r0.w, r4.x, r0.w
mov r4.x, c34.z
add r0.w, r0.w, r4.x
mov r4.x, c34.w
mul r0.w, r0.w, r4.x
mov_sat r0.w, r0.w
mov r4.x, r20.x
mov r4.y, c35.x
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c35.y
mov r4.z, c35.z
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c35.w
mov r4.z, c36.x
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.yzw, r8.xxyz
mov r5.xyz, c36.yzwx
max r4.yzw, r4.xyzw, r5.xxyz
mul r1.w, r3.w, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r4.x
mov r5.xyz, r0.wwwx
mul r4.xyz, r4.yzwx, r5.xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mov r4.xyz, r9.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c37.x
max r0.w, r0.w, r1.w
mov r1.w, r9.xxxw
mov r3.w, c37.y
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c37.z
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r10.wxxx
mov r4.y, c37.w
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c38.x
mov r4.z, c38.y
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r10.xwxx
mov r4.z, c38.z
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r4.x, r0.xyzx, r4.xyzx
rsq r0.w, r0.w
mul r0.w, r4.x, r0.w
mov r4.x, c38.w
add r0.w, r0.w, r4.x
mov r4.x, c39.x
mul r0.w, r0.w, r4.x
mov_sat r0.w, r0.w
mov r4.x, r20.x
mov r4.y, c39.y
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c39.z
mov r4.z, c39.w
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c40.x
mov r4.z, c40.y
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.yzw, r10.xxyz
mov r5.xyz, c41.xyzx
max r4.yzw, r4.xyzw, r5.xxyz
mul r1.w, r3.w, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r4.x
mov r5.xyz, r0.wwwx
mul r4.xyz, r4.yzwx, r5.xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mov r4.xyz, r11.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c40.z
max r0.w, r0.w, r1.w
mov r1.w, r11.xxxw
mov r3.w, c40.w
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c41.w
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r12.wxxx
mov r4.y, c42.x
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c42.y
mov r4.z, c42.z
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r12.xwxx
mov r4.z, c42.w
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r4.x, r0.xyzx, r4.xyzx
rsq r0.w, r0.w
mul r0.w, r4.x, r0.w
mov r4.x, c43.x
add r0.w, r0.w, r4.x
mov r4.x, c43.y
mul r0.w, r0.w, r4.x
mov_sat r0.w, r0.w
mov r4.x, r20.x
mov r4.y, c43.z
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c43.w
mov r4.z, c44.x
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c44.y
mov r4.z, c44.z
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.yzw, r12.xxyz
mov r5.xyz, c45.xyzx
max r4.yzw, r4.xyzw, r5.xxyz
mul r1.w, r3.w, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r4.x
mov r5.xyz, r0.wwwx
mul r4.xyz, r4.yzwx, r5.xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mov r4.xyz, r13.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c44.w
max r0.w, r0.w, r1.w
mov r1.w, r13.xxxw
mov r3.w, c45.w
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c46.x
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r14.wxxx
mov r4.y, c46.y
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c46.z
mov r4.z, c46.w
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r14.xwxx
mov r4.z, c47.x
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r4.x, r0.xyzx, r4.xyzx
rsq r0.w, r0.w
mul r0.w, r4.x, r0.w
mov r4.x, c47.y
add r0.w, r0.w, r4.x
mov r4.x, c47.z
mul r0.w, r0.w, r4.x
mov_sat r0.w, r0.w
mov r4.x, r20.x
mov r4.y, c47.w
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c48.x
mov r4.z, c48.y
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c48.z
mov r4.z, c48.w
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.yzw, r14.xxyz
mov r5.xyz, c49.xyzx
max r4.yzw, r4.xyzw, r5.xxyz
mul r1.w, r3.w, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r4.x
mov r5.xyz, r0.wwwx
mul r4.xyz, r4.yzwx, r5.xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mov r4.xyz, r15.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c49.w
max r0.w, r0.w, r1.w
mov r1.w, r15.xxxw
mov r3.w, c50.x
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c50.y
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r16.wxxx
mov r4.y, c50.z
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c50.w
mov r4.z, c51.x
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r16.xwxx
mov r4.z, c51.y
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r4.x, r0.xyzx, r4.xyzx
rsq r0.w, r0.w
mul r0.w, r4.x, r0.w
mov r4.x, c51.z
add r0.w, r0.w, r4.x
mov r4.x, c51.w
mul r0.w, r0.w, r4.x
mov_sat r0.w, r0.w
mov r4.x, r20.x
mov r4.y, c52.x
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c52.y
mov r4.z, c52.z
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, c52.w
mov r4.z, c53.x
mov r4.x, -r4.x
cmp r4.x, r4.x, r4.z, r4.y
mov r4.yzw, r16.xxyz
mov r5.xyz, c53.yzwx
max r4.yzw, r4.xyzw, r5.xxyz
mul r1.w, r3.w, r1.w
mul r0.w, r1.w, r0.w
mul r0.w, r0.w, r4.x
mov r5.xyz, r0.wwwx
mul r4.xyz, r4.yzwx, r5.xyzx
add r3.xyz, r3.xyzx, r4.xyzx
mov r4.xyz, r17.xyzx
mov r5.xyz, -r1.xyzx
add r4.xyz, r4.xyzx, r5.xyzx
mov r0.w, r4.x
mov r24.x, r0.w
mov r0.w, r4.xxxy
mov r24.y, r0.w
mov r0.w, r4.xxxz
mov r24.z, r0.w
mov r4.xyz, r24.xyzx
mov r5.xyz, r24.xyzx
dp3 r0.w, r4.xyzx, r5.xyzx
mov r1.w, c54.x
max r0.w, r0.w, r1.w
mov r1.w, r17.xxxw
mov r3.w, c54.y
max r1.w, r1.w, r3.w
mul r3.w, r1.w, r1.w
rcp r3.w, r3.w
mul r3.w, r0.w, r3.w
mov r4.x, c54.z
mul r3.w, r3.w, r3.w
mov r3.w, -r3.w
add r3.w, r4.x, r3.w
mov_sat r3.w, r3.w
mul r3.w, r3.w, r3.w
mov r4.x, r18.wxxx
mov r4.y, c54.w
mov r4.x, -r4.x
add r4.x, r4.y, r4.x
mov r4.y, c55.x
mov r4.z, c55.y
cmp r4.x, r4.x, r4.y, r4.z
mov r4.y, r18.xwxx
mov r4.z, c55.z
mul r1.w, r4.z, r1.w
mov r4.x, -r4.x
cmp r1.w, r4.x, r1.w, r4.y
mul r1.w, r1.w, r1.w
add r4.x, r0.w, r1.w
rcp r4.x, r4.x
mul r1.w, r1.w, r4.x
mov r4.xyz, r24.xyzx
dp3 r0.x, r0.xyzx, r4.xyzx
rsq r0.y, r0.w
mul r0.x, r0.x, r0.y
mov r0.y, c55.w
add r0.x, r0.x, r0.y
mov r0.y, c56.x
mul r0.x, r0.x, r0.y
mov_sat r0.x, r0.x
mov r0.y, r20.x
mov r0.y, r0.y
mov r0.z, c56.y
mov r0.y, r0.y
mov r0.y, -r0.y
add r0.y, r0.z, r0.y
mov r0.z, c56.z
mov r0.w, c56.w
cmp r0.y, r0.y, r0.z, r0.w
mov r0.y, r0.y
mov r0.z, c57.x
mov r0.w, c57.y
mov r0.y, r0.y
mov r0.y, -r0.y
cmp r0.y, r0.y, r0.w, r0.z
mov r4.xyz, r18.xyzx
mov r5.xyz, c58.xyzx
max r4.xyz, r4.xyzx, r5.xyzx
mul r0.z, r3.w, r1.w
mul r0.x, r0.z, r0.x
mul r0.x, r0.x, r0.y
mov r0.xyz, r0.x
mul r0.xyz, r4.xyzx, r0.xyzx
add r0.xyz, r3.xyzx, r0.xyzx
mov r3.xyz, r21.xyzx
mov r1.xyz, -r1.xyzx
add r1.xyz, r3.xyzx, r1.xyzx
mov r0.w, r1.x
mov r3.x, r0.w
mov r0.w, r1.xxxy
mov r3.y, r0.w
mov r0.w, r1.xxxz
mov r3.z, r0.w
mov r1.xyz, r3.xyzx
mov r3.xyz, r3.xyzx
dp3 r0.w, r1.xyzx, r3.xyzx
mov r1.x, c57.z
max r0.w, r0.w, r1.x
rsq r1.x, r0.w
mul r0.w, r0.w, r1.x
mov r1.x, r19.yxxx
mov r1.y, r19.x
mov r1.y, -r1.y
add r1.x, r1.x, r1.y
mov r1.y, c57.w
max r1.x, r1.x, r1.y
mov r1.y, r19.x
mov r1.y, -r1.y
add r0.w, r0.w, r1.y
rcp r1.x, r1.x
mul r0.w, r0.w, r1.x
mov_sat r0.w, r0.w
mov r1.x, c58.w
mul r1.y, r0.w, r0.w
mov r1.z, c59.x
mov r1.w, c59.y
mul r0.w, r1.w, r0.w
mov r0.w, -r0.w
add r0.w, r1.z, r0.w
mul r0.w, r1.y, r0.w
mov r0.w, -r0.w
add r0.w, r1.x, r0.w
mov r1.xyz, r22.xyzx
mul r0.xyz, r1.xyzx, r0.xyzx
mov r1.x, r19.zxxx
mov r1.y, c59.z
max r1.x, r1.x, r1.y
mul r0.w, r0.w, r1.x
mov r1.xyz, r0.wwwx
mul r0.xyz, r0.xyzx, r1.xyzx
mov r0.w, r0.x
mov r1.x, r0.w
mov r0.w, r0.xxxy
mov r1.y, r0.w
mov r0.x, r0.zxxx
mov r1.z, r0.x
mov r0.x, r2.wxxx
mov r1.w, r0.x
mov r0.xyzw, r1.xyzw
mov oC0.xyzw, r0.xyzw
