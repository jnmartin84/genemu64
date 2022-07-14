arch n64.cpu
endian msb
output "rsp_code", create
fill 4096 // Set ROM Size
origin $00000000
base $00000000 // Set Base Of RSP Code Object To Zero

include "lib/STUFF.INC"

align(8) // Align 64-Bit
RSPCode:
arch n64.rsp
base $0000

// t0 == giving_a_tile, 28 bytes from start of DMEM
// tile src data 4(DMEM)
// tile dest 8(DMEM)
// vf 12(DMEM)
// hf 16(DMEM)
// was blank 20(
// finished 24
// giving 28

RSPStart:
//    jal release_dma_semaphore
//	nop
//	b RSPStart
//	nop
	
	// clear "giving a tile"
//	add t0, r0, r0

wait_for_packet:
	// wait for busy to be over
	jal dma_wait
	nop
	
    // check latest value of "giving tile"
	lw t0, 28(r0)
	nop
	nop
	
	// still zero, loop
	beq t0, r0, wait_for_packet
	nop
	
//	jal acquire_dma_semaphore
//	nop
	jal dma_wait
	nop

//	b update_finish
	
	// not zero, do some things
	
	// DMEM addr of 64
	addiu a0, r0, 64
	// DRAM tile source addr
	lw a1, 4(r0)
	nop
	// 32 byte transfer no lines no skip
	addiu a2, r0, 31	
	// read data
	jal dma_read
	nop
	
	// wait for busy to be over
	jal dma_wait
	nop
	
	// t1 = vf
	// t2 = hf
	lw t1, 12(r0)
	nop
	lw t2, 16(r0)
	nop

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
	nop
//	nop
	
	// got the row in $t3
//	addu	a0, t3, r0
	// vf
	addu	a1, t2, r0
	
	// flips bytes and nybbles if vf is true
	jal	flip
	nop
	
	// adjust for dest location
	sll t5, t4, 3
	
	// store flipped value in dmem dest
	sw v0, 128(t5)
	nop

	addiu t4, t4, 1
	blt t4, s0, line_loop
	nop

finish:	

	// send the tile to the r4300
	addiu a0, r0, 128
	lw a1, 8(r0)
	nop
	addiu a2, r0, 61

	jal dma_write
	nop
	
	jal dma_wait
	nop
	
update_finish:
	// set the flags
	addiu t7,r0,1
	sw t7, 24(r0)
	nop
	sw r0, 28(r0)
	nop
	
	// send the packet update back to the r4300
	addu a0, r0, r0
	lw a1, 0(r0)
	nop
	addiu a2, r0, 31

	jal dma_wait
	nop

	jal dma_write
	nop
	
	jal dma_wait
	nop

//	jal release_dma_semaphore
//	nop
	
	b wait_for_packet
	nop

release_dma_semaphore:
	jr ra
	mtc0 r0, COP_SP_SEM

acquire_dma_semaphore:
	mfc0 t6, COP_SP_SEM
	nop
	nop
	bne t6, r0, acquire_dma_semaphore
	nop
	jr ra
	nop
	
dma_read:
	mtc0 a0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
	jr ra
	mtc0 a2, COP_SP_DMA_READ
	
dma_write:
	mtc0 a0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
	jr ra
	mtc0 a2, COP_SP_DMA_WRITE

dma_wait:
	mfc0 s7, COP_SP_DMA_BUSY
	nop
	nop
	bne s7, r0, dma_wait
	nop
	jr ra
	nop
	
flip:
	beq	a1,r0,L7
	addu	v0,a0,r0

	srl	a1,a0,8
	srl	v1,v0,16
	srl	a0,a0,24
	andi	t8,v0,0x00ff
	andi	t9,a1,0x00ff
	sll	a2,a0,4
	sll	v0,v0,4
	srl	a0,a0,4
	andi	r7,v1,0x00ff
	sll	a1,a1,4
	srl	t9,t9,4
	srl	t8,t8,4
	or	a2,a2,a0
	or	a1,a1,t9
	sll	a0,v1,4
	or	v0,v0,t8
	srl	v1,r7,4
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