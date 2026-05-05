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
// RELOC-NEXT:   0x0 R_ARM_THM_JUMP11 ext_jump
// RELOC-NEXT:   0x2 R_ARM_THM_JUMP8 ext_bcc
// RELOC-NEXT:   0x4 R_ARM_THM_CALL ext_call

// DIS:      00000000 <start>:
// DIS-NEXT: 0: fe 87         tj
// DIS:      00000000:  R_TC32_JUMP11 ext_jump
// DIS-NEXT: 2: fe c0         tjeq
// DIS:      00000002:  R_ARM_THM_JUMP8 ext_bcc
// DIS-NEXT: 4: ff 97 fe 9f   tjl
// DIS:      00000004:  R_ARM_THM_CALL ext_call
