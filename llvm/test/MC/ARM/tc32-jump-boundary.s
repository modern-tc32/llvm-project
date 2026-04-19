// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT:        0: ff 83         tj      0x802 <ok>              @ imm = #0x7fe
// CHECK:      00000802 <ok>:
// CHECK-NEXT:      802: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tj ok
  .space 2048
ok:
  tjex lr
