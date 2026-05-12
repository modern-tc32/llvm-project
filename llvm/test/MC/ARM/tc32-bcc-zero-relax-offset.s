// RUN: not llvm-mc -triple=tc32 -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

// CHECK: error: unsupported TC32 zero-displacement conditional branch

  .syntax unified
  .thumb

start:
  tcmp r4, r1
  tjne target
  tmov r4, #0
target:
  tjex lr
