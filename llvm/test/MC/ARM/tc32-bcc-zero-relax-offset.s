// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o - | llvm-objdump -d - | FileCheck %s

  .syntax unified
  .thumb

start:
  tcmp r4, r1
  tjne target
  .short 0x46c0
target:
  tjex lr

// CHECK:      0: 8c 02  tcmp r4, r1
// CHECK:      2: 00 c1  tjne 0x6 <target>
