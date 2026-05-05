// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 conditional branch out of range
// CHECK-SAME: value=5002
// CHECK-SAME: enc=2499

  .syntax unified
  .thumb

start:
  tjeq target
  .space 5000
target:
  tjex lr
