// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 call out of range
// CHECK-SAME: value=4194308
// CHECK-SAME: enc=2097152

start:
  tjl far_target
  .space 4194304
far_target:
  tjex lr
