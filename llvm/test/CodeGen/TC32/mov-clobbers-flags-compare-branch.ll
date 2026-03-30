; RUN: llc -mtriple=tc32 -O2 -filetype=asm %s -o - | FileCheck %s

; TC32 differs from ARM here: tmov must be treated as flag-clobbering, so a
; compare result may not be consumed by a conditional branch across an
; intervening tmov. The repair pass should force the safe shape below.

define ptr @cmp_then_select_ptr(i16 %id, ptr %base, ptr %fallback, i16 %want) {
; CHECK-LABEL: cmp_then_select_ptr:
; CHECK: tcmp
; CHECK-NEXT: tjeq
; CHECK-NEXT: tj
entry:
  %cmp = icmp eq i16 %id, %want
  %res = select i1 %cmp, ptr %base, ptr %fallback
  ret ptr %res
}
