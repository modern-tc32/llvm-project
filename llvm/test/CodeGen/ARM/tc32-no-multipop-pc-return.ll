; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

; On TLSR8258, Zephyr was observed hanging with the PC stuck at a tpop
; that restores both the TC32 padding register r3 and PC directly:
;
;   tpop {r3, r4, r5, r6, r7, pc}
;
; Keep PC out of that pop. Vendor GCC still uses pop-to-PC for ordinary
; callee-saved registers such as {r4, r5, r6, r7, pc}.

target datalayout = "e-m:e-p:32:32-Fi8-i64:64-v128:64:128-a:0:32-n32-S64"
target triple = "tc32-unknown-none-elf"

define dso_local i32 @do_device_init_min(ptr noundef %dev) minsize noinline nounwind optsize "frame-pointer"="all" "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: do_device_init_min:
; CHECK:       tpush {r3, r4, r5, r6, r7, lr}
; CHECK-NOT:   tpop {r3, r4, r5, r6, r7, pc}
; CHECK:       tpop {r3, r4, r5, r6, r7}
; CHECK-NEXT:  tpop {pc}
  %ops.init.ptr = getelementptr inbounds nuw i8, ptr %dev, i32 16
  %ops.init = load ptr, ptr %ops.init.ptr, align 4
  %has.init = icmp eq ptr %ops.init, null
  br i1 %has.init, label %done, label %call.init

call.init:
  %init.addr = ptrtoint ptr %ops.init to i32
  %init.target.addr = and i32 %init.addr, -2
  %init.target = inttoptr i32 %init.target.addr to ptr
  %rc = tail call i32 %init.target(ptr noundef nonnull %dev) nounwind
  %is.zero = icmp eq i32 %rc, 0
  br i1 %is.zero, label %done, label %store.result

store.result:
  %abs.rc = tail call i32 @llvm.abs.i32(i32 %rc, i1 true)
  %limited.rc = tail call i32 @llvm.umin.i32(i32 %abs.rc, i32 255)
  %init.res = trunc nuw i32 %limited.rc to i8
  %state.ptr.addr = getelementptr inbounds nuw i8, ptr %dev, i32 8
  %state.ptr = load ptr, ptr %state.ptr.addr, align 4
  store i8 %init.res, ptr %state.ptr, align 1
  br label %done

done:
  %ret.rc = phi i32 [ 0, %0 ], [ %rc, %store.result ], [ 0, %call.init ]
  %state.ptr.addr2 = getelementptr inbounds nuw i8, ptr %dev, i32 8
  %state.ptr2 = load ptr, ptr %state.ptr.addr2, align 4
  %initialized.ptr = getelementptr inbounds nuw i8, ptr %state.ptr2, i32 1
  store i8 1, ptr %initialized.ptr, align 1
  %ret = sub nsw i32 0, %ret.rc
  ret i32 %ret
}

declare i32 @llvm.abs.i32(i32, i1 immarg)
declare i32 @llvm.umin.i32(i32, i32)
