// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 jump out of range
// CHECK-SAME: value=2052
// CHECK-SAME: enc=1024

start:
  tj far_target
  .space 2050
far_target:
  tjex lr
