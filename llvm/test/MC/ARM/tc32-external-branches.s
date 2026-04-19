// RUN: llvm-mc -triple=tc32 -filetype=obj %s -o %t
// RUN: llvm-readobj -r %t | FileCheck %s --check-prefix=RELOC
// RUN: llvm-objdump --triple=tc32 -dr %t | FileCheck %s --check-prefix=DIS

  .syntax unified
  .thumb
  .text

start:
  tj ext_jump
  tjeq ext_bcc
  tjl ext_call

// RELOC:      Relocations [
// RELOC:      Section {{.*}} .rel.text {
// RELOC-NEXT:   0x0 R_ARM_THM_JUMP24 ext_jump
// RELOC-NEXT:   0x4 R_ARM_THM_JUMP19 ext_bcc
// RELOC-NEXT:   0x8 R_ARM_THM_CALL ext_call

// DIS:      00000000 <start>:
// DIS-NEXT: 0: ff 97 fe 6f   tj
// DIS:      00000000:  R_ARM_THM_JUMP24 ext_jump
// DIS-NEXT: 4: 3f 94 fe 7f   tjeq
// DIS:      00000004:  R_ARM_THM_JUMP19 ext_bcc
// DIS-NEXT: 8: ff 97 fe 9f   tjl
// DIS:      00000008:  R_ARM_THM_CALL ext_call
