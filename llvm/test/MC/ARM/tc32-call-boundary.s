// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump --triple=tc32 -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: ff 93 ff 9f   tjl     0x400002 <ok>           @ imm = #0x3ffffe
// CHECK:      00400002 <ok>:
// CHECK-NEXT:   400002: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tjl ok
  .space 4194302
ok:
  tjex lr
