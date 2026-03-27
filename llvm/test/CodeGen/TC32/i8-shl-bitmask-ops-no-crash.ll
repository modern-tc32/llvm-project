; RUN: llc -mtriple=tc32 -O2 -filetype=asm -o /dev/null %s

define i8 @bitmask_or(i8 %lhs, i8 %rhs) {
entry:
  %bit = shl i8 1, %rhs
  %res = or i8 %lhs, %bit
  ret i8 %res
}

define i8 @bitmask_andnot(i8 %lhs, i8 %rhs) {
entry:
  %bit = shl i8 1, %rhs
  %notbit = xor i8 %bit, -1
  %res = and i8 %lhs, %notbit
  ret i8 %res
}
