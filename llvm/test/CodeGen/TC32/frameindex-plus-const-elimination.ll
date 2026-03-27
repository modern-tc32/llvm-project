; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

declare void @llvm.fake.use(...)

define i32 @frameindex_plus_const(i32 %x) #0 {
entry:
  %buf = alloca [16 x i32], align 4
  %p = getelementptr inbounds [16 x i32], ptr %buf, i32 0, i32 13
  store volatile i32 %x, ptr %p, align 4
  %v = load volatile i32, ptr %p, align 4
  call void (...) @llvm.fake.use(i32 %v)
  ret i32 %v
}

attributes #0 = { noinline nounwind "frame-pointer"="all" }
