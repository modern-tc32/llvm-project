@ RUN: llvm-mc -triple=tc32-unknown-none-elf -filetype=obj %s -o %t
@ RUN: llvm-objdump -d %t | FileCheck %s

	.text
	.code	16
foo:
	nop

@ CHECK-LABEL: <foo>:
@ CHECK: nop
