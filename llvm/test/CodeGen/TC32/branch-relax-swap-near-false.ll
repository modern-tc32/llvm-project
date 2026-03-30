; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

declare void @sink(i32)

define void @f(i32 %x) {
; CHECK-LABEL: f:
; CHECK: tcmp
; CHECK-NEXT: tjne [[NEAR:\.LBB[0-9_]+]]
; CHECK-NEXT: tj [[FAR:\.LBB[0-9_]+]]
; CHECK: [[NEAR]]:
; CHECK: tmovs r0, #1
; CHECK: [[FAR]]:
entry:
  %c = icmp eq i32 %x, 0
  br i1 %c, label %far, label %near

near:
  call void @sink(i32 1)
  br label %exit

far:
  call void @sink(i32 2)
  call void @sink(i32 3)
  call void @sink(i32 4)
  call void @sink(i32 5)
  call void @sink(i32 6)
  call void @sink(i32 7)
  call void @sink(i32 8)
  call void @sink(i32 9)
  call void @sink(i32 10)
  call void @sink(i32 11)
  call void @sink(i32 12)
  call void @sink(i32 13)
  call void @sink(i32 14)
  call void @sink(i32 15)
  call void @sink(i32 16)
  call void @sink(i32 17)
  call void @sink(i32 18)
  call void @sink(i32 19)
  call void @sink(i32 20)
  call void @sink(i32 21)
  call void @sink(i32 22)
  call void @sink(i32 23)
  call void @sink(i32 24)
  call void @sink(i32 25)
  call void @sink(i32 26)
  call void @sink(i32 27)
  call void @sink(i32 28)
  call void @sink(i32 29)
  call void @sink(i32 30)
  call void @sink(i32 31)
  call void @sink(i32 32)
  call void @sink(i32 33)
  call void @sink(i32 34)
  call void @sink(i32 35)
  call void @sink(i32 36)
  call void @sink(i32 37)
  call void @sink(i32 38)
  call void @sink(i32 39)
  call void @sink(i32 40)
  br label %exit

exit:
  ret void
}
