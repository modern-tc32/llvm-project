; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -verify-machineinstrs -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

%struct.list = type { ptr, ptr }
%struct.kernel_like = type { [5 x i32], ptr, %struct.list }

@gk = external global %struct.kernel_like

define ptr @ready_q_ptr() minsize noinline nounwind optsize "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: ready_q_ptr:
; CHECK:       tloadr [[BASE:r[0-7]]], .LCPI0_0
; CHECK:       tmov [[PQ:r[0-7]]], #24
; CHECK:       tadd [[PQ]], [[PQ]], [[BASE]]
  %pq = getelementptr inbounds %struct.kernel_like, ptr @gk, i32 0, i32 2
  %head = load ptr, ptr %pq, align 4
  %is_self = icmp eq ptr %head, %pq
  %ret = select i1 %is_self, ptr null, ptr %pq
  ret ptr %ret
}

define ptr @kernel_base_from_runq(ptr %pq) minsize noinline nounwind optsize "target-cpu"="tc32" "target-features"="+armv4t,+thumb-mode" {
; CHECK-LABEL: kernel_base_from_runq:
; CHECK:       tmov [[BASE:r[0-7]]], r0
; CHECK:       tmov [[OFF:r[0-7]]], #24
; CHECK:       tmov [[TMP:r[0-7]]], [[OFF]]
; CHECK:       tmov [[RES:r[0-7]]], [[BASE]]
; CHECK:       tsub [[RES]], [[RES]], [[TMP]]
  %base = getelementptr inbounds i8, ptr %pq, i32 -24
  %head = load ptr, ptr %pq, align 4
  %is_null = icmp eq ptr %head, null
  %ret = select i1 %is_null, ptr null, ptr %base
  ret ptr %ret
}
