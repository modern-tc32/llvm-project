; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

%struct.ev_timer_ctrl_t = type { ptr, ptr, [676 x i8] }

@ev_timer = global %struct.ev_timer_ctrl_t zeroinitializer, align 4

declare void @ev_unon_timer(ptr)
declare void @ev_timer_nearestUpdate()

define void @ev_timer_executeCB() {
; CHECK-LABEL: ev_timer_executeCB:
; CHECK: tmov	r4, r0
; CHECK-NEXT: tmov	r5, r3
; CHECK-NEXT: tcmp	r0, #0
; CHECK-NEXT: tjne
; CHECK: tmov	r0, r3
; CHECK-NEXT: tcmp	r5, r3
; CHECK-NEXT: tjne
entry:
  %1 = load ptr, ptr @ev_timer, align 4
  %2 = icmp eq ptr %1, null
  br i1 %2, label %33, label %3

3:
  %4 = phi ptr [ %31, %29 ], [ %1, %entry ]
  %5 = phi ptr [ %30, %29 ], [ %1, %entry ]
  %6 = getelementptr inbounds nuw i8, ptr %4, i32 12
  %7 = load i32, ptr %6, align 1
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %9, label %27

9:
  %10 = getelementptr inbounds nuw i8, ptr %4, i32 25
  store i8 1, ptr %10, align 1
  %11 = getelementptr inbounds nuw i8, ptr %4, i32 4
  %12 = load ptr, ptr %11, align 1
  %13 = getelementptr inbounds nuw i8, ptr %4, i32 8
  %14 = load ptr, ptr %13, align 1
  %15 = call i32 %12(ptr %14)
  %16 = icmp slt i32 %15, 0
  br i1 %16, label %17, label %18

17:
  call void @ev_unon_timer(ptr %4)
  br label %24

18:
  %19 = icmp eq i32 %15, 0
  %20 = getelementptr inbounds nuw i8, ptr %4, i32 16
  br i1 %19, label %21, label %23

21:
  %22 = load i32, ptr %20, align 1
  store i32 %22, ptr %6, align 1
  br label %24

23:
  store i32 %15, ptr %20, align 1
  store i32 %15, ptr %6, align 1
  br label %24

24:
  store i8 0, ptr %10, align 1
  %25 = load ptr, ptr @ev_timer, align 4
  %26 = icmp eq ptr %5, %25
  br i1 %26, label %27, label %29

27:
  %28 = load ptr, ptr %4, align 1
  br label %29

29:
  %30 = phi ptr [ %25, %24 ], [ %5, %27 ]
  %31 = phi ptr [ %25, %24 ], [ %28, %27 ]
  %32 = icmp eq ptr %31, null
  br i1 %32, label %33, label %3

33:
  call void @ev_timer_nearestUpdate()
  ret void
}
