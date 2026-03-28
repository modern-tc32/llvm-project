; RUN: llc -mtriple=tc32 -O2 < %s | FileCheck %s

declare i32 @is_ev_buf(ptr)
declare void @sys_exceptionPost(i32, i32)

define ptr @memset_like(ptr %dest, i32 %val, i32 %len) {
entry:
  %exc = alloca i32, align 4
  store i32 121, ptr %exc, align 4
  %isnull = icmp eq ptr %dest, null
  br i1 %isnull, label %trap, label %check

check:
  %ev = call i32 @is_ev_buf(ptr %dest)
  store i32 125, ptr %exc, align 4
  %big = icmp ugt i32 %len, 504
  %evnz = icmp ne i32 %ev, 0
  %bad = and i1 %big, %evnz
  br i1 %bad, label %trap, label %loop.preheader

loop.preheader:
  %cmp0 = icmp eq i32 %len, 0
  br i1 %cmp0, label %ret, label %loop

loop:
  %ptr = phi ptr [ %dest, %loop.preheader ], [ %next, %loop ]
  %n = phi i32 [ %len, %loop.preheader ], [ %n1, %loop ]
  %b = trunc i32 %val to i8
  store i8 %b, ptr %ptr, align 1
  %next = getelementptr inbounds i8, ptr %ptr, i32 1
  %n1 = add i32 %n, -1
  %done = icmp eq i32 %n1, 0
  br i1 %done, label %ret, label %loop

trap:
  %code = load i32, ptr %exc, align 4
  call void @sys_exceptionPost(i32 %code, i32 0)
  br label %ret

ret:
  ret ptr %dest
}

; CHECK-LABEL: memset_like:
; CHECK: tstorer	r6, [r7, #12]
; CHECK: tloadr	r6, [r7, #12]
