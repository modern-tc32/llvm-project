; RUN: llc -mtriple=tc32-unknown-none-elf -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @sparse_switch(i32 %x) {
; CHECK-LABEL: sparse_switch:
; CHECK-NOT:   LJTI
; CHECK-NOT:   tloadrb
; CHECK-NOT:   tloadrh
; CHECK:       tcmp r0, #32
; CHECK:       tcmp r0, #17
; CHECK:       tcmp r0, #65
entry:
  switch i32 %x, label %default [
    i32 0, label %c0
    i32 17, label %c1
    i32 33, label %c2
    i32 65, label %c3
  ]

c0:
  ret i32 1

c1:
  ret i32 2

c2:
  ret i32 3

c3:
  ret i32 4

default:
  ret i32 5
}
