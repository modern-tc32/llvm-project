; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

; TC32 has a one-instruction negate (tnegs, encoding 0x0240 | rn<<3 | rd).
; It is used for ineg instead of the four-instruction 0 - x expansion.

; CHECK-LABEL: neg32:
; CHECK:       tnegs r0, r0
; CHECK-NOT:   tmovn
; CHECK-NOT:   taddc
define i32 @neg32(i32 %x) {
  %r = sub i32 0, %x
  ret i32 %r
}

declare void @use(i32)

; CHECK-LABEL: neg32_call:
; CHECK:       tnegs r0, r0
; CHECK:       tjl use
define void @neg32_call(i32 %x) {
  %n = sub i32 0, %x
  call void @use(i32 %n)
  ret void
}
