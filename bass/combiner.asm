arch n64.cpu
endian msb
output "rsp_code", create
fill 4096 // Set ROM Size
origin $00000000
include "lib/STUFF.INC"
include "lib/GFX.INC"
include "lib/RDP.INC"
constant taskdram_addr = 0
constant sprite_data_addr = 4

constant dram_addr0 = 8
constant palnum0 = 12
constant vf0 = 16
constant hf0 = 20

constant dram_addr1 = 24
constant palnum1 = 28
constant vf1 = 32
constant hf1 = 36

constant received = 40
constant finished = 44

constant dmem0 = 64
constant dmem1 = 128

constant combined_dmem = 256

constant vorigin = 768

constant tlut = 1024 

align(8) // Align 64-Bit
RSPCode:
arch n64.rsp
base $0000

//wait_for_packet:		
//	lw t0, received(r0)
//	nop
//	beq t0, r0, wait_for_packet

    // wait for active DMA to complete
	RSPDMASPWait()


	// t1 = tile 0 DRAM addr
	lw t1, dram_addr0(r0)
    // I'm honestly not sure wtf this is about, I don't even remember writing this
	// It looks like maybe I was trying to write something for the "last tilenum0/1 pair was the same" case in the vdp code
    // this is what it was, it used a special value of 3333 for DRAM addr to signal "just copy the last DMEM combined tile to DRAM dest"
	lui	t9, 0
	ori t9, t9, 0x3333
	beq	t9, t1, just_dma_the_last_thing
	nop
	
	//
	// first tile dma
    //
    // t0 is DMEM address for tile 0 destination
	addiu t0, r0, dmem0
	mtc0 t0, COP_SP_DMA_DMEM
	// t1 is DRAM address for tile 0 source
    mtc0 t1, COP_SP_DMA_DRAM
	// copy in tile 0
	// 32 bytes no skips
    // 8 * 8 @ 4bpp
	addiu t2, r0, 31
	// this starts the DMA from DRAM to DMEM
	mtc0 t2, COP_SP_DMA_READ
    // repeat for tile 1	
	// copy in tile 1
	lw t4, dram_addr1(r0)
	addiu t3, r0, dmem1
	mtc0 t3, COP_SP_DMA_DMEM
    mtc0 t4, COP_SP_DMA_DRAM
	addiu t5, r0, 31
	// second tile dma starts
    // can have two DMA, one active one waiting
	// we already tested DMA was available before doing this
	mtc0 t5, COP_SP_DMA_READ

	// t8 is 0
	addu	t8, r0, r0
	// t9 is 8
	addiu	t9, r0, 8

	// uses s5
	// wait for the two DRAM-to-DMEM DMA to complete
	RSPDMASPWait()

	// is tile 0 vflipped
	lw	s4, vf0(r0)
	// is tile 0 hflipped
	lw	s5, hf0(r0)
	// is tile 1 vflipped
	lw	s6, vf1(r0)
	// is tile 1 hflipped
	lw	s7, hf1(r0)
    
	// tile 0 hscroll pixoff 
	lw r28, 12*4(r0)
	// tile 1 hscroll pixoff 
	lw r29, 13*4(r0)
	sll r28,r28,2
	sll r29,r29,2
	
// t8 == loop counter
// t9 == 8, limit


    // we use palette number as part of the 4bpp to 8bpp conversion
	// palette number is really (palnum << 4)
	// because it is in the high nibble, we can add it directly
	// to extracted nibble from 4bpp tile source
	// and if we put the 4 16-color palette togethers into a
	// 64-color palette on the RDP
	// we have colors from 00 - 3F
	// which coincide with 
	// palette 0 - 3, color 0 - F
	//
	// palette8_index(pal4num,col4num) = (pal4num<<4) | col4num
	//
    // tile 0 palette number 
	lw		k0, palnum0(r0)
	// tile 1 palette number
	lw		k1, palnum1(r0)
	
line_loop:

check_vf_1:
	beq		s4,	r0, check_vf_2
	addu	t0,	r0,	t8
vf_1:
	addiu	t0, r0, 7
	subu	t0, t0, t8
	
check_vf_2:	
	beq		s6,	r0, row_load
	addu	t1,	r0,	t8
vf_2:
	addiu	t1, r0, 7
	subu	t1, t1, t8
	
row_load:
	sll		t0, t0, 2
	lw		t3, dmem0(t0)
	sll		t1, t1, 2
	lw		t5, dmem1(t1)

	// two calls to flip
	// which touch a0-a3,v0-v1, two temps
	add		a0, r0, t3
	add		a1, r0, s5
	jal flip
	nop
	// beq r28, r0 to actually do the test
	beq		r28, r0, skip_pixoff
	addi	a0,	r0, 32
	subu	a0, a0, r28
	sll		a2, v0, a0
	srl		a3, v0, r28
	or		v0,	a2, a3
