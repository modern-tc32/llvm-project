; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

target triple = "tc32"

@ga = external global i32
@gb = external global i32

define i32 @f() {
entry:
  %a = load ptr, ptr @ga, align 4
  %b = load ptr, ptr @gb, align 4
  %v1 = load i32, ptr %a, align 4
  %v2 = load i32, ptr %b, align 4
  %s = add i32 %v1, %v2
  ret i32 %s
}

; CHECK-LABEL: f:
; CHECK: tloadr	r0, [pc, #.Ltmp
; CHECK-NEXT: tloadr	r0, [r0]
; CHECK-NEXT: tloadr	r0, [r0]
; CHECK: tloadr	r1, [pc, #.Ltmp
; CHECK-NEXT: tloadr	r1, [r1]
; CHECK-NEXT: tloadr	r1, [r1]
; CHECK-NOT: tj
; CHECK: tmov	pc, lr
; CHECK-NEXT: .p2align	2, 0x0
; CHECK-NEXT: .Ltmp
; CHECK-NEXT: .long
; CHECK-NEXT: .Ltmp
; CHECK-NEXT: .long
