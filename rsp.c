#include <stdint.h>
#include <stddef.h>

//#include "packet.h"
#include "rsp_code.h"
//#include "last_dma.h"
  
#define SP_MEM_ADDR_REG 0xA4040000
#define SP_DRAM_ADDR_REG 0xA4040004
#define SP_RD_LEN_REG 0xA4040008
#define SP_WR_LEN_REG 0xA404000C
#define SP_STATUS_REG 0xA4040010
#define SP_DMA_FULL_REG 0x04040014
#define SP_DMA_BUSY_REG 0x04040018
#define SP_DMA_SEM_REG 0x0404001C
#define SP_PC_REG 0x04080000 
    // MIPS addresses
    #define KSEG0 0x80000000
    #define KSEG1 0xA0000000

    // Memory translation stuff
    #define	PHYS_TO_K1(x)       ((uint32_t)(x)|KSEG1)
    #define	IO_WRITE(addr,data) (*(volatile uint32_t *)PHYS_TO_K1(addr)=(uint32_t)(data))
    #define	IO_READ(addr)       (*(volatile uint32_t *)PHYS_TO_K1(addr))
volatile uint32_t __attribute__((aligned(8))) datas[40*20][64];
static volatile uint32_t *datau;	
void copy_code_to_rsp() {
	/*volatile uint32_t **/datau = (volatile uint32_t*)((uint32_t)&datas[0] | 0xA0000000);

	for (int i=0;i<4096;i+=64) {
		while (IO_READ(SP_DMA_BUSY_REG) != 0)
		{
			// wait for DMA
		}

		IO_WRITE(SP_MEM_ADDR_REG, 0x1000 + i);
		IO_WRITE(SP_DRAM_ADDR_REG, (((uintptr_t)&rsp_code[0]) | 0xA0000000)+ i);
		IO_WRITE(SP_RD_LEN_REG, 63);
	}

	while(IO_READ(SP_DMA_BUSY_REG) != 0) {
		// wait
	}

	rsp_init();
	IO_WRITE(SP_PC_REG, 0);
	run_ucode();
}

extern void rsp_copy(void *dest, void *src, size_t n);

uint32_t rsp_sprite_data;

uint32_t rsp_addr0;
uint32_t rsp_pn0;
uint32_t rsp_vf0;
uint32_t rsp_hf0;

uint32_t rsp_addr1;
uint32_t rsp_pn1;
uint32_t rsp_vf1;
uint32_t rsp_hf1;

#define RSP_DRAM_MASK 0x07fffff8
void rsp_combine_tiles(int i, uint32_t pixoff, uint32_t pixoff2) {
	//volatile uint32_t *datau = (volatile uint32_t*)((uint32_t)&datas[0] | 0xA0000000);
	datau[0] = (uint32_t)datau & RSP_DRAM_MASK;
	datau[1] = (uint32_t)rsp_sprite_data & RSP_DRAM_MASK;
	datau[2] = (uint32_t)rsp_addr0 & RSP_DRAM_MASK;
	datau[3] = rsp_pn0;
	datau[4] = rsp_vf0;
	datau[5] = rsp_hf0;
	datau[6] = (uint32_t)rsp_addr1 & RSP_DRAM_MASK;
	datau[7] = rsp_pn1;
	datau[8] = rsp_vf1;
	datau[9] = rsp_hf1;
	datau[10] = (i%4);
	datau[11] = 0;
	datau[12] = pixoff;
	datau[13] = pixoff2;

	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
	IO_WRITE(SP_MEM_ADDR_REG, 0);
	IO_WRITE(SP_DRAM_ADDR_REG, datau);
	IO_WRITE(SP_RD_LEN_REG, 55);	

	IO_WRITE(SP_PC_REG, 0);
	IO_WRITE(SP_STATUS_REG, 0x101);

	while(!datau[11]) {}
}

#if 0
int do_tile(void* dest, void* src, int vf, int hf) {
volatile uint32_t *datau = (volatile uint32_t*)((uint32_t)&datas[0] | 0xA0000000);

/*volatile rsp_tile_packet_t __attribute__((aligned(8))) tile_command;
volatile rsp_tile_packet_t *tp = (rsp_tile_packet_t *)PHYS_TO_K1(&tile_command);

tp->dram_this_ptr = tp;
tp->giving_a_tile = 1;
tp->tile_finished = 0;
tp->was_tile_blank = 0;
tp->dram_source_ptr = src;//vdp_vram + tilenum;
tp->dram_dest_ptr = dest;//a_new_sprite->data|0xA0000000;
tp->vf = vf;
tp->hf = hf;
*/
datau[0] = //tp->dram_this_ptr = 
(uint32_t)(&datau[0]) & 0x07FFFFFF;
datau[1] = //tp->dram_source_ptr = 
(uint32_t)src & 0x07ffffff;//0xA0000000;//vdp_vram + tilenum;
datau[2] = //tp->dram_dest_ptr = 
(uint32_t)dest & 0x07ffffff;//0xA0000000;//a_new_sprite->data|0xA0000000;
datau[3] = //tp->vf = 
vf;
datau[4] = //tp->hf = 
hf;
datau[5] = //tp->was_tile_blank = 
0;
datau[6] = //tp->tile_finished = 
0;
datau[7] = //tp->giving_a_tile = 
1;

// (rsp_tile_packet_t *, dmem_index)
//rsp_send_tile_packet(&tile_command, 0);

//disable_interrupts();
	//while(IO_READ(SP_DMA_SEM_REG) != 0) {}		// wait for DMA semaphore before anything

//	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
	IO_WRITE(SP_MEM_ADDR_REG, 0);
	IO_WRITE(SP_DRAM_ADDR_REG, &datau[0]);//& 0x07FFFFFF));
	IO_WRITE(SP_RD_LEN_REG, 31);	
	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA

	// last run of this task halts the rsp, so here we restart it
	IO_WRITE(SP_STATUS_REG, 0x101);
/*IO_WRITE(0xA4000000, datau[0]);
IO_WRITE(0xA4000004, datau[1]);
IO_WRITE(0xA4000008, datau[2]);
IO_WRITE(0xA400000C, datau[3]);
IO_WRITE(0xA4000010, datau[4]);
IO_WRITE(0xA4000014, datau[5]);
IO_WRITE(0xA4000018, datau[6]);
IO_WRITE(0xA400001C, datau[7]);*/

//	void rsp_copy(void *dest, void *src, size_t n)
//rsp_copy(datau[2], datau[1], 64);
	
//	for(int i=0;i<256;i++) {
//		if(datau[6]) break;
//		else continue;
//	}
	
//	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
	
	//IO_WRITE(SP_DMA_SEM_REG, 0);

//enable_interrupts();	
#if 0
volatile uint32_t gotresult = 0;

while(!gotresult) {
	//	while(IO_READ(SP_DMA_SEM_REG) != 0) {}		// wait for DMA semaphore before anything
gotresult = datau[6];
	//IO_WRITE(SP_DMA_SEM_REG, 0);
}
#endif
while(!datau[6]) {} // && datau[7]) {}
//while(1){}
return datau[5];
}

#endif