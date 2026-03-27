; RUN: llc -mtriple=tc32 -O0 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O0 -filetype=obj -o /dev/null %s

; This used to crash during MC emission because PEI folded a frame-index store
; to tstorer [fp,#imm] while leaving a high source register (for example r8).

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

%struct.color_t = type { i8, i8, i8 }

@g_color = dso_local global %struct.color_t zeroinitializer, align 1

; CHECK: hsvToRGB:
; CHECK: tmov	r6, r8
; CHECK-NOT: tstorer	r8, [r7

declare void @llvm.fake.use(...)

define internal fastcc void @hsvToRGB(i8 noundef zeroext %0, i8 noundef zeroext %1, i8 noundef zeroext %2) unnamed_addr #0 {
  %4 = zext i8 %1 to i32
  %5 = icmp eq i8 %1, 0
  br i1 %5, label %36, label %6

6:
  %7 = zext i8 %0 to i32
  %8 = zext i8 %2 to i32
  %9 = udiv i8 %0, 43
  %10 = zext nneg i8 %9 to i32
  %11 = mul nsw i32 %10, -43
  %12 = add nsw i32 %11, %7
  %13 = mul nsw i32 %12, 6
  %14 = xor i32 %4, 255
  %15 = mul nuw nsw i32 %14, %8
  %16 = lshr i32 %15, 8
  %17 = trunc nuw i32 %16 to i8
  %18 = mul nsw i32 %13, %4
  %19 = lshr i32 %18, 8
  %20 = sub nsw i32 255, %19
  %21 = mul i32 %20, %8
  %22 = lshr i32 %21, 8
  %23 = trunc i32 %22 to i8
  %24 = sub nsw i32 255, %13
  %25 = mul nsw i32 %24, %4
  %26 = lshr i32 %25, 8
  %27 = sub nsw i32 255, %26
  %28 = mul i32 %27, %8
  %29 = lshr i32 %28, 8
  %30 = trunc i32 %29 to i8
  switch i8 %9, label %35 [
    i8 0, label %36
    i8 1, label %31
    i8 2, label %32
    i8 3, label %33
    i8 4, label %34
  ]

31:
  br label %36

32:
  br label %36

33:
  br label %36

34:
  br label %36

35:
  br label %36

36:
  %37 = phi i8 [ %2, %3 ], [ %23, %31 ], [ %17, %32 ], [ %17, %33 ], [ %30, %34 ], [ %2, %35 ], [ %2, %6 ]
  %38 = phi i8 [ %2, %3 ], [ %2, %31 ], [ %2, %32 ], [ %23, %33 ], [ %17, %34 ], [ %17, %35 ], [ %30, %6 ]
  %39 = phi i8 [ %2, %3 ], [ %17, %31 ], [ %30, %32 ], [ %2, %33 ], [ %2, %34 ], [ %23, %35 ], [ %17, %6 ]
  %40 = phi i8 [ undef, %3 ], [ %9, %31 ], [ %9, %32 ], [ %9, %33 ], [ %9, %34 ], [ %9, %35 ], [ %9, %6 ]
  %41 = phi i8 [ undef, %3 ], [ %17, %31 ], [ %17, %32 ], [ %17, %33 ], [ %17, %34 ], [ %17, %35 ], [ %17, %6 ]
  %42 = phi i8 [ undef, %3 ], [ %23, %31 ], [ %23, %32 ], [ %23, %33 ], [ %23, %34 ], [ %23, %35 ], [ %23, %6 ]
  %43 = phi i8 [ undef, %3 ], [ %30, %31 ], [ %30, %32 ], [ %30, %33 ], [ %30, %34 ], [ %30, %35 ], [ %30, %6 ]
  %44 = phi i32 [ undef, %3 ], [ %7, %31 ], [ %7, %32 ], [ %7, %33 ], [ %7, %34 ], [ %7, %35 ], [ %7, %6 ]
  %45 = phi i32 [ undef, %3 ], [ %8, %31 ], [ %8, %32 ], [ %8, %33 ], [ %8, %34 ], [ %8, %35 ], [ %8, %6 ]
  %46 = phi i32 [ undef, %3 ], [ %13, %31 ], [ %13, %32 ], [ %13, %33 ], [ %13, %34 ], [ %13, %35 ], [ %13, %6 ]
  store i8 %37, ptr @g_color, align 1
  store i8 %38, ptr getelementptr inbounds nuw (i8, ptr @g_color, i32 1), align 1
  store i8 %39, ptr getelementptr inbounds nuw (i8, ptr @g_color, i32 2), align 1
  call void (...) @llvm.fake.use(i32 %46)
  call void (...) @llvm.fake.use(i32 %45)
  call void (...) @llvm.fake.use(i32 %4)
  call void (...) @llvm.fake.use(i32 %44)
  call void (...) @llvm.fake.use(i8 %43)
  call void (...) @llvm.fake.use(i8 %42)
  call void (...) @llvm.fake.use(i8 %41)
  call void (...) @llvm.fake.use(i8 %40)
  call void (...) @llvm.fake.use(ptr nonnull getelementptr inbounds nuw (i8, ptr @g_color, i32 2))
  call void (...) @llvm.fake.use(ptr nonnull getelementptr inbounds nuw (i8, ptr @g_color, i32 1))
  call void (...) @llvm.fake.use(ptr nonnull @g_color)
  call void (...) @llvm.fake.use(i8 %2)
  call void (...) @llvm.fake.use(i8 %1)
  call void (...) @llvm.fake.use(i8 %0)
  ret void
}

attributes #0 = { noinline nounwind "frame-pointer"="all" }
