# REQUIRES: x86

# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/writable.s -o %t/writable.o
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/oob.s -o %t/oob.o
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/overlap.s -o %t/overlap.o
# RUN: llvm-mc -filetype=obj -triple=x86_64 %t/reloc.s -o %t/reloc.o

# RUN: not ld.lld %t/writable.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=WRITABLE --implicit-check-not=error:
# RUN: not ld.lld %t/oob.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=OOB --implicit-check-not=error:
# RUN: not ld.lld %t/overlap.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=OVERLAP --implicit-check-not=error:
# RUN: not ld.lld %t/reloc.o -o /dev/null 2>&1 | FileCheck %s --check-prefix=RELOC --implicit-check-not=error:

# WRITABLE: error: {{.*}}writable.o:(.rodata.pnu): writable .rodata.pnu section is not supported
# OOB: error: {{.*}}oob.o:(.rodata.pnu): symbol 'pnu_oob' range is outside .rodata.pnu
# OVERLAP: error: {{.*}}overlap.o:(.rodata.pnu): overlapping .rodata.pnu symbol ranges are not supported
# RELOC: error: {{.*}}reloc.o:(.rodata.pnu): relocations in .rodata.pnu sections are not supported

#--- writable.s
.globl _start
_start:
  ret

.section .rodata.pnu,"aw",@progbits
.type pnu_writable,@object
.size pnu_writable,4
pnu_writable:
  .long 1

#--- oob.s
.globl _start
_start:
  ret

.section .rodata.pnu,"a",@progbits
.type pnu_oob,@object
.size pnu_oob,8
pnu_oob:
  .long 1

#--- overlap.s
.globl _start
_start:
  ret

.section .rodata.pnu,"a",@progbits
.type pnu_a,@object
.size pnu_a,8
pnu_a:
  .long 1
.type pnu_b,@object
.size pnu_b,8
pnu_b:
  .long 2
  .long 3

#--- reloc.s
.globl _start
_start:
  ret

.section .rodata.pnu,"a",@progbits
.type pnu_reloc,@object
.size pnu_reloc,8
pnu_reloc:
  .quad _start
