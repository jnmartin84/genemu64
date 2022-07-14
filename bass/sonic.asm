arch n64.cpu
endian msb
output "rsp_code", create
fill 4096 // Set ROM Size
origin $00000000
include "lib/STUFF.INC"
align(8) // Align 64-Bit
RSPCode:
arch n64.rsp
base $0000
nop
nop
// get dma sem
//	mfc0 s1, COP_SP_SEM
//	nop
//	nop
//	bne	s1, r0, RSPCode
//	nop
	
// clear the dmem on startup
//	addiu t9, r0, 4092

//cleardmem:
//	sw r0, 0(t9)
//	addiu t9, t9, -4
//	bge t9, r0, cleardmem
//	nop

// now that memory clear release sem 	
//	mtc0 r0, COP_SP_SEM
//	nop
//	nop
	
// dram address of results 0(DMEM)
// tile src data 4(DMEM)
// tile dest 8(DMEM)
// vf 12(DMEM)
// hf 16(DMEM)
// was blank 20
// finished 24
// giving 28

wait_for_packet:		
    // check latest value of "giving tile"
	lw t0, 28(r0)
	nop
	// still zero, loop
	beq t0, r0, wait_for_packet
	nop

	
	// get sem
//templabel0:
//	mfc0 s1, COP_SP_SEM
//	nop
//	nop
//	bne	s1, r0, templabel0
//	nop
	// wait til dma isnt busy anymore
waitbusyful1st:
	mfc0 s5,6 
	bnez s5, waitbusyful1st 
    nop 
	
	// DMEM addr of 64
	addiu a0, r0, 64
	// DRAM tile source addr
	lw a1, 4(r0)
	// 32 byte transfer no lines no skip
	addiu a2, r0, 31	
	
	addiu t0, t0, 1
	sw t0, 20(r0)
	
	mtc0 a0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
	// transfer from dram to dmem
	mtc0 a2, COP_SP_DMA_READ
	
	// wait til dma isnt busy anymore
waitbusyful33:
	mfc0 s5,6 
	bnez s5, waitbusyful33 
    nop 
	
	// t1 = vf
	// t2 = hf
	//lw t1, 12(r0)
	addu t1,r0,r0
	//lw t2, 16(r0)
	addu t2,r0,r0

	// $t4 = loop counter
	// $t3 = eight pixels
	addiu t4, r0, 0
	addiu s0, r0, 8

	
line_loop:	
	beq t1, r0, not_vflipped
	nop

vflipped:
	addiu t5, r0, 7
	subu t5, t5, t4
	sll	t5, t5, 2
	b row_load
	nop
	
not_vflipped:
	sll t5, t4, 2

row_load:
	lw a0, 64(t5)
	// vf
	addu	a1, t2, r0
	
	// flips bytes and nybbles if vf is true
	jal	flip
	nop
	
//	lw v1, 20(r0)
	
	beq v0, r0, noclearblank
	nop

	sw r0, 20(r0)
noclearblank:
		
	// adjust for dest location
	sll t5, t4, 3
	
	// store flipped value in dmem dest
	sw v0, 128(t5)
	sw v0, 20(r0)

	addiu t4, t4, 1
	blt t4, s0, line_loop
	nop	
	
	
	// DMEM addr of 64
	addiu a0, r0, 128
	// DRAM tile dest addr
	lw a1, 8(r0)
	// 64 byte transfer no lines no skip
	addiu a2, r0, 63	
	
	mtc0 a0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
	// kick off dmem to dram
	mtc0 a2, COP_SP_DMA_WRITE
	
	// wait for dma to finish
//waitbusyful:
//	mfc0 s5,6 
 //   nop
//	nop
//	bnez s5, waitbusyful // IF TRUE RSP DMA Busy & Full
 //   nop // Delay Slot
	
	// set the flags
	addiu t7,r0,1
	sw t7, 24(r0)
	sw r0, 28(r0)
	
	// send the packet update back to the r4300
	addu a0, r0, r0
	lw a1, 0(r0)
	addiu a2, r0, 31

//templabel:
//	mfc0 s1, COP_SP_SEM
//	nop
//	nop
//	bne	s1, r0, templabel
//	nop
//waitbusyful0:
//	mfc0 s5,6 // T0 = RSP Status Register ($A4040010)
 //   nop
//	nop
//	andi s5,$c // AND RSP Status Status With $C: DMA Busy (Bit 2) DMA Full (Bit 3)
//    nop
//	bnez s5, waitbusyful0 // IF TRUE RSP DMA Busy & Full
 //   nop // Delay Slot
//	nop

	mtc0 r0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
// send back to dram
	mtc0 a2, COP_SP_DMA_WRITE

waitbusyful234:
	mfc0 s5,6 
	bnez s5, waitbusyful234 
    nop 

	// release sem
//	mtc0 r0, COP_SP_SEM
//	nop

	// halt the rsp
	addi t0, r0, 0x2
	mtc0 t0, COP_SP_STATUS
	
	// back to beginning
	b wait_for_packet
	nop
	
	
flip:
	addu	v0,a0,r0
	b L7
	nop
	beq	a1,r0,L7
	nop
	srl	a1,a0,8
	srl	v1,v0,16
	srl	a0,a0,24
	andi	t8,v0,0x00ff
	andi	t9,a1,0x00ff
	sll	a2,a0,4
	sll	v0,v0,4
	srl	a0,a0,4
	andi	a3,v1,0x00ff
	sll	a1,a1,4
	srl	t9,t9,4
	srl	t8,t8,4
	or	a2,a2,a0
	or	a1,a1,t9
	sll	a0,v1,4
	or	v0,v0,t8
	srl	v1,a3,4
	or	a0,a0,v1
	sll	v0,v0,24
	andi	v1,a2,0x00ff
	andi	a1,a1,0x00ff
	or	v0,v0,v1
	sll	a1,a1,16
	andi	a0,a0,0x00ff
	or	v0,v0,a1
	sll	a0,a0,8
	or	v0,v0,a0
L7:
	jr	ra
	nop		
	
align(8) // Align 64-Bit
base RSPCode+pc() // Set End Of RSP Code Object
RSPCodeEnd:

// __attribute__((aligned(8))) 