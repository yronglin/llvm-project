; NOTE: Assertions have been autogenerated by utils/update_analyze_test_checks.py UTC_ARGS: --version 3
; RUN: opt -passes="print<cost-model>" -cost-kind=all 2>&1 -disable-output -mtriple aarch64-linux-gnu -mattr=+sve -S -o - < %s | FileCheck %s --check-prefixes=CHECK,CHECK-NOBF16
; RUN: opt -passes="print<cost-model>" -cost-kind=all 2>&1 -disable-output -mtriple aarch64-linux-gnu -mattr=+sve,+bf16 -S -o - < %s | FileCheck %s --check-prefixes=CHECK,CHECK-BF16

target datalayout = "e-m:e-i8:8:32-i16:16:32-i64:64-i128:128-n32:64-S128"
target triple = "aarch64-unknown-linux-gnu"

define void @sve_fptruncs() {
; CHECK-LABEL: 'sve_fptruncs'
; CHECK-NEXT:  Cost Model: Found costs of 1 for: %nxv2_f16_from_f32 = fptrunc <vscale x 2 x float> undef to <vscale x 2 x half>
; CHECK-NEXT:  Cost Model: Found costs of 1 for: %nxv4_f16_from_f32 = fptrunc <vscale x 4 x float> undef to <vscale x 4 x half>
; CHECK-NEXT:  Cost Model: Found costs of RThru:3 CodeSize:1 Lat:1 SizeLat:1 for: %nxv8_f16_from_f32 = fptrunc <vscale x 8 x float> undef to <vscale x 8 x half>
; CHECK-NEXT:  Cost Model: Found costs of 1 for: %nxv2_f16_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x half>
; CHECK-NEXT:  Cost Model: Found costs of RThru:3 CodeSize:1 Lat:1 SizeLat:1 for: %nxv4_f16_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x half>
; CHECK-NEXT:  Cost Model: Found costs of RThru:7 CodeSize:1 Lat:1 SizeLat:1 for: %nxv8_f16_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x half>
; CHECK-NEXT:  Cost Model: Found costs of 1 for: %nxv2_f32_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x float>
; CHECK-NEXT:  Cost Model: Found costs of RThru:3 CodeSize:1 Lat:1 SizeLat:1 for: %nxv4_f32_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x float>
; CHECK-NEXT:  Cost Model: Found costs of RThru:6 CodeSize:1 Lat:1 SizeLat:1 for: %nxv8_f32_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x float>
; CHECK-NEXT:  Cost Model: Found costs of RThru:0 CodeSize:1 Lat:1 SizeLat:1 for: ret void
;
  %nxv2_f16_from_f32 = fptrunc <vscale x 2 x float> undef to <vscale x 2 x half>
  %nxv4_f16_from_f32 = fptrunc <vscale x 4 x float> undef to <vscale x 4 x half>
  %nxv8_f16_from_f32 = fptrunc <vscale x 8 x float> undef to <vscale x 8 x half>

  %nxv2_f16_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x half>
  %nxv4_f16_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x half>
  %nxv8_f16_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x half>

  %nxv2_f32_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x float>
  %nxv4_f32_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x float>
  %nxv8_f32_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x float>

  ret void
}

define void @sve_fptruncs_bf16() {
; CHECK-LABEL: 'sve_fptruncs_bf16'
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv2_f16_from_f32 = fptrunc <vscale x 2 x float> undef to <vscale x 2 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv4_f16_from_f32 = fptrunc <vscale x 4 x float> undef to <vscale x 4 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv8_f16_from_f32 = fptrunc <vscale x 8 x float> undef to <vscale x 8 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv2_f16_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv4_f16_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:Invalid CodeSize:1 Lat:1 SizeLat:1 for: %nxv8_f16_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x bfloat>
; CHECK-NEXT:  Cost Model: Found costs of RThru:0 CodeSize:1 Lat:1 SizeLat:1 for: ret void
;
  %nxv2_f16_from_f32 = fptrunc <vscale x 2 x float> undef to <vscale x 2 x bfloat>
  %nxv4_f16_from_f32 = fptrunc <vscale x 4 x float> undef to <vscale x 4 x bfloat>
  %nxv8_f16_from_f32 = fptrunc <vscale x 8 x float> undef to <vscale x 8 x bfloat>

  %nxv2_f16_from_f64 = fptrunc <vscale x 2 x double> undef to <vscale x 2 x bfloat>
  %nxv4_f16_from_f64 = fptrunc <vscale x 4 x double> undef to <vscale x 4 x bfloat>
  %nxv8_f16_from_f64 = fptrunc <vscale x 8 x double> undef to <vscale x 8 x bfloat>

  ret void
}
;; NOTE: These prefixes are unused and the list is autogenerated. Do not add tests below this line:
; CHECK-BF16: {{.*}}
; CHECK-NOBF16: {{.*}}
