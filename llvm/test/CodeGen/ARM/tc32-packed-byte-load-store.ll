; RUN: llc -mtriple=tc32-unknown-none-elf %s -o - | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i8 @load_i32_lo(ptr %p) {
; CHECK-LABEL: load_i32_lo:
; CHECK-NOT: tloadr
; CHECK: tloadrb r0, [r0]
entry:
  %v = load i32, ptr %p, align 1
  %t = trunc i32 %v to i8
  ret i8 %t
}

define i8 @load_i32_hi16(ptr %p) {
; CHECK-LABEL: load_i32_hi16:
; CHECK-NOT: tloadr
; CHECK: tloadrb r0, [r0, #2]
entry:
  %v = load i32, ptr %p, align 1
  %s = lshr i32 %v, 16
  %t = trunc i32 %s to i8
  ret i8 %t
}

define i8 @load_i16_hi(ptr %p) {
; CHECK-LABEL: load_i16_hi:
; CHECK-NOT: tloadr
; CHECK: tloadrb r0, [r0, #1]
entry:
  %v = load i16, ptr %p, align 1
  %s = lshr i16 %v, 8
  %t = trunc i16 %s to i8
  ret i8 %t
}
