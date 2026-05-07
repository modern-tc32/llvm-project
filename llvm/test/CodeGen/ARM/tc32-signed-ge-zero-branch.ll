; RUN: llc -mtriple=tc32-unknown-none-elf -O2 -o - %s | FileCheck %s

target triple = "tc32-unknown-none-elf"

@tc32_signed_marker = global i32 0, align 4

define i32 @tc32_eof_callback(i32 %c, ptr %ctx) noinline optsize {
  ret i32 -1
}

define i32 @tc32_cbprintf_like(ptr %out, ptr %ctx) noinline optsize {
; CHECK-LABEL: tc32_cbprintf_like:
; CHECK:       tjl
; CHECK:       tcmp [[RC:r[0-7]]], #0
; CHECK-NEXT:  tjlt [[NEGATIVE:\.LBB[0-9]+_[0-9]+]]
; CHECK-NEXT:  tj [[NONNEGATIVE:\.LBB[0-9]+_[0-9]+]]
; CHECK-NOT:   tjge
; CHECK-NOT:   tjpl
  %rc = tail call i32 %out(i32 76, ptr %ctx)
  %ok = icmp sgt i32 %rc, -1
  %marker = select i1 %ok, i32 24589, i32 2989
  %ret = select i1 %ok, i32 1, i32 %rc
  store volatile i32 %marker, ptr @tc32_signed_marker, align 4
  ret i32 %ret
}

define i32 @main() optsize {
  %ret = tail call i32 @tc32_cbprintf_like(ptr @tc32_eof_callback, ptr null)
  ret i32 %ret
}
