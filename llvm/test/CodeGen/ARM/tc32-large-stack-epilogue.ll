; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

@sink = global i32 0, align 4

define i32 @tc32_large_stack_callback_loop(ptr readonly %out, ptr %ctx, ptr readonly %s) minsize noinline nounwind optsize "frame-pointer"="all" "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: tc32_large_stack_callback_loop:
; CHECK:       tpush {r4, r5, r6, r7, lr}
; CHECK:       tsub sp, #{{[0-9]+}}
; CHECK:       tadd sp, #{{[0-9]+}}
; CHECK-NEXT:  tpop {r4, r5, r6, r7, pc}
; CHECK-NOT:   tpop {r1}
; CHECK-NOT:   tjex r1
  %frame = alloca [152 x i8], align 1
  store volatile i8 1, ptr %frame, align 1
  %last = getelementptr inbounds i8, ptr %frame, i32 151
  store volatile i8 2, ptr %last, align 1
  call void @use_frame(ptr nonnull %frame) nounwind
  br label %loop

loop:
  %p = phi ptr [ %s, %0 ], [ %next, %body ]
  %count = phi i32 [ 0, %0 ], [ %inc, %body ]
  %ch = load i8, ptr %p, align 1
  %done = icmp eq i8 %ch, 0
  br i1 %done, label %finish, label %body

body:
  %c = sext i8 %ch to i32
  %rc = call i32 %out(i32 %c, ptr %ctx) nounwind
  %ok = icmp sgt i32 %rc, -1
  %next = getelementptr inbounds i8, ptr %p, i32 1
  %inc = add nuw nsw i32 %count, 1
  br i1 %ok, label %loop, label %ret

finish:
  %a = load volatile i8, ptr %frame, align 1
  %az = zext i8 %a to i32
  %b = load volatile i8, ptr %last, align 1
  %bz = zext i8 %b to i32
  %sum = add nuw nsw i32 %bz, %az
  store volatile i32 %sum, ptr @sink, align 4
  br label %ret

ret:
  %result = phi i32 [ %count, %finish ], [ %rc, %body ]
  ret i32 %result
}

declare void @use_frame(ptr)
