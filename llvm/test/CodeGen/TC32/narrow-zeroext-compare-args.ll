; RUN: llc -mtriple=tc32-unknown-unknown-elf -O2 -filetype=asm %s -o - | FileCheck %s

@g16 = external dso_local global i16, align 2

define dso_local zeroext i8 @cmp_i16_arg(i8 noundef zeroext %ep, i16 noundef zeroext %id) {
; CHECK-LABEL: cmp_i16_arg:
; CHECK: tshftl [[ARG:r[0-7]]], r1, #16
; CHECK-NEXT: tshftr [[ARG]], [[ARG]], #16
; CHECK: tcmp {{r[0-7]}}, [[ARG]]
  %v = load i16, ptr @g16, align 2
  %cmp = icmp eq i16 %v, %id
  %ret = zext i1 %cmp to i8
  ret i8 %ret
}

define dso_local zeroext i8 @cmp_i16_thirdarg(i8 noundef zeroext %ep,
                                              i16 noundef zeroext %cluster,
                                              i16 noundef zeroext %attr) {
; CHECK-LABEL: cmp_i16_thirdarg:
; CHECK: tshftl [[ARG3:r[0-7]]], r2, #16
; CHECK-NEXT: tshftr [[ARG3]], [[ARG3]], #16
; CHECK: tcmp {{r[0-7]}}, [[ARG3]]
  %v = load i16, ptr @g16, align 2
  %cmp = icmp eq i16 %v, %attr
  %ret = zext i1 %cmp to i8
  ret i8 %ret
}