skip_pixoff:	
	add		t3, r0, v0
	
	add		a0, r0, t5
	add		a1, r0, s7
	jal flip
	nop

	// beq r29, r0 to actually do the test
	beq r0, r29, skip_pixoff2
	addi	a0,	r0, 32
	subu	a0, a0, r29
	sll		a2, v0, a0
	srl		a3, v0, r29
	or		v0,	a2, a3
skip_pixoff2:
	add		t5, r0, v0
	
	add		a0, r0, t3
	add		a1,	r0,	k0 //t4
	add		a2, r0, t5
	add		a3, r0, k1 // t6
	
// a0 == row of eight pixels from B tile
// a1 == palnum B
// a2 == row eight pixels from A tile
// a3 == palnum A

// v0 == first 4 8bpp pixels of combined tile row
// v1 == second 4 8bpp pixels of combined tile row

// t0 == B source pixel 0
// t1 == B source pixel 1
// t2 == B source pixel 2
// t3 == B source pixel 3

// t4 == A source pixel 0
// t5 == A source pixel 1
// t6 == A source pixel 2
// t7 == A source pixel 3

// s0 == combined p 0
// s1 == combined p 1
// s2 == combined p 2
// s3 == combined p 3

first_half:
	// first tile pixels
	srl		t0,	a0,	28 
	andi	t0,	t0,	0x000f
	or		t0,	t0, a1

	srl		t1,	a0,	24
	andi	t1,	t1,	0x000f
	or		t1,	t1, a1

	srl		t2,	a0,	20
	andi	t2,	t2,	0x000f
	or		t2,	t2, a1

	srl		t3,	a0,	16
	andi	t3,	t3,	0x000f
	or		t3,	t3, a1

	// second tile pixels
	srl		t4,	a2,	28 
	andi	t4,	t4,	0x000f
	or		t4,	t4, a3

	srl		t5,	a2,	24
	andi	t5,	t5,	0x000f
	or		t5,	t5, a3

	srl		t6,	a2,	20
	andi	t6,	t6,	0x000f
	or		t6,	t6, a3

	srl		t7,	a2,	16
	andi	t7,	t7,	0x000f
	or		t7,	t7, a3
	
	// test which pixel to use
	// it is whichever is not blank or the higher one if both arent
	
	addu	s0, t0, r0
//	andi	r1, s0, 0x000f
//	beq		r1, r0, swap_anyway0
	andi	r1, t4, 0x000f
	beq		r1,	r0,	use_B0
	nop
swap_anyway0:
	add		s0,	t4,	r0
use_B0:
	
	addu	s1, t1, r0
//	andi	r1, s1, 0x000f
//	beq		r1, r0, swap_anyway1
	andi	r1, t5, 0x000f
	beq		r1,	r0,	use_B1
	nop
swap_anyway1:	
	add		s1,	t5,	r0
use_B1:	

	addu	s2, t2, r0
//	andi	r1, s2, 0x000f
//	beq		r1, r2, swap_anyway2
	andi	r1, t6, 0x000f
	beq		r1,	r0,	use_B2
	nop
swap_anyway2:
	add		s2,	t6,	r0
use_B2:

	addu	s3, t3, r0
//	andi	r1, s3, 0x000f
//	beq		r1, r0, swap_anyway3
	andi	r1, t7, 0x000f
	beq		r1,	r0,	use_B3
	nop
swap_anyway3:
	add		s3,	t7,	r0
use_B3:

    // at this point we have the combined first 4 pixels of row, separately
	
	sll		s0, s0, 24
	sll		s1,	s1,	16
	sll		s2,	s2,	8
	addu	v0,	r0,	s3
	or		v0,	v0,	s2
	or		v0,	v0,	s1
	or		v0,	v0,	s0
	
second_half:
	srl		t0,	a0,	12 
	andi	t0,	t0,	0x000f
	or		t0,	t0, a1

	srl		t1,	a0,	8
	andi	t1,	t1,	0x000f
	or		t1,	t1, a1

	srl		t2,	a0,	4
	andi	t2,	t2,	0x000f
	or		t2,	t2, a1

//	srl		t3,	a0,	0
	addu	t3, a0, r0
	andi	t3,	t3,	0x000f
	or		t3,	t3, a1

	srl		t4,	a2,	12 
	andi	t4,	t4,	0x000f
	or		t4,	t4, a3

	srl		t5,	a2,	8
	andi	t5,	t5,	0x000f
	or		t5,	t5, a3

	srl		t6,	a2,	4
	andi	t6,	t6,	0x000f
	or		t6,	t6, a3

//	srl		t7,	a2,	0
	addu	t7, a2, r0
	andi	t7,	t7,	0x000f
	or		t7,	t7, a3
	
	addu	s0, t0, r0
