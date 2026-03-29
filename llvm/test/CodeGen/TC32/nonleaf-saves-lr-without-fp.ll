; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

declare void @callee()

define void @nonleaf_nofp() {
entry:
  call void @callee()
  ret void
}

; CHECK-LABEL: nonleaf_nofp:
; CHECK: tsub	sp, #8
; CHECK: tmov	r7, r14
; CHECK: tstorer	r7, [sp, #4]
; CHECK: tjl	callee
; CHECK: tloadr	r7, [sp, #4]
; CHECK: tmov	r14, r7
; CHECK: tadd	sp, #8
; CHECK-NOT: tpush	{r7, lr}
