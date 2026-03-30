; RUN: llc -mtriple=tc32 -O2 -filetype=asm %s -o - | FileCheck %s

define i32 @usubsat_i32(i32 %a, i32 %b) {
; CHECK-LABEL: usubsat_i32:
; CHECK: tcmp
; CHECK: tjhi
; CHECK: tmovs
; CHECK: tsub
; CHECK-NOT: llvm.usub.sat
  %r = call i32 @llvm.usub.sat.i32(i32 %a, i32 %b)
  ret i32 %r
}

declare i32 @llvm.usub.sat.i32(i32, i32)
