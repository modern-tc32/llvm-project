// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 conditional branch out of range
// CHECK-SAME: value=524290
// CHECK-SAME: enc=262143

start:
  tjeq far_target
  .space 524288
far_target:
  tjex lr
