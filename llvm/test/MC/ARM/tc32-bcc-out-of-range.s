// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 conditional branch out of range
// CHECK-SAME: value=260
// CHECK-SAME: enc=128

start:
  tjeq far_target
  .space 258
far_target:
  tjex lr
