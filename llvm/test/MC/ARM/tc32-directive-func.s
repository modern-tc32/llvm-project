@ RUN: llvm-mc -triple=tc32-unknown-none-elf -filetype=obj %s -o %t
@ RUN: llvm-objdump -d %t | FileCheck %s

	.text
	.globl	foo
	.type	foo,%function
	.code	16
	.tc32_func
foo:
	tpush	{r6, lr}
	tmov	r6, sp
	tpop	{r6}
	tpop	{r0}
	tjex	r0

@ CHECK-LABEL: <foo>:
@ CHECK: tpush	{r6, lr}
@ CHECK: tmov	r6, sp
@ CHECK: tjex	r0
