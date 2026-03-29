	.file	"lowreg-custom-isel-no-crash.ll"
	.text
	.global	not_const                       @ -- Begin function not_const
	.p2align	1
	.type	not_const,%function
not_const:                              @ @not_const
@ %bb.0:                                @ %entry
	tmovs	r1, #255
	tshftl	r2, r1, #8
	tadds	r2, r2, #255
	tshftl	r2, r2, #8
	tadds	r2, r2, #255
	tshftl	r2, r2, #8
	tadds	r2, r2, #255
txor	r0, r2
	tadds	r0, r0, r1
tmov	pc, lr
.Lfunc_end0:
	.size	not_const, .Lfunc_end0-not_const
                                        @ -- End function
	.global	minus_four                      @ -- Begin function minus_four
	.p2align	1
	.type	minus_four,%function
minus_four:                             @ @minus_four
@ %bb.0:                                @ %entry
	tmovs	r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #252
	tadds	r0, r0, r1
tmov	pc, lr
.Lfunc_end1:
	.size	minus_four, .Lfunc_end1-minus_four
                                        @ -- End function
	.global	align4                          @ -- Begin function align4
	.p2align	1
	.type	align4,%function
align4:                                 @ @align4
@ %bb.0:                                @ %entry
	tmovs	r1, #3
	tadds	r0, r0, r1
	tmovs	r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #255
	tshftl	r1, r1, #8
	tadds	r1, r1, #252
tand	r0, r1
tmov	pc, lr
.Lfunc_end2:
	.size	align4, .Lfunc_end2-align4
                                        @ -- End function
	.section	".note.GNU-stack","",%progbits
