; RUN: llc -mtriple=tc32 -O2 -filetype=asm < %s | FileCheck %s

declare i32 @printf(ptr, ...)

@.fmt = private unnamed_addr constant [7 x i8] c"%d %d\0A\00", align 1
@.fmt_f = private unnamed_addr constant [4 x i8] c"%f\0A\00", align 1

define i32 @call_printf(i32 %a, i32 %b) {
; CHECK-LABEL: call_printf:
; CHECK: tloadr
; CHECK: tmov
; CHECK: tjl_r0_r1_r2
entry:
  %fmt = getelementptr inbounds [7 x i8], ptr @.fmt, i32 0, i32 0
  %call = call i32 (ptr, ...) @printf(ptr %fmt, i32 %a, i32 %b)
  ret i32 %call
}

define i32 @call_printf_double(double %x) {
; CHECK-LABEL: call_printf_double:
; CHECK: tloadr	r0
; CHECK: tjl_r0_r1_r2
entry:
  %fmt = getelementptr inbounds [4 x i8], ptr @.fmt_f, i32 0, i32 0
  %call = call i32 (ptr, ...) @printf(ptr %fmt, double %x)
  ret i32 %call
}
