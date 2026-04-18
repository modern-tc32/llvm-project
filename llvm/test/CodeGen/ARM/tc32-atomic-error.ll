; RUN: not llc -mtriple=tc32-unknown-none-elf %s -o /dev/null 2>&1 | FileCheck %s

target triple = "tc32-unknown-none-elf"

define { i32, i1 } @cx(ptr %p, i32 %expected, i32 %desired) {
; CHECK: error: TC32 does not support atomic operations
entry:
  %r = cmpxchg ptr %p, i32 %expected, i32 %desired monotonic monotonic
  ret { i32, i1 } %r
}

declare void @__atomic_store(i32, ptr, ptr, i32)

define void @store_via_libcall(ptr %dst, ptr %src) {
entry:
  call void @__atomic_store(i32 4, ptr %dst, ptr %src, i32 5)
  ret void
}
