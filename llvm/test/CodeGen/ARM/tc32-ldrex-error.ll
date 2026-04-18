; RUN: not llc -mtriple=tc32-unknown-none-elf %s -o /dev/null 2>&1 | FileCheck %s

target triple = "tc32-unknown-none-elf"

define i32 @ldrex_i32(ptr %p) {
; CHECK: error: TC32 does not support atomic operations
entry:
  %value = call i32 @llvm.arm.ldrex.p0(ptr elementtype(i32) %p)
  ret i32 %value
}

declare i32 @llvm.arm.ldrex.p0(ptr elementtype(i32))
