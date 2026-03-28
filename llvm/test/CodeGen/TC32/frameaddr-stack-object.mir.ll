; RUN: llc -mtriple=tc32 -O2 -stop-after=finalize-isel %s -o - | FileCheck %s

declare i32 @callee(ptr, ptr)

define void @frameaddr_stack_object() {
entry:
  %a = alloca i32, align 4
  %b = alloca [16 x i8], align 1
  store i32 0, ptr %a, align 4
  call i32 @callee(ptr %a, ptr %b)
  ret void
}

; CHECK-LABEL: name: frameaddr_stack_object
; CHECK: %[[A:[0-9]+]]:logr32 = TFRAMEADDR {{[0-9]+}}, 0
; CHECK: TSTORErr{{( killed)?}} %{{[0-9]+}}, %[[A]]
; CHECK: %[[B:[0-9]+]]:logr32 = TFRAMEADDR {{[0-9]+}}, 0
; CHECK: $r0 = COPY %[[A]]
; CHECK: $r1 = COPY %[[B]]
; CHECK-NOT: = TFRAMEADDR %
