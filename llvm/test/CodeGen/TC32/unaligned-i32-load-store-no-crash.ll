; RUN: llc -mtriple=tc32 -O0 -filetype=asm -o /dev/null %s
; RUN: llc -mtriple=tc32 -O2 -filetype=asm -o /dev/null %s

%ctx = type { i8, i8, i8, i8, i8, i8, i8, i8 }

@g = external global %ctx, align 1

define i32 @load_unaligned_i32() {
entry:
  %p = getelementptr inbounds i8, ptr @g, i32 1
  %v = load i32, ptr %p, align 1
  ret i32 %v
}

define void @store_unaligned_i32(i32 %v) {
entry:
  %p = getelementptr inbounds i8, ptr @g, i32 1
  store i32 %v, ptr %p, align 1
  ret void
}
