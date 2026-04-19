; RUN: llc -mtriple=tc32-unknown-none-elf -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @byte_jumptable(i32 %x) {
; CHECK-LABEL: byte_jumptable:
; CHECK:       tcmp r0, #5
; CHECK-NEXT:  tjhi [[DEFAULT:\.LBB0_[0-9]+]]
; CHECK:       tadd r0, pc
; CHECK-NEXT:  tloadrb r0, [r0, #4]
; CHECK-NEXT:  tshftl r0, r0, #1
; CHECK:       [[ANCHOR:\.LCPI0_0]]:
; CHECK-NEXT:  tadd pc, r0
; CHECK:       [[JTI:\.LJTI0_0]]:
; CHECK-NEXT:  .byte ([[CASE0:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK-NEXT:  .byte ([[CASE1:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK-NEXT:  .byte ([[CASE2:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK-NEXT:  .byte ([[CASE3:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK-NEXT:  .byte ([[CASE4:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK-NEXT:  .byte ([[CASE5:\.LBB0_[0-9]+]]-([[ANCHOR]]+4))/2
; CHECK:       [[DEFAULT]]:
entry:
  switch i32 %x, label %default [
    i32 0, label %c0
    i32 1, label %c1
    i32 2, label %c2
    i32 3, label %c3
    i32 4, label %c4
    i32 5, label %c5
  ]

c0:
  ret i32 11

c1:
  ret i32 22

c2:
  ret i32 33

c3:
  ret i32 44

c4:
  ret i32 55

c5:
  ret i32 66

default:
  ret i32 77
}
