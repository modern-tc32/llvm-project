; RUN: llc -mtriple=tc32 -O2 -filetype=asm -o /dev/null %s
; RUN: llc -mtriple=tc32 -O2 -stop-after=greedy -o - %s | FileCheck %s --check-prefix=MIR

@buf = external global [4 x i8], align 1

define i32 @pack_bytes() {
entry:
  %p0 = getelementptr inbounds [4 x i8], ptr @buf, i32 0, i32 0
  %b0 = load i8, ptr %p0, align 1
  %p1 = getelementptr inbounds [4 x i8], ptr @buf, i32 0, i32 1
  %b1 = load i8, ptr %p1, align 1
  %p2 = getelementptr inbounds [4 x i8], ptr @buf, i32 0, i32 2
  %b2 = load i8, ptr %p2, align 1
  %p3 = getelementptr inbounds [4 x i8], ptr @buf, i32 0, i32 3
  %b3 = load i8, ptr %p3, align 1
  %w0 = zext i8 %b0 to i32
  %w1 = zext i8 %b1 to i32
  %w2 = zext i8 %b2 to i32
  %w3 = zext i8 %b3 to i32
  %s1 = shl i32 %w1, 8
  %s2 = shl i32 %w2, 16
  %s3 = shl i32 %w3, 24
  %o1 = or i32 %w0, %s1
  %o2 = or i32 %o1, %s2
  %o3 = or i32 %o2, %s3
  ret i32 %o3
}

; MIR-LABEL: name:            pack_bytes
; MIR-NOT: TORrr undef
