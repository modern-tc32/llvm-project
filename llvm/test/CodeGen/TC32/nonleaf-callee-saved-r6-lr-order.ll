; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

declare void @callee()

define i32 @nonleaf_preserves_r6(i32 %a, i32 %b, i32 %c, i32 %d) {
entry:
  %s0 = add i32 %a, %b
  %s1 = add i32 %c, %d
  %s2 = add i32 %s0, %s1
  call void @callee()
  %r = add i32 %s2, %a
  ret i32 %r
}

; CHECK-LABEL: nonleaf_preserves_r6:
; CHECK: tsub	sp, #
; CHECK: tmov	r7, r14
; CHECK-NEXT: tstorer	r7,
; CHECK: tstorer	r4,
; CHECK: tstorer	r5,
