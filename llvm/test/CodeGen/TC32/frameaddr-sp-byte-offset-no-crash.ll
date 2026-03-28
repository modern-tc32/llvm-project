; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s
; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

define void @byte_offset_from_sp(ptr %out) {
entry:
  %buf = alloca [5 x i8], align 1
  %p = getelementptr inbounds [5 x i8], ptr %buf, i32 0, i32 1
  store ptr %p, ptr %out, align 1
  ret void
}

; CHECK-LABEL: byte_offset_from_sp:
; CHECK: tmov
; CHECK: tadds
