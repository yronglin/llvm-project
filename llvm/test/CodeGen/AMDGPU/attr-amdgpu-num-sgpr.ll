; RUN: llc -mtriple=amdgcn--amdhsa -mcpu=fiji < %s | FileCheck -check-prefix=ALL %s

; FIXME: Vectorization can increase required SGPR count beyond limit.

; ALL-LABEL: {{^}}max_10_sgprs:

; ALL: SGPRBlocks: 2
; ALL: NumSGPRsForWavesPerEU: 24
define amdgpu_kernel void @max_10_sgprs() #0 {
  %one = load volatile i32, ptr addrspace(4) poison
  %two = load volatile i32, ptr addrspace(4) poison
  %three = load volatile i32, ptr addrspace(4) poison
  %four = load volatile i32, ptr addrspace(4) poison
  %five = load volatile i32, ptr addrspace(4) poison
  %six = load volatile i32, ptr addrspace(4) poison
  %seven = load volatile i32, ptr addrspace(4) poison
  %eight = load volatile i32, ptr addrspace(4) poison
  %nine = load volatile i32, ptr addrspace(4) poison
  %ten = load volatile i32, ptr addrspace(4) poison
  %eleven = load volatile i32, ptr addrspace(4) poison
  call void asm sideeffect "", "s,s,s,s,s,s,s,s,s,s"(i32 %one, i32 %two, i32 %three, i32 %four, i32 %five, i32 %six, i32 %seven, i32 %eight, i32 %nine, i32 %ten)
  store volatile i32 %one, ptr addrspace(1) poison
  store volatile i32 %two, ptr addrspace(1) poison
  store volatile i32 %three, ptr addrspace(1) poison
  store volatile i32 %four, ptr addrspace(1) poison
  store volatile i32 %five, ptr addrspace(1) poison
  store volatile i32 %six, ptr addrspace(1) poison
  store volatile i32 %seven, ptr addrspace(1) poison
  store volatile i32 %eight, ptr addrspace(1) poison
  store volatile i32 %nine, ptr addrspace(1) poison
  store volatile i32 %ten, ptr addrspace(1) poison
  store volatile i32 %eleven, ptr addrspace(1) poison
  ret void
}

; private resource: 4
; scratch wave offset: 1
; workgroup ids: 3
; dispatch id: 2
; queue ptr: 2
; flat scratch init: 2
; ---------------------
; total: 14

; + reserved vcc = 16

; Because we can't handle re-using the last few input registers as the
; special vcc etc. registers (as well as decide to not use the unused
; features when the number of registers is frozen), this ends up using
; more than expected.

; XALL-LABEL: {{^}}max_12_sgprs_14_input_sgprs:
; XTOSGPR: SGPRBlocks: 1
; XTOSGPR: NumSGPRsForWavesPerEU: 16

; This test case is disabled: When calculating the spillslot addresses AMDGPU
; creates an extra vreg to save/restore m0 which in a point of maximum register
; pressure would trigger an endless loop; the compiler aborts earlier with
; "Incomplete scavenging after 2nd pass" in practice.
;define amdgpu_kernel void @max_12_sgprs_14_input_sgprs(ptr addrspace(1) %out1,
;                                        ptr addrspace(1) %out2,
;                                        ptr addrspace(1) %out3,
;                                        ptr addrspace(1) %out4,
;                                        i32 %one, i32 %two, i32 %three, i32 %four) #2 {
;  %x.0 = call i32 @llvm.amdgcn.workgroup.id.x()
;  %x.1 = call i32 @llvm.amdgcn.workgroup.id.y()
;  %x.2 = call i32 @llvm.amdgcn.workgroup.id.z()
;  %x.3 = call i64 @llvm.amdgcn.dispatch.id()
;  %x.4 = call ptr addrspace(4) @llvm.amdgcn.dispatch.ptr()
;  %x.5 = call ptr addrspace(4) @llvm.amdgcn.queue.ptr()
;  store volatile i32 0, ptr poison
;  br label %stores
;
;stores:
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  store volatile i64 %x.3, ptr addrspace(1) poison
;  store volatile ptr addrspace(4) %x.4, ptr addrspace(1) poison
;  store volatile ptr addrspace(4) %x.5, ptr addrspace(1) poison
;
;  store i32 %one, ptr addrspace(1) %out1
;  store i32 %two, ptr addrspace(1) %out2
;  store i32 %three, ptr addrspace(1) %out3
;  store i32 %four, ptr addrspace(1) %out4
;  ret void
;}

; The following test is commented out for now; http://llvm.org/PR31230
; XALL-LABEL: max_12_sgprs_12_input_sgprs{{$}}
; ; Make sure copies for input buffer are not clobbered. This requires
; ; swapping the order the registers are copied from what normally
; ; happens.

; XALL: SGPRBlocks: 2
; XALL: NumSGPRsForWavesPerEU: 18
;define amdgpu_kernel void @max_12_sgprs_12_input_sgprs(ptr addrspace(1) %out1,
;                                        ptr addrspace(1) %out2,
;                                        ptr addrspace(1) %out3,
;                                        ptr addrspace(1) %out4,
;                                        i32 %one, i32 %two, i32 %three, i32 %four) #2 {
;  store volatile i32 0, ptr poison
;  %x.0 = call i32 @llvm.amdgcn.workgroup.id.x()
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  %x.1 = call i32 @llvm.amdgcn.workgroup.id.y()
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  %x.2 = call i32 @llvm.amdgcn.workgroup.id.z()
;  store volatile i32 %x.0, ptr addrspace(1) poison
;  %x.3 = call i64 @llvm.amdgcn.dispatch.id()
;  store volatile i64 %x.3, ptr addrspace(1) poison
;  %x.4 = call ptr addrspace(4) @llvm.amdgcn.dispatch.ptr()
;  store volatile ptr addrspace(4) %x.4, ptr addrspace(1) poison
;
;  store i32 %one, ptr addrspace(1) %out1
;  store i32 %two, ptr addrspace(1) %out2
;  store i32 %three, ptr addrspace(1) %out3
;  store i32 %four, ptr addrspace(1) %out4
;  ret void
;}

declare i32 @llvm.amdgcn.workgroup.id.x() #1
declare i32 @llvm.amdgcn.workgroup.id.y() #1
declare i32 @llvm.amdgcn.workgroup.id.z() #1
declare i64 @llvm.amdgcn.dispatch.id() #1
declare ptr addrspace(4) @llvm.amdgcn.dispatch.ptr() #1
declare ptr addrspace(4) @llvm.amdgcn.queue.ptr() #1

attributes #0 = { nounwind "amdgpu-num-sgpr"="14" }
attributes #1 = { nounwind readnone }
attributes #2 = { nounwind "amdgpu-num-sgpr"="12" }
attributes #3 = { nounwind "amdgpu-num-sgpr"="11" }