//	andi	r1, s0, 0x000f
//	beq		r1, r0, swap_anyway4
	andi	r1, t4, 0x000f
	beq		r1,	r0,	use_B4
	nop
swap_anyway4:
	add		s0,	t4,	r0
use_B4:
	
	addu	s1, t1, r0
//	andi	r1, s1, 0xf
//	beq		r1, r1, swap_anyway5
	andi	r1, t5, 0xf
	beq		r1,	r0,	use_B5
	nop
swap_anyway5:
	add		s1,	t5,	r0
use_B5:	

	addu	s2, t2, r0
//	andi	r1, s2, 0xf
//	beq		r1, r0, swap_anyway6
	andi	r1, t6, 0xf
	beq	r1,	r0,	use_B6
	nop
swap_anyway6:
	add		s2,	t6,	r0
use_B6:

	addu	s3, t3, r0
//	andi	r1, s3, 0xf
//	beq		r1, r0, swap_anyway7
	andi	r1, t7, 0xf
	beq		r1,	r0,	use_B7
	nop
swap_anyway7:
	add		s3,	t7,	r0
use_B7:

    // at this point we have the combined first 4 pixels of row, separately
	
	sll		s0, s0, 24
	sll		s1,	s1,	16
	sll		s2,	s2,	8
	addu	v1, r0,	s3
	or		v1,	v1,	s2
	or		v1,	v1,	s1
	or		v1,	v1,	s0	
	
// now we have in v0 the first 4 combined 8bpp pixels as a uint32_t
// and we have in v1 the next  4 combined 8bpp pixels as a uint32_t

	sll		r1, t8, 3 
	// 4 for 16 bit

	sw v0, combined_dmem(r1)
	sw v1, 4+combined_dmem(r1)
	
	// now loop again
	addiu t8, t8, 1
	blt t8, t9, line_loop
	nop	

just_dma_the_last_thing:	
	
// copy the i%4 value to construct the dma write reg val	
	lw t0, received(r0)

	// set the flags
	addiu t7,r0,1

	// t1 = (i%4)*8
	// start x pixel
	sll t1, t0, 3
	
	// then the rest of the parameters are
	// copy 8 bytes at a time
	// 8 lines
	// skip (64-8) pixels

	sw t7, finished(r0)
	sw r0, received(r0)

	// send combined pixels back to dram
	addiu t3, r0, combined_dmem
	// sprite
	// use start x pixel offset
	lw t4, sprite_data_addr(r0)
	add	t4,t4,t1
	lui	t5, 0x0180
	ori t5, t5, 0x7007

	// combined tile dma
	mtc0 t3, COP_SP_DMA_DMEM
    mtc0 t4, COP_SP_DMA_DRAM
	// transfer from dram to dmem
	mtc0 t5, COP_SP_DMA_WRITE

	// uses s5
//	RSPDMASPWait()

	// send the packet update back to the r4300
	lw a1, 0(r0)
	addiu a2, r0, 55
	
	mtc0 r0, COP_SP_DMA_DMEM
    mtc0 a1, COP_SP_DMA_DRAM
	mtc0 a2, COP_SP_DMA_WRITE
	
	// uses s5
	RSPDMASPWait()
	
	// halt the rsp
	addi t0, r0, 0x2
	mtc0 t0, COP_SP_STATUS
	
//	// back to beginning
//	b wait_for_packet
//	nop

	
// uses a0 for pixes
// a1 for hf
// touches modified a0,a1,a2,a3,v0,v1,r28,r29
flip:
	addu	v0,a0,r0
	beq	a1,r0,L7
	nop
	srl	a1,a0,8
	srl	v1,v0,16
	srl	a0,a0,24
	andi	s0,v0,0x00ff
	andi	s1,a1,0x00ff
	sll	a2,a0,4
	sll	v0,v0,4
	srl	a0,a0,4
	andi	a3,v1,0x00ff 
	sll	a1,a1,4
	srl	s1,s1,4
	srl	s0,s0,4
	or	a2,a2,a0
	or	a1,a1,s1
	sll	a0,v1,4
	or	v0,v0,s0
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


//align(8) // Align 64-Bit
//RDPBuffer:
//arch n64.rdp
////	Set_Other_Modes EN_TLUT|BI_LERP_0 // Set Other Modes
////	Set_Combine_Mode $0,$00, 0,0, $1,$01, $0,$F, 1,0, 0,0,0, 7,7,7 // Set Combine Mode: SubA RGB0,MulRGB0, SubA Alpha0,MulAlpha0, SubA RGB1,MulRGB1, SubB RGB0,SubB RGB1, SubA Alpha1,MulAlpha1, AddRGB0,SubB Alpha0,AddAlpha0, AddRGB1,SubB Alpha1,AddAlpha1
//	Sync_Tile
//set_texture_image:
//	db 0,0,0,0
//	Sync_Full
//RDPBufferEnd:

// __attribute__((aligned(8))) 	