; RUN: llc -mtriple=tc32 -stop-after=prologepilog -o - %s | FileCheck %s

; Check that materialized addresses for allocas survive until PEI and land in
; distinct stack slots instead of collapsing to the frame pointer base.

define void @store_args(ptr %a, ptr %b, ptr %c, ptr %d, i8 %e, i8 %f, i8 %g) #0 {
entry:
  %ap = alloca ptr, align 4
  %bp = alloca ptr, align 4
  %cp = alloca ptr, align 4
  %dp = alloca ptr, align 4
  %ep = alloca i8, align 1
  %fp = alloca i8, align 1
  %gp = alloca i8, align 1
  store ptr %a, ptr %ap, align 4
  store ptr %b, ptr %bp, align 4
  store ptr %c, ptr %cp, align 4
  store ptr %d, ptr %dp, align 4
  store i8 %e, ptr %ep, align 1
  store i8 %f, ptr %fp, align 1
  store i8 %g, ptr %gp, align 1
  ret void
}

attributes #0 = { noinline nounwind optnone }

; CHECK-LABEL: name:            store_args
; CHECK: TSTOREfpu8 killed $r2, 24
; CHECK: TSTOREfpu8 killed $r3, 20
; CHECK: TSTOREfpu8 killed $r0, 16
; CHECK: TSTOREfpu8 killed $r0, 12
; CHECK: TADDSri8 $r6, 11
; CHECK: TSTOREBrr killed $r5, $r6
; CHECK: TADDSri8 $r6, 10
; CHECK: TSTOREBrr killed $r1, $r6
; CHECK: TADDSri8 $r6, 9
; CHECK: TSTOREBrr killed $r4, $r6
