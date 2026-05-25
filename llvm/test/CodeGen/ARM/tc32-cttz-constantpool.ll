; RUN: llc -mtriple=tc32-unknown-none-elf -filetype=obj -o - %s | llvm-objdump -d - | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @ctz_lsb(i32 %x) {
; CHECK-LABEL: <ctz_lsb>:
; CHECK:      tjl {{.*}} <__tc32_cttzsi2>
; CHECK-NOT:  tloadrb
  %ctz = call i32 @llvm.cttz.i32(i32 %x, i1 false)
  ret i32 %ctz
}

define i32 @ffs_lsb(i32 %x) {
; CHECK-LABEL: <ffs_lsb>:
; CHECK:      tjl {{.*}} <__tc32_cttzsi2>
; CHECK-NOT:  tloadrb
  %ffs = call i32 @llvm.cttz.i32(i32 %x, i1 true)
  ret i32 %ffs
}

; CHECK-LABEL: <__tc32_cttzsi2>:
; CHECK:      tcmp r0, #0x0
; CHECK:      tmov r1, #0x0
; CHECK:      tmovn r1, r1
; CHECK:      tmov r2, #0x1
; CHECK:      tadd r1, r1, r2
; CHECK:      tshftr r2, r1
; CHECK:      tjmi
; CHECK-NOT:  tloadrb

declare i32 @llvm.cttz.i32(i32, i1 immarg)
