# REQUIRES: arm
# RUN: rm -rf %t && split-file %s %t
# RUN: llvm-mc -triple=tc32-unknown-none-elf -filetype=obj %t/tc32-thunk.s -o %t/tc32-thunk.o
# RUN: ld.lld -T %t/tc32-thunk.lds %t/tc32-thunk.o -o %t/tc32-thunk
# RUN: llvm-objdump --triple=tc32 -d %t/tc32-thunk | FileCheck %s

# CHECK-LABEL: <_start>:
# CHECK-NEXT:    {{[0-9a-f]+}}: 00 90 02 98   tjl 0x{{[0-9a-f]+}} <__TC32ABSLongThunk_target> @ imm = #0x4
# CHECK-NEXT:    {{[0-9a-f]+}}: 70 07         tjex lr
# CHECK:       {{[0-9a-f]+}} <__TC32ABSLongThunk_target>:
# CHECK-NEXT:    {{[0-9a-f]+}}: 03 64         tpush {r0, r1}
# CHECK-NEXT:    {{[0-9a-f]+}}: 04 08         tloadr r0, [pc, #0x10]
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: 01 30         tstorer r0, [sp, #0x4]
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: 01 6d         tpop {r0, pc}
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: c0 06         nop
# CHECK-NEXT:    {{[0-9a-f]+}}: 00 00 43 00   .word 0x00430000

#--- tc32-thunk.s
  .syntax unified
  .thumb

  .globl _start
  .section .text.start, "ax", %progbits
_start:
  tjl target
  tjex lr

  .section .text.target, "ax", %progbits
  .type target, %function
target:
  tjex lr

#--- tc32-thunk.lds
SECTIONS {
  . = 0x200b4;
  .text.start : { *(.text.start) }
  . = 0x430000;
  .text.target : { *(.text.target) }
}
