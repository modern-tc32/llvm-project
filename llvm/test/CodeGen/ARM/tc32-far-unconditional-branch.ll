; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

@tc32_far_tj_sink = global i32 0, align 4

; Regression for a Zephyr cbprintf shape where a short inverted conditional
; branch skips over an out-of-range unconditional branch to the epilogue. The
; dedicated TC32 long TJ encoding disassembles plausibly but does not execute
; reliably on TLSR8258, so codegen must use TJL when LR is already spilled.
define i32 @tc32_far_tj_epilogue_repro(ptr %out, ptr %s, ptr %ctx) minsize noinline nounwind optsize "frame-pointer"="all" "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: tc32_far_tj_epilogue_repro:
; CHECK-NOT: {{\ttj[ \t]+\.LBB}}
; CHECK:      tcmp r0, #0
; CHECK-NEXT: tjeq [[ZERO_VENEER:.LBB[0-9_]+]]
; CHECK-NEXT: tj [[NONZERO:.LBB[0-9_]+]]
; CHECK-NEXT: [[ZERO_VENEER]]:
; CHECK-NEXT: tjl [[DONE_FROM_ZERO:.LBB[0-9_]+]]
; CHECK-NEXT: [[NONZERO]]:
; CHECK-NOT: {{\ttj[ \t]+\.LBB}}
; CHECK:      tcmp r0, #0
; CHECK-NEXT: tjmi [[NEGATIVE_VENEER:.LBB[0-9_]+]]
; CHECK-NEXT: tj [[CONTINUE:.LBB[0-9_]+]]
; CHECK-NEXT: [[NEGATIVE_VENEER]]:
; CHECK-NEXT: tjl [[DONE_FROM_NEGATIVE:.LBB[0-9_]+]]
; CHECK-NOT: {{\ttj[ \t]+\.LBB}}
entry:
  %first = load i8, ptr %s, align 1
  %first_is_zero = icmp eq i8 %first, 0
  br i1 %first_is_zero, label %done, label %loop

loop:
  %ch = phi i8 [ %next_ch, %continue ], [ %first, %entry ]
  %n = phi i32 [ %n.next, %continue ], [ 0, %entry ]
  %p = phi ptr [ %p.next, %continue ], [ %s, %entry ]
  %ch.ext = sext i8 %ch to i32
  %call = tail call i32 %out(i32 %ch.ext, ptr %ctx)
  %call_failed = icmp slt i32 %call, 0
  br i1 %call_failed, label %done, label %continue

continue:
  tail call void asm sideeffect ".rept 1500\0Anop\0A.endr\0A", "~{memory}"()
  %p.next = getelementptr inbounds i8, ptr %p, i32 1
  %n.next = add nuw nsw i32 %n, 1
  %next_ch = load i8, ptr %p.next, align 1
  %next_is_zero = icmp eq i8 %next_ch, 0
  br i1 %next_is_zero, label %done, label %loop

done:
  %result = phi i32 [ 0, %entry ], [ %n.next, %continue ], [ %n, %loop ]
  store volatile i32 %result, ptr @tc32_far_tj_sink, align 4
  ret i32 %result
}
