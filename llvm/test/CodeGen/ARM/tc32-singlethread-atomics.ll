; RUN: llc -mtriple=tc32-unknown-none-elf -thread-model=single %s -o - | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @load_relaxed(ptr %p) {
; CHECK-LABEL: load_relaxed:
; CHECK: tloadr r0, [r0]
entry:
  %v = load atomic i32, ptr %p syncscope("singlethread") monotonic, align 4
  ret i32 %v
}

define void @store_relaxed(ptr %p, i32 %v) {
; CHECK-LABEL: store_relaxed:
; CHECK: tstorer r1, [r0]
entry:
  store atomic i32 %v, ptr %p syncscope("singlethread") monotonic, align 4
  ret void
}
