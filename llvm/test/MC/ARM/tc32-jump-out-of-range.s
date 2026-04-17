// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: TC32 jump out of range

start:
  tj far_target
  .space 5000
far_target:
  tjex lr
