vs_3_0
dcl_position0 v0
dcl_normal0 v1
dcl_texcoord0 v2
dcl_position0 o0
dcl_texcoord0 o1
dcl_texcoord1 o2
dcl_texcoord2 o3
mov r0.xyzw, v2.xyzw
mov r1.xyzw, v1.xyzw
mov r2.xyzw, v0.xyzw
mov r3.xyzw, c4.xyzw
mov r4.xyzw, c0.xyzw
mov r5.xyzw, c1.xyzw
mov r6.xyzw, c2.xyzw
mov r7.xyzw, c3.xyzw
mov r8.x, r2.x
mov r8.y, r4.x
mul r8.x, r8.x, r8.y
mov r8.y, r2.xyxx
mov r8.z, r4.xxyx
mul r8.y, r8.y, r8.z
add r8.x, r8.x, r8.y
mov r8.y, r2.xzxx
mov r8.z, r4.xxzx
mul r8.y, r8.y, r8.z
add r8.x, r8.x, r8.y
mov r4.x, r4.wxxx
add r4.x, r8.x, r4.x
mov r4.y, r2.x
mov r4.z, r5.x
mul r4.y, r4.y, r4.z
mov r4.z, r2.xxyx
mov r4.w, r5.xxxy
mul r4.z, r4.z, r4.w
add r4.y, r4.y, r4.z
mov r4.z, r2.xxzx
mov r4.w, r5.xxxz
mul r4.z, r4.z, r4.w
add r4.y, r4.y, r4.z
mov r4.z, r5.xxwx
add r4.y, r4.y, r4.z
mov r4.z, r2.x
mov r4.w, r6.x
mul r4.z, r4.z, r4.w
mov r4.w, r2.xxxy
mov r5.x, r6.yxxx
mul r4.w, r4.w, r5.x
add r4.z, r4.z, r4.w
mov r4.w, r2.xxxz
mov r5.x, r6.zxxx
mul r4.w, r4.w, r5.x
add r4.z, r4.z, r4.w
mov r4.w, r6.xxxw
add r4.z, r4.z, r4.w
mov r4.w, r2.x
mov r5.x, r7.x
mul r4.w, r4.w, r5.x
mov r5.x, r2.yxxx
mov r5.y, r7.xyxx
mul r5.x, r5.x, r5.y
add r4.w, r4.w, r5.x
mov r5.x, r2.zxxx
mov r5.y, r7.xzxx
mul r5.x, r5.x, r5.y
add r4.w, r4.w, r5.x
mov r5.x, r7.wxxx
add r4.w, r4.w, r5.x
mov r5.x, r4.x
mov r5.y, r4.y
mov r5.z, r4.z
mov r5.w, r4.w
mov r4.xyzw, r5.xyzw
mov r0.xy, r0.xyxx
mov r0.zw, r3.xxxy
add r0.xy, r0.xyxx, r0.zwxx
mov o0.xyzw, r4.xyzw
mov o1.xy, r0.xyxx
mov r0.xyz, r2.xyzx
mov o2.xyz, r0.xyzx
mov r0.xyz, r1.xyzx
mov o3.xyz, r0.xyzx
