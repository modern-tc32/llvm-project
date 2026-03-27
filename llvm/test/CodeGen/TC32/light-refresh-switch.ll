; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

%struct.app = type { [18 x i8], i8 }

@gLightCtx = external global %struct.app, align 1

declare void @light_onOffUpdate()
declare void @light_colorUpdate()

; CHECK-LABEL: light_refresh:
; CHECK: tcmp
; CHECK: tjle
; CHECK: tjl	light_colorUpdate
; CHECK: tcmp
; CHECK: tjne
; CHECK: tjl	light_onOffUpdate
; CHECK-NOT: tneg

define void @light_refresh(i8 noundef zeroext %0) {
  %2 = zext i8 %0 to i32
  switch i32 %2, label %7 [
    i32 0, label %5
    i32 1, label %6
    i32 2, label %6
  ]

5:
  call void @light_onOffUpdate()
  br label %8

6:
  call void @light_colorUpdate()
  br label %8

7:
  br label %9

8:
  store i8 1, ptr getelementptr inbounds (%struct.app, ptr @gLightCtx, i32 0, i32 1), align 1
  br label %9

9:
  ret void
}
