@ RUN: not llvm-mc -triple=tc32-unknown-none-elf -filetype=obj %s -o /dev/null 2>&1 | FileCheck %s

	.text
	.code	16
foo:
	cpsid i

@ CHECK: :[[@LINE-2]]:2: error: instruction is not supported on TC32
