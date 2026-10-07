; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

; PHI elimination places the copy for the %1 incoming value after the compare
; that feeds the switch branch. A low-register copy writes N/Z, so that copy
; used to detour through r12 (two tmov). The copy is now hoisted above the
; compare, leaving a single flag-neutral-before-compare tmov and no r12.

declare void @h(i32)
declare void @k(i32)

define i32 @f(i32 %0, i32 %1) {
; CHECK-LABEL: f:
; CHECK-NOT: r12
; CHECK: tmov r0, r4
; CHECK-NEXT: tcmp r1, #1
  switch i32 %0, label %10 [
    i32 1, label %7
    i32 2, label %3
    i32 3, label %5
  ]
3:
  %4 = add nsw i32 %1, 5
  br label %7
5:
  %6 = add nsw i32 %1, 9
  br label %7
7:
  %8 = phi i32 [ %6, %5 ], [ %4, %3 ], [ %1, %2 ]
  call void @h(i32 %8)
  %9 = add nsw i32 %1, 1
  call void @k(i32 %9)
  br label %10
10:
  %11 = phi i32 [ 0, %2 ], [ 1, %7 ]
  ret i32 %11
}
