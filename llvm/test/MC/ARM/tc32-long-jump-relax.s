// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 jump out of range
// CHECK-SAME: value=5002
// CHECK-SAME: enc=2499

  .syntax unified
  .thumb

start:
  tj target
  .space 5000
target:
  tjex lr
