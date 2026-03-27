; RUN: llc -mtriple=tc32 -O2 -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32 -O2 -filetype=obj -o /dev/null %s

; ARM keeps the frame-pointer reservation decision independent from the final
; stack size. TC32 used to reserve r7 only after PEI, so Greedy could allocate
; r7 in a frame-pointer function and later PEI would also need it as FP.
; This reproducer forces spills from a simple i8 shift/mask sequence.

target datalayout = "e-m:e-p:32:32-i8:8:8-i16:16:16-i32:32:32-i64:32:32-f32:32:32-f64:32:32-n32-S32"
target triple = "tc32"

declare void @llvm.fake.use(...)

; CHECK-LABEL: pwm_start_repro:
; CHECK-NOT: tmov	r7,
; CHECK-NOT: taddc	r7,
; CHECK-NOT: tsub	r7,

define dso_local void @pwm_start_repro(i8 noundef zeroext range(i8 0, 3) %0) #0 {
  %2 = icmp eq i8 %0, 0
  br i1 %2, label %3, label %6

3:
  %4 = load volatile i8, ptr inttoptr (i32 8390529 to ptr), align 1
  %5 = or i8 %4, 1
  store volatile i8 %5, ptr inttoptr (i32 8390529 to ptr), align 1
  br label %10

6:
  %7 = shl nuw nsw i8 1, %0
  %8 = load volatile i8, ptr inttoptr (i32 8390528 to ptr), align 128
  %9 = or i8 %8, %7
  store volatile i8 %9, ptr inttoptr (i32 8390528 to ptr), align 128
  br label %10

10:
  call void (...) @llvm.fake.use(i8 %0)
  ret void
}

attributes #0 = { nofree noinline norecurse nounwind memory(readwrite, target_mem0: none, target_mem1: none) "frame-pointer"="all" }
