; RUN: llc -mtriple=tc32 -O2 -filetype=asm -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

@g0 = external global i8, align 1
@g1 = external global i8, align 1
@g2 = external global i8, align 1
@g3 = external global i8, align 1

declare void @callee7(ptr, ptr, ptr, ptr, i16, i16, i8)
declare void @callee1(i8)

define void @caller7(ptr %p) {
; CHECK-LABEL: caller7:
; CHECK: tjl	callee7
; CHECK: tjl	callee1
entry:
  %a0 = getelementptr inbounds i8, ptr @g0, i32 16
  %a1 = getelementptr inbounds i8, ptr @g1, i32 10
  %a2 = getelementptr inbounds i8, ptr @g2, i32 6
  %a3 = getelementptr inbounds i8, ptr @g3, i32 38
  call void @callee7(ptr %a0, ptr %a1, ptr %a2, ptr %a3, i16 0, i16 -1, i8 1)
  call void @callee1(i8 2)
  ret void
}
