; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target triple = "tc32"

define i32 @fp_attr_kept(i32 %x) #0 {
entry:
  %y = add i32 %x, 7
  ret i32 %y
}

attributes #0 = { noinline nounwind "frame-pointer"="all" }

; CHECK-LABEL: fp_attr_kept:
; CHECK: tpush	{r7, lr}
; CHECK: tadd	r7, sp, #0
; CHECK: tpop	{r7, pc}
