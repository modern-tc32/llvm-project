; RUN: llc -mtriple=tc32-unknown-none-elf -O2 %s -o - | FileCheck %s

@G = global i32 0

define void @tc32_switch(i32 %x) {
entry:
  switch i32 %x, label %def [
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
  store volatile i32 10, ptr @G
  ret void

c1:
  store volatile i32 11, ptr @G
  ret void

c2:
  store volatile i32 12, ptr @G
  ret void

c3:
  store volatile i32 13, ptr @G
  ret void

c4:
  store volatile i32 14, ptr @G
  ret void

c5:
  store volatile i32 15, ptr @G
  ret void

c6:
  store volatile i32 16, ptr @G
  ret void

c7:
  store volatile i32 17, ptr @G
  ret void

def:
  store volatile i32 99, ptr @G
  ret void
}

; CHECK-LABEL: tc32_switch:
; CHECK: tcmp [[IDX:r[0-7]]], #7
; CHECK: tshftl [[IDX]], [[IDX]], #2
; CHECK-NEXT: tadd [[IDX]], pc
; CHECK-NEXT: tloadr [[IDX]], [[[IDX]], #4]
; CHECK-NEXT: tmov pc, [[IDX]]
; CHECK-NEXT: .align 2
; CHECK-NEXT: .LJTI0_0:
; CHECK-NEXT: .long .LBB0_3+1
; CHECK-NEXT: .long .LBB0_8+1
; CHECK-NEXT: .long .LBB0_5+1
; CHECK-NEXT: .long .LBB0_6+1
; CHECK-NEXT: .long .LBB0_4+1
; CHECK-NEXT: .long .LBB0_9+1
; CHECK-NEXT: .long .LBB0_10+1
; CHECK-NEXT: .long .LBB0_7+1
