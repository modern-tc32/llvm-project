; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s

declare void @llvm.va_start(ptr)

define i32 @sum_first_vararg(i32 %fixed, ...) {
entry:
  %ap = alloca ptr, align 4
  call void @llvm.va_start(ptr %ap)
  %ap.cur = load ptr, ptr %ap, align 4
  %v = load i32, ptr %ap.cur, align 4
  ret i32 %v
}

; CHECK-LABEL: sum_first_vararg:
; CHECK: tpush	{r0, r1, r2, r3}
; CHECK: tpush	{r7, lr}
; CHECK: tadd
; CHECK: tstorer
; CHECK: tloadr
; CHECK: tpop	{r7}
; CHECK: tpop	{r3}
; CHECK: tadd	sp, #16
; CHECK: tjex	r3
