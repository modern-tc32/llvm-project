; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

define i32 @select_reg_copy(i32 %condv, i32 %truev, i32 %falsev) {
; CHECK-LABEL: select_reg_copy:
; CHECK: tcmp
; CHECK-NEXT: tjeq
; CHECK-NEXT: tj
entry:
  %cmp = icmp eq i32 %condv, 0
  %sel = select i1 %cmp, i32 %truev, i32 %falsev
  %andv = and i32 %sel, 1
  ret i32 %andv
}

define i8 @select_i8_bool(i8 %x) {
; CHECK-LABEL: select_i8_bool:
; CHECK: tcmp
; CHECK-NEXT: tjeq
; CHECK-NEXT: tj
; CHECK-NOT: tadds
entry:
  %cmp = icmp eq i8 %x, 0
  %sel = select i1 %cmp, i8 1, i8 0
  %andv = and i8 %sel, 1
  ret i8 %andv
}

define i8 @toggle_like(i8 %x) {
; CHECK-LABEL: toggle_like:
; CHECK: tcmp
; CHECK-NEXT: tjeq
; CHECK-NEXT: tj
; CHECK-NOT: tadds
entry:
  %is_zero = icmp eq i8 %x, 0
  %next = select i1 %is_zero, i8 1, i8 0
  ret i8 %next
}
