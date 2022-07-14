arch n64.cpu
endian msb
output "last_dma", create
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

	// uses s5
	RSPDMASPWait()

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
	
align(8) // Align 64-Bit
base RSPCode+pc() // Set End Of RSP Code Object
RSPCodeEnd: