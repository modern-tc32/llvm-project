// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 call out of range

start:
  tjl far_target
  .space 10000000
far_target:
  tjex lr
