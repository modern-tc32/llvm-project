; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

define i32 @load_align1_i32(ptr %p) {
; CHECK-LABEL: load_align1_i32:
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK-NOT: tloadr
  %q = getelementptr i8, ptr %p, i32 12
  %v = load i32, ptr %q, align 1
  ret i32 %v
}

define void @store_align1_i32(ptr %p, i32 %v) {
; CHECK-LABEL: store_align1_i32:
; CHECK: tstorerb
; CHECK: tstorerb
; CHECK: tstorerb
; CHECK: tstorerb
; CHECK-NOT: tstorer
  %q = getelementptr i8, ptr %p, i32 16
  store i32 %v, ptr %q, align 1
  ret void
}

define ptr @load_align1_ptr(ptr %p) {
; CHECK-LABEL: load_align1_ptr:
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK: tloadrb
; CHECK-NOT: tloadr
  %q = getelementptr i8, ptr %p, i32 4
  %v = load ptr, ptr %q, align 1
  ret ptr %v
}
