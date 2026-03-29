; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s --check-prefix=DEFAULT
; RUN: llc -mtriple=tc32 -mattr=+reserve-r7 -O2 < %s | FileCheck %s --check-prefix=RESERVED
; RUN: llc -mtriple=tc32 -mattr=+reserve-r7 -O2 -filetype=obj -o /dev/null %s

target triple = "tc32"

define i32 @r7_pressure(i32 %a, i32 %b, i32 %c, i32 %d,
                        i32 %e, i32 %f, i32 %g, i32 %h) local_unnamed_addr {
entry:
  %x0 = add i32 %a, %b
  %x1 = add i32 %c, %d
  %x2 = add i32 %e, %f
  %x3 = add i32 %g, %h
  %x4 = xor i32 %x0, %x1
  %x5 = xor i32 %x2, %x3
  %x6 = add i32 %x4, %x5
  %x7 = mul i32 %x0, 3
  %x8 = mul i32 %x1, 5
  %x9 = add i32 %x7, %x8
  %x10 = xor i32 %x9, %x6
  ret i32 %x10
}

; DEFAULT-LABEL: r7_pressure:
; DEFAULT: tstorer	r7, [sp, #16]
; DEFAULT: tadd	r7, sp, #12
; DEFAULT: tloadr	r7, [r7]
; DEFAULT: tloadr	r7, [sp, #16]

; RESERVED-LABEL: r7_pressure:
; RESERVED-NOT: tstorer	r7
; RESERVED-NOT: tadd	r7,
; RESERVED-NOT: tloadr	r7
