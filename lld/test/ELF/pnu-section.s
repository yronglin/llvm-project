# REQUIRES: x86

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/a.s -o %t/a.o
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/b.s -o %t/b.o
# RUN: ld.lld %t/a.o %t/b.o -o %t/out
# RUN: llvm-readelf -x .rodata %t/out | FileCheck %s --check-prefix=HEX
# RUN: llvm-nm -n %t/out | FileCheck %s --check-prefix=NM

# HEX:      Hex dump of section '.rodata':
# HEX-NEXT: 0x{{[0-9a-f]+}} 01000000 02000000 03000000

# NM-DAG: [[#%x,BASE:]] R pnu_123
# NM-DAG: {{0*}}[[#BASE]] R pnu_123_b
# NM-DAG: {{0*}}[[#BASE+4]] R pnu_23

#--- a.s
.text
.globl _start
_start:
  .quad pnu_123
  .quad pnu_23
  .quad pnu_123_b

.section .rodata.pnu,"a",@progbits
.p2align 2
.globl pnu_123
.type pnu_123,@object
.size pnu_123,12
pnu_123:
  .long 1
  .long 2
  .long 3

.p2align 2
.globl pnu_23
.type pnu_23,@object
.size pnu_23,8
pnu_23:
  .long 2
  .long 3

#--- b.s
.section .rodata.pnu,"a",@progbits
.p2align 2
.globl pnu_123_b
.type pnu_123_b,@object
.size pnu_123_b,12
pnu_123_b:
  .long 1
  .long 2
  .long 3
