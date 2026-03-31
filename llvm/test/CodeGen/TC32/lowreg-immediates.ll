; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s

define i32 @cmp_imm_97(i32 %x) {
; CHECK-LABEL: cmp_imm_97:
; CHECK: tcmp	r0, #97
entry:
  %c = icmp eq i32 %x, 97
  br i1 %c, label %ok, label %bad

ok:
  ret i32 0

bad:
  ret i32 1
}

define i32 @sub_imm_3(i32 %x) {
; CHECK-LABEL: sub_imm_3:
; CHECK: tsub	r0, r0, #3
entry:
  %y = sub i32 %x, 3
  ret i32 %y
}

define i32 @sub_imm_200(i32 %x) {
; CHECK-LABEL: sub_imm_200:
; CHECK: tsub	r0, r0, #200
entry:
  %y = sub i32 %x, 200
  ret i32 %y
}
