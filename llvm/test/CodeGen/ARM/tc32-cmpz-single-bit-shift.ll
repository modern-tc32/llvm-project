; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

; TC32 should test a single shifted bit through the N flag. In particular,
; bit 0 is shifted left by 31, and vendor GCC branches on MI/PL instead of
; relying on Z from that shift.

target triple = "tc32-unknown-none-elf"

declare void @yes()
declare void @no()

define void @bit0_branch(i32 %x) minsize nounwind optsize "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: bit0_branch:
; CHECK:       tshftl r0, r0, #31
; CHECK-NEXT:  tjmi
; CHECK-NOT:   tjne
; CHECK-NOT:   tjeq
entry:
  %and = and i32 %x, 1
  %is.zero = icmp eq i32 %and, 0
  br i1 %is.zero, label %zero, label %nonzero

zero:
  tail call void @yes()
  ret void

nonzero:
  tail call void @no()
  ret void
}

define void @bit5_branch(i32 %x) minsize nounwind optsize "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: bit5_branch:
; CHECK:       tshftl r0, r0, #26
; CHECK-NEXT:  tjmi
entry:
  %and = and i32 %x, 32
  %is.zero = icmp eq i32 %and, 0
  br i1 %is.zero, label %zero, label %nonzero

zero:
  tail call void @yes()
  ret void

nonzero:
  tail call void @no()
  ret void
}
