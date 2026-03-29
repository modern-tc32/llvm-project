; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s

declare zeroext i8 @sys_exceptionPost(i16 zeroext, i8 zeroext)
declare zeroext i8 @is_ev_buf(ptr)

define ptr @memset_guarded(ptr returned %dest, i32 %val, i32 %len) {
entry:
  %isnull = icmp eq ptr %dest, null
  br i1 %isnull, label %trap, label %check

check:
  %ev = tail call zeroext i8 @is_ev_buf(ptr nonnull %dest)
  %evnz = icmp ne i8 %ev, 0
  %big = icmp ugt i32 %len, 504
  %bad = and i1 %big, %evnz
  br i1 %bad, label %trap.big, label %ret

trap:
  %t0 = tail call zeroext i8 @sys_exceptionPost(i16 zeroext 50, i8 zeroext 0)
  br label %ret

trap.big:
  %t1 = tail call zeroext i8 @sys_exceptionPost(i16 zeroext 54, i8 zeroext 0)
  br label %ret

ret:
  ret ptr %dest
}

define ptr @memcpy_guarded(ptr returned %out, ptr %in, i32 %len) {
entry:
  %iszero = icmp eq i32 %len, 0
  br i1 %iszero, label %ret, label %nonnull_out

nonnull_out:
  %outnull = icmp eq ptr %out, null
  br i1 %outnull, label %trap.out, label %nonnull_in

nonnull_in:
  %innull = icmp eq ptr %in, null
  br i1 %innull, label %trap.in, label %check

check:
  %ev = tail call zeroext i8 @is_ev_buf(ptr nonnull %out)
  %evnz = icmp ne i8 %ev, 0
  %big = icmp ugt i32 %len, 504
  %bad = and i1 %big, %evnz
  br i1 %bad, label %trap.big, label %ret

trap.out:
  %t0 = tail call zeroext i8 @sys_exceptionPost(i16 zeroext 71, i8 zeroext 0)
  br label %ret

trap.in:
  %t1 = tail call zeroext i8 @sys_exceptionPost(i16 zeroext 75, i8 zeroext 0)
  br label %ret

trap.big:
  %t2 = tail call zeroext i8 @sys_exceptionPost(i16 zeroext 79, i8 zeroext 0)
  br label %ret

ret:
  ret ptr %out
}

; CHECK-LABEL: memset_guarded:
; CHECK: tmov	r4, r0
; CHECK: tmov	r0, r4
; CHECK-NEXT: tjl	is_ev_buf
; CHECK: tmovs	r0, #54
; CHECK: tmovs	r1, #0
; CHECK-NEXT: tjl	sys_exceptionPost
; CHECK: tmov	r0, r4
; CHECK: tmov	pc, lr

; CHECK-LABEL: memcpy_guarded:
; CHECK: tmov	r4, r0
; CHECK: tjl	is_ev_buf
; CHECK: tmov	r1, r0
; CHECK: tmov	r0, r4
; CHECK: tmovs	r1, #0
; CHECK-NEXT: tjl	sys_exceptionPost
; CHECK: tmov	r0, r4
; CHECK: tmov	pc, lr
