// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 conditional branch out of range

start:
  tjeq far_target
  .space 2000
far_target:
  tjex lr
