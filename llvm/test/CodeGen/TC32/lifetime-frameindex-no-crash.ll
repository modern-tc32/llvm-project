; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture)
declare void @sink(ptr, i32)

define void @repro(ptr %dst, i32 %v) {
entry:
  %buf = alloca [12 x i8], align 1
  %slot = alloca i32, align 4
  %p = getelementptr inbounds [12 x i8], ptr %buf, i32 0, i32 3
  call void @llvm.lifetime.start.p0(i64 12, ptr %buf)
  call void @llvm.lifetime.start.p0(i64 4, ptr %slot)
  store i32 %v, ptr %slot, align 4
  %x = load i32, ptr %slot, align 4
  store i8 42, ptr %p, align 1
  store ptr %p, ptr %dst, align 1
  call void @sink(ptr %p, i32 %x)
  call void @llvm.lifetime.end.p0(i64 4, ptr %slot)
  call void @llvm.lifetime.end.p0(i64 12, ptr %buf)
  ret void
}
