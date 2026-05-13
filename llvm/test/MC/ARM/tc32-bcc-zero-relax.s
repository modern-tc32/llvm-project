// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-objdump -d %t | FileCheck %s

// CHECK:      00000000 <start>:
// CHECK-NEXT: 0: 00 c1         tjne    0x4 <target>
// CHECK-NEXT: 2: c0 06         nop
// CHECK:      00000004 <target>:
// CHECK-NEXT: 4: 70 07         tjex    lr

  .syntax unified
  .thumb

start:
  tjne target
  nop
target:
  tjex lr
