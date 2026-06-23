// RUN: llvm-mc -filetype=obj -triple x86_64-pc-linux-gnu < %s | llvm-readobj -r - | FileCheck %s

// CHECK:      Relocations [
// CHECK:        Section {{.*}} .rela.text {
// CHECK-NEXT:     0x3 R_X86_64_PC32 pnu_local 0xFFFFFFFFFFFFFFFC
// CHECK-NEXT:     0x7 R_X86_64_64 pnu_local 0x0
// CHECK-NEXT:   }
// CHECK-NEXT: ]

  .text
  leaq pnu_local(%rip), %rax
  .quad pnu_local

  .section .rodata.pnu,"a",@progbits
  .type pnu_local,@object
  .size pnu_local,4
pnu_local:
  .long 1
