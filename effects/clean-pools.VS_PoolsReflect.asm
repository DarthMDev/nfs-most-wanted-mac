vs_3_0
dcl_position0 v0
dcl_normal0 v1
dcl_texcoord0 v2
dcl_tangent v3
dcl_position0 o0
dcl_texcoord0 o1
dcl_texcoord1 o2
dcl_texcoord2 o3
dcl_texcoord3 o4
mov r0.xyzw, v3.xyzw
mov r1.xyzw, v2.xyzw
mov r2.xyzw, v1.xyzw
mov r3.xyzw, v0.xyzw
mov r4.xyzw, c4.xyzw
mov r5.xyzw, c0.xyzw
mov r6.xyzw, c1.xyzw
mov r7.xyzw, c2.xyzw
mov r8.xyzw, c3.xyzw
mov r9.x, r3.x
mov r9.y, r5.x
mul r9.x, r9.x, r9.y
mov r9.y, r3.xyxx
mov r9.z, r5.xxyx
mul r9.y, r9.y, r9.z
add r9.x, r9.x, r9.y
mov r9.y, r3.xzxx
mov r9.z, r5.xxzx
mul r9.y, r9.y, r9.z
add r9.x, r9.x, r9.y
mov r5.x, r5.wxxx
add r5.x, r9.x, r5.x
mov r5.y, r3.x
mov r5.z, r6.x
mul r5.y, r5.y, r5.z
mov r5.z, r3.xxyx
mov r5.w, r6.xxxy
mul r5.z, r5.z, r5.w
add r5.y, r5.y, r5.z
mov r5.z, r3.xxzx
mov r5.w, r6.xxxz
mul r5.z, r5.z, r5.w
add r5.y, r5.y, r5.z
mov r5.z, r6.xxwx
add r5.y, r5.y, r5.z
mov r5.z, r3.x
mov r5.w, r7.x
mul r5.z, r5.z, r5.w
mov r5.w, r3.xxxy
mov r6.x, r7.yxxx
mul r5.w, r5.w, r6.x
add r5.z, r5.z, r5.w
mov r5.w, r3.xxxz
mov r6.x, r7.zxxx
mul r5.w, r5.w, r6.x
add r5.z, r5.z, r5.w
mov r5.w, r7.xxxw
add r5.z, r5.z, r5.w
mov r5.w, r3.x
mov r6.x, r8.x
mul r5.w, r5.w, r6.x
mov r6.x, r3.yxxx
mov r6.y, r8.xyxx
mul r6.x, r6.x, r6.y
add r5.w, r5.w, r6.x
mov r6.x, r3.zxxx
mov r6.y, r8.xzxx
mul r6.x, r6.x, r6.y
add r5.w, r5.w, r6.x
mov r6.x, r8.wxxx
add r5.w, r5.w, r6.x
mov r6.x, r5.x
mov r6.y, r5.y
mov r6.z, r5.z
mov r6.w, r5.w
mov r5.xyzw, r6.xyzw
mov r1.xy, r1.xyxx
mov r1.zw, r4.xxxy
add r1.xy, r1.xyxx, r1.zwxx
mov o0.xyzw, r5.xyzw
mov o1.xy, r1.xyxx
mov r1.xyz, r3.xyzx
mov o2.xyz, r1.xyzx
mov r1.xyz, r2.xyzx
mov o3.xyz, r1.xyzx
mov o4.xyzw, r0.xyzw
