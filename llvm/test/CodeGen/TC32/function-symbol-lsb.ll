; RUN: llc -mtriple=tc32 -filetype=obj %s -o - | llvm-readelf -s - | FileCheck %s

define i32 @foo(i32 %a) {
entry:
  %add = add i32 %a, 1
  ret i32 %add
}

; CHECK: Symbol table '.symtab'
; CHECK: FUNC    GLOBAL
; CHECK: 00000001
; CHECK: foo
