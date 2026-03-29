; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target triple = "tc32"

define i32 @not_const(i32 %x) local_unnamed_addr {
entry:
  %a = xor i32 %x, -1
  %b = add i32 %a, 255
  ret i32 %b
}

define ptr @minus_four(ptr %p) local_unnamed_addr {
entry:
  %q = getelementptr inbounds i8, ptr %p, i32 -4
  ret ptr %q
}

define i32 @align4(i32 %x) local_unnamed_addr {
entry:
  %a = add i32 %x, 3
  %b = and i32 %a, -4
  ret i32 %b
}

; CHECK-LABEL: not_const:
; CHECK: tmovs	r1, #255
; CHECK: tmovn	r0, r0
; CHECK: tadds	r0, r0, r1
; CHECK-LABEL: minus_four:
; CHECK: tmovs	r1, #3
; CHECK: tmovn	r1, r1
; CHECK: tadds	r0, r0, r1
; CHECK-LABEL: align4:
; CHECK: tmovs	r1, #3
; CHECK: tadds	r0, r0, r1
; CHECK: tmovn	r1, r1
; CHECK: tand	r0, r1
