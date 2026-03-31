; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s

@x = dso_local global double 1.000000e+02, align 8

define dso_local i32 @main() {
entry:
  %0 = load double, ptr @x, align 8
  %cmp = fcmp olt double %0, 1.000000e+00
  %res = zext i1 %cmp to i32
  ret i32 %res
}

; CHECK-LABEL: main:
; CHECK-NOT: tstorer r0, [sp
; CHECK-NOT: tstorer r1, [sp
; CHECK-NOT: tstorer r2, [sp
; CHECK-NOT: tstorer r3, [sp
; CHECK: tloadr r0, [pc
; CHECK: tloadr r0, [r0]
; CHECK: tloadr r1, [pc
; CHECK: tloadr r1, [r1]
; CHECK: tmovs r2, #63
; CHECK: tshftl r3, r2, #8
; CHECK: tmovs r4, #0
; CHECK: tmov r2, r4
; CHECK: tjl __ltdf2
