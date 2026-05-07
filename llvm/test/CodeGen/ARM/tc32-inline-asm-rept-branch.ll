; RUN: llc -mtriple=tc32-unknown-none-elf -filetype=obj -o /dev/null %s
; RUN: llc -mtriple=tc32-unknown-none-elf -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

define void @tc32_inline_asm_rept_branch(i32 %x) {
; CHECK-LABEL: tc32_inline_asm_rept_branch:
; CHECK:       tcmp r0, #0
; CHECK-NEXT:  tjne [[SKIP:\.LBB0_[0-9]+]]
; CHECK-NEXT:  tj [[DONE:\.LBB0_[0-9]+]]
; CHECK:       [[SKIP]]:
; CHECK:       nop
; CHECK:       [[DONE]]:
entry:
  %is_zero = icmp eq i32 %x, 0
  br i1 %is_zero, label %done, label %pad

pad:
  call void asm sideeffect ".rept 320\0Anop\0A.endr\0A", "~{memory}"()
  br label %done

done:
  ret void
}
