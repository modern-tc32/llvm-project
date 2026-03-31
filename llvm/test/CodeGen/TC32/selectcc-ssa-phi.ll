; RUN: llc -mtriple=tc32 -O2 -verify-machineinstrs -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

define i32 @select_zero_or_five(i32 %x) {
; CHECK-LABEL: select_zero_or_five:
; CHECK: tcmp
; CHECK: tjne
; CHECK: tmov	r0, r2
entry:
  %cmp = icmp eq i32 %x, 5
  %sel = select i1 %cmp, i32 0, i32 5
  ret i32 %sel
}
