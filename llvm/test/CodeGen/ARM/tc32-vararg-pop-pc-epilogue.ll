; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

%struct.__va_list = type { ptr }

@sink = global i32 0, align 4

define i32 @vararg_large(i32 %fixed, ...) minsize noinline nounwind optsize "frame-pointer"="all" "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: vararg_large:
; CHECK:       tsub sp, #12
; CHECK:       tpush {r4, r5, r6, r7, lr}
; CHECK:       tpop {r4, r5, r6, r7}
; CHECK:       tloadr [[RET:r[0-7]]], [sp]
; CHECK:       tstorer [[RET]], [sp, #12]
; CHECK-NEXT:  tadd sp, #12
; CHECK-NEXT:  tpop {pc}
; CHECK-NOT:   tjex [[RET]]
  %frame = alloca [152 x i8], align 1
  %ap = alloca %struct.__va_list, align 4
  call void @llvm.lifetime.start.p0(ptr nonnull %frame)
  call void @llvm.lifetime.start.p0(ptr nonnull %ap)
  call void @llvm.va_start.p0(ptr nonnull %ap)
  %x = va_arg ptr %ap, i32
  %xb = trunc i32 %x to i8
  store volatile i8 %xb, ptr %frame, align 1
  %last = getelementptr inbounds i8, ptr %frame, i32 151
  store volatile i8 2, ptr %last, align 1
  call void @use_frame(ptr nonnull %frame) nounwind
  call void @llvm.va_end.p0(ptr nonnull %ap)
  %a = load volatile i8, ptr %frame, align 1
  %az = zext i8 %a to i32
  %b = load volatile i8, ptr %last, align 1
  %bz = zext i8 %b to i32
  %sum = add nuw nsw i32 %bz, %az
  store volatile i32 %sum, ptr @sink, align 4
  %result = load volatile i32, ptr @sink, align 4
  call void @llvm.lifetime.end.p0(ptr nonnull %ap)
  call void @llvm.lifetime.end.p0(ptr nonnull %frame)
  ret i32 %result
}

declare void @llvm.lifetime.start.p0(ptr)
declare void @llvm.va_start.p0(ptr)
declare void @use_frame(ptr)
declare void @llvm.va_end.p0(ptr)
declare void @llvm.lifetime.end.p0(ptr)
