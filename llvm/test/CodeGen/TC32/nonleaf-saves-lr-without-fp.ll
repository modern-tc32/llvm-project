; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

declare void @callee()

define void @nonleaf_nofp() {
entry:
  call void @callee()
  ret void
}

; CHECK-LABEL: nonleaf_nofp:
; CHECK: tsub	sp, #4
; CHECK: tmov	r7, lr
; CHECK: tstorer	r7, [sp, #0]
; CHECK: tjl	callee
; CHECK: tloadr	r7, [sp, #0]
; CHECK: tmov	lr, r7
; CHECK: tadd	sp, #4
; CHECK-NOT: tpush	{r7, lr}
