# REQUIRES: arm

# RUN: llvm-mc -filetype=obj -triple=tc32-unknown-none-elf %s -o %t.o
# RUN: ld.lld -m tc32elf %t.o -o %t
# RUN: llvm-readobj --file-headers %t | FileCheck %s
# RUN: echo 'OUTPUT_FORMAT(elf32-littletc32)' > %t.script
# RUN: ld.lld %t.script %t.o -o %t
# RUN: llvm-readobj --file-headers %t | FileCheck %s

# CHECK:      ElfHeader {
# CHECK:        Class: 32-bit (0x1)
# CHECK:        DataEncoding: LittleEndian (0x1)
# CHECK:        Machine: EM_TC32 (0x8800)
# CHECK:      }

    .syntax unified
    .thumb
    .globl _start
    .thumb_func
_start:
    nop
