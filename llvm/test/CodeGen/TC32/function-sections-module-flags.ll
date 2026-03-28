; RUN: llc -mtriple=tc32 -O1 < %s | FileCheck %s

target triple = "tc32"

define void @foo() {
entry:
  ret void
}

define void @bar() {
entry:
  ret void
}

!llvm.module.flags = !{!0, !1}
!0 = !{i32 1, !"function-sections", i32 1}
!1 = !{i32 1, !"data-sections", i32 1}

; CHECK: .section	.text.foo,"ax",%progbits
; CHECK-LABEL: foo:
; CHECK: .section	.text.bar,"ax",%progbits
; CHECK-LABEL: bar:
