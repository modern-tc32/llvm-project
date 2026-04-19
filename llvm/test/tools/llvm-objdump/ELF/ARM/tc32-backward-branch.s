# RUN: llvm-mc -triple=tc32-unknown-none-elf -filetype=obj %s -o %t.o
# RUN: llvm-objdump --triple=tc32 -dr %t.o | FileCheck %s

    .syntax unified
    .thumb
    .globl foo
    .thumb_func
foo:
loop:
    nop
    b loop

# CHECK: {{.*}}tc32-backward-branch{{.*}}:	file format elf32-unknown
# CHECK-EMPTY:
# CHECK: Disassembly of section .text:
# CHECK-EMPTY:
# CHECK-NEXT: 00000000 <loop>:
# CHECK-NEXT:        0: c0 06        nop
# CHECK-NEXT:        2: fd 87         tj	0x0 <loop>              @ imm = #-0x6
