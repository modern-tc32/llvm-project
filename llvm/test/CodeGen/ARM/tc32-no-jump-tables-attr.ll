; RUN: llc -mtriple=tc32-unknown-none-elf -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @dense_switch_nojt(i32 %x) #0 {
; CHECK-LABEL: dense_switch_nojt:
; CHECK-NOT:   LJTI
; CHECK-NOT:   tloadrb
; CHECK-NOT:   tloadrh
; CHECK:       tcmp r0, #2
; CHECK:       tcmp r0, #5
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

attributes #0 = { "no-jump-tables"="true" }
