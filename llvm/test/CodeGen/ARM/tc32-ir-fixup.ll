; RUN: llc -mtriple=tc32-unknown-none-elf %s -o - | FileCheck %s

target triple = "tc32-unknown-none-elf"

define void @hint_and_signal() {
; CHECK-LABEL: hint_and_signal:
; CHECK-NOT: sev
; CHECK: tjex
entry:
  call void @llvm.arm.hint(i32 4)
  call void asm sideeffect "sev", ""()
  ret void
}

define void @disable_irq() {
; CHECK-LABEL: disable_irq:
; CHECK: tloadr r0, .LCPI{{[0-9]+}}_0
; CHECK-NEXT: tmov r1, #0
; CHECK-NEXT: tstorerb r1, [r0]
; CHECK: .long 8390211
entry:
  call void asm sideeffect "cpsid i", ""()
  ret void
}

define void @enable_irq() {
; CHECK-LABEL: enable_irq:
; CHECK: tloadr r0, .LCPI{{[0-9]+}}_0
; CHECK-NEXT: tmov r1, #1
; CHECK-NEXT: tstorerb r1, [r0]
; CHECK: .long 8390211
entry:
  call void asm sideeffect "cpsie i", ""()
  ret void
}

define i32 @mrs_read() {
; CHECK-LABEL: mrs_read:
; CHECK: tmrss	r0
entry:
  %v = call i32 asm sideeffect "mrs $0, PRIMASK", "=r"()
  ret i32 %v
}

define void @msr_write(i32 %x) {
; CHECK-LABEL: msr_write:
; CHECK: tmssr	r0
entry:
  call void asm sideeffect "msr PRIMASK, $0", "r"(i32 %x)
  ret void
}

define i32 @mrs_primask_ns() {
; CHECK-LABEL: mrs_primask_ns:
; CHECK-NOT: tmrss
; CHECK: tmov r0, #0
entry:
  %v = call i32 asm sideeffect "mrs $0, PRIMASK_NS", "=r"()
  ret i32 %v
}

define void @msr_primask_ns(i32 %x) {
; CHECK-LABEL: msr_primask_ns:
; CHECK-NOT: tmssr
; CHECK-NOT: tadd sp
; CHECK: tjex
entry:
  call void asm sideeffect "msr PRIMASK_NS, $0", "r"(i32 %x)
  ret void
}

define i32 @mrs_other() {
; CHECK-LABEL: mrs_other:
; CHECK-NOT: tmrss
; CHECK: tmov r0, #0
entry:
  %v = call i32 asm sideeffect "mrs $0, CONTROL", "=r"()
  ret i32 %v
}

define void @msr_other(i32 %x) {
; CHECK-LABEL: msr_other:
; CHECK-NOT: tmssr
; CHECK-NOT: tadd sp
; CHECK: tjex
entry:
  call void asm sideeffect "msr CONTROL, $0", "r"(i32 %x)
  ret void
}
declare void @llvm.arm.hint(i32 immarg)
