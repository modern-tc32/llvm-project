; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

@glob = internal global [4 x i32] [i32 1, i32 2, i32 3, i32 4], align 4

define i32 @aligned_reader(i32 %idx) {
entry:
  %inrange = icmp ult i32 %idx, 4
  br i1 %inrange, label %load, label %out

load:
  %p = getelementptr inbounds [4 x i32], ptr @glob, i32 0, i32 %idx
  %v = load i32, ptr %p, align 4
  br label %out

out:
  %r = phi i32 [ %v, %load ], [ 0, %entry ]
  ret i32 %r
}

define i32 @aligned_callee(i32 %x) {
entry:
  %a = tail call i32 @aligned_reader(i32 %x)
  %b = add i32 %a, 7
  ret i32 %b
}
