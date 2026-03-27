; RUN: llc -mtriple=tc32 -O2 -filetype=asm -o /dev/null %s

define i8 @var_shift_i8(i8 %rhs) {
entry:
  %res = shl i8 1, %rhs
  ret i8 %res
}
