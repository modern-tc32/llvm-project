// RUN: not %clang_cc1 -triple tc32-unknown-none-elf -Wno-atomic-alignment -emit-obj %s -o /dev/null 2>&1 | FileCheck %s

_Atomic(int) x;

void store_atomic(int v) {
  x = v;
}

// CHECK: error: TC32 does not support atomic operations
