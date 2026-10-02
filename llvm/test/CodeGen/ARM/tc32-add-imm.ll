; RUN: llc -mtriple=tc32-unknown-none-elf -O2 %s -o - | FileCheck %s

; Hardware-validated (tests/hw_imm_carry): the short tadd/tsub immediate forms
; carry across bytes, and vendor GCC emits them for pointer arithmetic.

define i32 @inc_ptr(i32 %ptr) {
; CHECK-LABEL: inc_ptr:
; CHECK-NOT: tmov
; CHECK: tadd r0, r0, #1
  %next = add i32 %ptr, 1
  ret i32 %next
}

define i32 @dec_ptr(i32 %ptr) {
; CHECK-LABEL: dec_ptr:
; CHECK-NOT: tmov
; CHECK: tsub r0, r0, #1
  %next = add i32 %ptr, -1
  ret i32 %next
}
