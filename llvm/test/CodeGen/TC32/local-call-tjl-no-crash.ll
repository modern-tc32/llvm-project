; RUN: llc -mtriple=tc32 -O0 -filetype=asm -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O0 -filetype=obj -o /dev/null %s

define internal void @callee() {
entry:
  ret void
}

define void @caller() {
entry:
  call void @callee()
  ret void
}

; CHECK-LABEL: caller:
; CHECK: tjl	callee
