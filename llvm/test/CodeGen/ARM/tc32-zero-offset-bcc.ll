; RUN: llc -mtriple=tc32-unknown-none-elf -o - %s | FileCheck %s
; RUN: llc -mtriple=tc32-unknown-none-elf -filetype=obj -o /dev/null %s

target triple = "tc32-unknown-none-elf"

%struct.list = type { ptr, ptr, ptr }

define ptr @tc32_zero_offset_bcc(ptr %l, ptr %sentinel) {
; CHECK-LABEL: tc32_zero_offset_bcc:
; CHECK:       tloadr r0, [r0, #4]
; CHECK-NEXT:  nop
; CHECK-NEXT:  nop
; CHECK-NEXT:  tcmp r0, r1
; CHECK-NEXT:  tjne [[KEEP:\.LBB0_[0-9]+]]
; CHECK:       tmov r0, #0
; CHECK-NEXT:  nop
; CHECK:       [[KEEP]]:
entry:
  %head.addr = getelementptr inbounds %struct.list, ptr %l, i32 0, i32 1
  %head = load ptr, ptr %head.addr, align 4
  %is_empty = icmp eq ptr %head, %sentinel
  %.head = select i1 %is_empty, ptr null, ptr %head
  ret ptr %.head
}
