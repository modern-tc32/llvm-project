; RUN: llc -mtriple=tc32 -O2 -filetype=asm %s -o - | FileCheck %s

%S = type { ptr, i32 }

define i32 @call_cb(ptr %p) {
; CHECK-LABEL: call_cb:
; CHECK: tloadr	r3, [r0]
; CHECK: tjl	__tc32_indirect_call_r3
; CHECK-NOT: __tc32_indirect_call_r12
entry:
  %cbp = getelementptr inbounds %S, ptr %p, i32 0, i32 0
  %cb = load ptr, ptr %cbp, align 4
  %argp = getelementptr inbounds %S, ptr %p, i32 0, i32 1
  %arg = load i32, ptr %argp, align 4
  %res = call i32 %cb(i32 %arg)
  ret i32 %res
}
