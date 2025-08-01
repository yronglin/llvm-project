; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py UTC_ARGS: --version 4
; RUN: llc -global-isel=0 -mtriple=amdgcn -mcpu=gfx950 < %s | FileCheck -check-prefix=GFX950-SDAG %s
; RUN: llc -global-isel=1 -global-isel-abort=2 -mtriple=amdgcn -mcpu=gfx950 < %s | FileCheck -check-prefix=GFX950-GISEL %s

declare <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.bf16(<32 x bfloat> %src, float %scale)
declare <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.f16(<32 x half> %src, float %scale)
declare <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.bf16(<32 x bfloat> %src, float %scale)
declare <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.f16(<32 x half> %src, float %scale)

define amdgpu_ps void @test_scalef32_pk32_bf6_bf16_vv(<32 x bfloat> %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_bf16_vv:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[0:15], v16
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_bf16_vv:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[0:15], v16
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.bf16(<32 x bfloat> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_bf16_sl(<32 x bfloat> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_bf16_sl:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_bf16_sl:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], s0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.bf16(<32 x bfloat> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_f16_vv(<32 x half> %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_f16_vv:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[0:15], v16
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_f16_vv:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[0:15], v16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.f16(<32 x half> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_f16_sl(<32 x half> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_f16_sl:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_f16_sl:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], v24
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.f16(<32 x half> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_bf16_vv(<32 x bfloat> %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_bf16_vv:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[0:15], v16
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_bf16_vv:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[0:15], v16
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.bf16(<32 x bfloat> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_bf16_sl(<32 x bfloat> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_bf16_sl:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_bf16_sl:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], s0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.bf16(<32 x bfloat> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_f16_vv(<32 x half> %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_f16_vv:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[0:15], v16
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_f16_vv:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v17
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v18
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[0:15], v16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.f16(<32 x half> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_f16_sl(<32 x half> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_f16_sl:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_f16_sl:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], v24
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.f16(<32 x half> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_bf16_vv_inreg_src(<32 x bfloat> inreg %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_bf16_vv_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], v0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_bf16_vv_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], v0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.bf16(<32 x bfloat> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_bf16_sl_inreg_src(<32 x bfloat> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_bf16_sl_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_bf16_sl_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_bf16 v[18:23], v[2:17], s0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.bf16(<32 x bfloat> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_f16_vv_inreg_src(<32 x half> inreg %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_f16_vv_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], v0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_f16_vv_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], v0
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.f16(<32 x half> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_bf6_f16_sl_inreg_src(<32 x half> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_bf6_f16_sl_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_bf6_f16_sl_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_bf6_f16 v[18:23], v[2:17], v24
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.bf6.f16(<32 x half> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_bf16_vv_inreg_src(<32 x bfloat> inreg %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_bf16_vv_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], v0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_bf16_vv_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], v0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.bf16(<32 x bfloat> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_bf16_sl_inreg_src(<32 x bfloat> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_bf16_sl_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_bf16_sl_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-GISEL-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_bf16 v[18:23], v[2:17], s0
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.bf16(<32 x bfloat> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_f16_vv_inreg_src(<32 x half> inreg %src, float %scale, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_f16_vv_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], v0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_f16_vv_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v25, v2
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, v1
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], v0
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[24:25], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[24:25], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.f16(<32 x half> %src, float %scale)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

define amdgpu_ps void @test_scalef32_pk32_fp6_f16_sl_inreg_src(<32 x half> inreg %src, ptr addrspace(1) %out) {
; GFX950-SDAG-LABEL: test_scalef32_pk32_fp6_f16_sl_inreg_src:
; GFX950-SDAG:       ; %bb.0:
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v2, s0
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v3, s1
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v4, s2
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v5, s3
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v6, s4
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v7, s5
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v8, s6
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v9, s7
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v10, s8
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v11, s9
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v12, s10
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v13, s11
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v14, s12
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v15, s13
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v16, s14
; GFX950-SDAG-NEXT:    v_mov_b32_e32 v17, s15
; GFX950-SDAG-NEXT:    s_mov_b32 s0, 0x42c80000
; GFX950-SDAG-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], s0
; GFX950-SDAG-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-SDAG-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-SDAG-NEXT:    s_endpgm
;
; GFX950-GISEL-LABEL: test_scalef32_pk32_fp6_f16_sl_inreg_src:
; GFX950-GISEL:       ; %bb.0:
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[16:17], s[14:15]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[14:15], s[12:13]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[12:13], s[10:11]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[10:11], s[8:9]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[8:9], s[6:7]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[6:7], s[4:5]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[4:5], s[2:3]
; GFX950-GISEL-NEXT:    v_mov_b64_e32 v[2:3], s[0:1]
; GFX950-GISEL-NEXT:    v_mov_b32_e32 v24, 0x42c80000
; GFX950-GISEL-NEXT:    v_cvt_scalef32_pk32_fp6_f16 v[18:23], v[2:17], v24
; GFX950-GISEL-NEXT:    global_store_dwordx4 v[0:1], v[18:21], off
; GFX950-GISEL-NEXT:    global_store_dwordx2 v[0:1], v[22:23], off offset:16
; GFX950-GISEL-NEXT:    s_endpgm
  %cvt = tail call <6 x i32> @llvm.amdgcn.cvt.scalef32.pk32.fp6.f16(<32 x half> %src, float 100.0)
  store <6 x i32> %cvt, ptr addrspace(1) %out, align 8
  ret void
}

