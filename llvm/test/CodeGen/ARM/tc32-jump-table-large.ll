; RUN: llc -mtriple=tc32-unknown-none-elf -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

declare i32 @llvm.arm.space(i32, i32)

define i32 @large_jumptable(i32 %x) {
; CHECK-LABEL: large_jumptable:
; CHECK:       tcmp r0, #7
; CHECK-NEXT:  tjls [[TABLE:\.LBB0_[0-9]+]]
; CHECK-NEXT:  tj [[DEFAULT:\.LBB0_[0-9]+]]
; CHECK:       [[TABLE]]:
; CHECK-NEXT:  tshftl r0, r0, #2
; CHECK-NEXT:  tadd r0, pc
; CHECK-NEXT:  tloadr r0, [r0, #4]
; CHECK-NEXT:  tmov pc, r0
; CHECK:       [[JTI:\.LJTI0_0]]:
; CHECK-NEXT:  .long [[CASE0:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE1:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE2:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE3:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE4:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE5:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE6:\.LBB0_[0-9]+]]+1
; CHECK-NEXT:  .long [[CASE7:\.LBB0_[0-9]+]]+1
; CHECK-NOT:   .byte
; CHECK-NOT:   .short
; CHECK:       [[DEFAULT]]:
entry:
  switch i32 %x, label %default [
    i32 0, label %c0
    i32 1, label %c1
    i32 2, label %c2
    i32 3, label %c3
    i32 4, label %c4
    i32 5, label %c5
    i32 6, label %c6
    i32 7, label %c7
  ]

c0:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 10

c1:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 11

c2:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 12

c3:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 13

c4:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 14

c5:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 15

c6:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 16

c7:
  call i32 @llvm.arm.space(i32 1500, i32 undef)
  ret i32 17

default:
  ret i32 20
}
