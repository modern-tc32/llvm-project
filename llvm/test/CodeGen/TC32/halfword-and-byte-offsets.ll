; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

%buf = type { [64 x i8] }

@g = external global %buf, align 1

define i32 @load_half_imm() {
entry:
  %p = getelementptr inbounds %buf, ptr @g, i32 0, i32 0, i32 20
  %v = load i16, ptr %p, align 1
  %z = zext i16 %v to i32
  ret i32 %z
}

define void @store_half_imm(i16 %x) {
entry:
  %p = getelementptr inbounds %buf, ptr @g, i32 0, i32 0, i32 22
  store i16 %x, ptr %p, align 1
  ret void
}

define i32 @load_byte_imm31() {
entry:
  %p = getelementptr inbounds %buf, ptr @g, i32 0, i32 0, i32 31
  %v = load i8, ptr %p, align 1
  %z = zext i8 %v to i32
  ret i32 %z
}

; CHECK-LABEL: load_half_imm:
; CHECK: tloadrh
;
; CHECK-LABEL: store_half_imm:
; CHECK: tstorerh
;
; CHECK-LABEL: load_byte_imm31:
; CHECK: tloadrb
; CHECK-SAME: #31
