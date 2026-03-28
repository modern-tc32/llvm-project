; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s

define i32 @ugt_len_504(i32 %len) {
; CHECK-LABEL: ugt_len_504:
; CHECK: tcmp
; CHECK: tjls
; CHECK-NOT: txor
entry:
  %cmp = icmp ugt i32 %len, 504
  br i1 %cmp, label %gt, label %le

gt:
  ret i32 1

le:
  ret i32 0
}

define i32 @ult_ptr(ptr %dst, ptr %src) {
; CHECK-LABEL: ult_ptr:
; CHECK: tcmp
; CHECK: tjcs
; CHECK-NOT: txor
entry:
  %cmp = icmp ult ptr %dst, %src
  br i1 %cmp, label %lt, label %ge

lt:
  ret i32 1

ge:
  ret i32 0
}
