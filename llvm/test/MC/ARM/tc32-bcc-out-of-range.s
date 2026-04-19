// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 long conditional branch out of range
// CHECK-SAME: value=524292
// CHECK-SAME: enc=262144

start:
  tjeq far_target
  .space 524288
far_target:
  tjex lr
