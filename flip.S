	.file	1 "flip.c"
	.section .mdebug.abiO64
	.previous
	.section .gcc_compiled_long32
	.previous
	.nan	legacy
	.module	fp=64
	.module	oddspreg
	.text
	.align	2
	.align	3
	.globl	flip
	.set	nomips16
	.set	nomicromips
	.ent	flip
	.type	flip, @function
flip:
	.frame	$sp,0,$31		# vars= 0, regs= 0/0, args= 0, gp= 0
	.mask	0x00000000,0
	.fmask	0x00000000,0
	.set	noreorder
	.set	nomacro
	beq	$5,$0,$L7
	move	$2,$4

	srl	$5,$4,8
	srl	$3,$2,16
	srl	$4,$4,24
	andi	$8,$2,0x00ff
	andi	$9,$5,0x00ff
	sll	$6,$4,4
	sll	$2,$2,4
	srl	$4,$4,4
	andi	$7,$3,0x00ff
	sll	$5,$5,4
	srl	$9,$9,4
	srl	$8,$8,4
	or	$6,$6,$4
	or	$5,$5,$9
	sll	$4,$3,4
	or	$2,$2,$8
	srl	$3,$7,4
	or	$4,$4,$3
	sll	$2,$2,24
	andi	$3,$6,0x00ff
	andi	$5,$5,0x00ff
	or	$2,$2,$3
	sll	$5,$5,16
	andi	$4,$4,0x00ff
	or	$2,$2,$5
	sll	$4,$4,8
	or	$2,$2,$4
$L7:
	jr	$31
	nop

	.set	macro
	.set	reorder
	.end	flip
	.size	flip, .-flip
	.ident	"GCC: (GNU) 10.2.0"
