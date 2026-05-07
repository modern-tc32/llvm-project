; RUN: llc -mtriple=tc32-unknown-none-elf -O2 %s -o - | FileCheck %s

define i32 @inc_ptr(i32 %ptr) {
; CHECK-LABEL: inc_ptr:
; CHECK-NOT: tadd r0, #0x1
; CHECK-NOT: tadd r0, r0, #1
; CHECK: tmov [[ONE:r[0-7]]], #1
; CHECK: tadd r0, r0, [[ONE]]
  %next = add i32 %ptr, 1
  ret i32 %next
}

define i32 @dec_ptr(i32 %ptr) {
; CHECK-LABEL: dec_ptr:
; CHECK-NOT: tsub r0, #0x1
; CHECK-NOT: tsub r0, r0, #1
; CHECK: tmov [[ONE:r[0-7]]], #1
; CHECK: tsub r0, r0, [[ONE]]
  %next = add i32 %ptr, -1
  ret i32 %next
}
