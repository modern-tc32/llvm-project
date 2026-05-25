; RUN: llc -mtriple=tc32-unknown-none-elf -filetype=obj -o - %s | llvm-objdump -d - | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @ctz_lsb(i32 %x) {
; CHECK-LABEL: <ctz_lsb>:
; CHECK:      tmul
; CHECK-NEXT: tshftr
; CHECK-NEXT: tmov [[BASE:r[0-7]]], #0x{{[0-9a-f]+}}
; CHECK-NEXT: tshftl [[BASE]], [[BASE]], #0x2
; CHECK-NEXT: tadd [[BASE]], pc
; CHECK-NEXT: tloadrb r0, {{\[}}[[BASE]], r0]
  %ctz = call i32 @llvm.cttz.i32(i32 %x, i1 false)
  ret i32 %ctz
}

declare i32 @llvm.cttz.i32(i32, i1 immarg)
