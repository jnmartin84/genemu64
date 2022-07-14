/* $Id: vdp_c,v 1.18 2002-08-30 20:23:20 crt0 Exp $ */


#include <libdragon.h>
#include <stdio.h>
#include <stdlib.h>
#include "gen-emu.h"
//#include "vdp.h"
#include "m68k.h"
#include "z80.h"
//#include "h"

#include "packet.h"

//#define NORDP

#define DOSTATS 0

#define MULTITEX 0

#define high 0

extern uint8_t		*rom;
extern uint32_t	rom_len;
extern uint8_t		*sram;
extern uint32_t	sram_len;
extern uint32_t	sram_start;
extern uint32_t	sram_end;
extern uint32_t	banks[8];
extern uint8_t		banked;
extern uint8_t		sram_banked;


extern int rect_per_frame;
extern int wb_per_frame;
extern int texload_per_frame;

/*
typedef struct rsp_tile_packet_s {
	void*	dram_this_ptr;
	void* sprite_data_ptr;
	
	void*	tile0_ptr;
	uint32_t pn0;
	uint32_t vf0;
	uint32_t hf0;

	void*	tile1_ptr;
	uint32_t pn1;
	uint32_t vf1;
	uint32_t hf1;

	uint32_t new_request;
	uint32_t finished;
	
} rsp_tile_packet_t;
*/
//rsp_packet_tile_t __attribute__((aligned(8))) background[40*28];


uint8_t *vdp_vram;
static uint8_t __attribute__((aligned(8))) vdp_vram2[65536];
uint16_t *vdp_cram;
static uint16_t __attribute__((aligned(8))) vdp_cram2[64];
uint16_t *vdp_vsram;
static uint16_t __attribute__((aligned(8))) vdp_vsram2[64];		/* Only first 40 used. rest are address padding. */
uint16_t *vdp_dc_cram;
uint16_t __attribute__((aligned(8))) vdp_dc_cram2[64];	/* cram in dc format */
uint8_t *vdp_regs;
uint8_t __attribute__((aligned(8))) vdp_regs2[32];		/* Only first 24 used, rest are address padding. */

static uint32_t vdp_control;
static uint16_t *vdp_bga;
static uint16_t *vdp_bgb;
static uint16_t *vdp_wnd;
static uint64_t *vdp_sat;

static uint16_t vdp_status;
static uint16_t vdp_scanline;
static uint16_t vdp_hv;

static uint16_t vdp_hs_off;
uint16_t vdp_addr;
static uint8_t vdp_code;
static uint8_t vdp_h_int_counter;
static uint8_t vdp_write_pending;
static uint8_t vdp_sc_width;
static uint8_t vdp_sc_height;
static uint8_t vdp_dis_cells;

extern uint32_t rsp_sprite_data;

extern uint32_t rsp_addr0;
extern uint32_t rsp_pn0;
extern uint32_t rsp_vf0;
extern uint32_t rsp_hf0;

extern uint32_t rsp_addr1;
extern uint32_t rsp_pn1;
extern uint32_t rsp_vf1;
extern uint32_t rsp_hf1;


extern display_context_t _dc;
//sprite_t *paltile[64];
display_context_t lockVideo(int wait)
{
	display_context_t dc;

	if (wait)
	{
		while (!(dc = display_lock()));
	}
	else
	{
		dc = display_lock();
	}

	return dc;
}

void unlockVideo(display_context_t dc)
{
	if (dc)
	{
		display_show(dc);
	}
}

//volatile int start_printing=0;

#define vdpr ((val&0x000f)<<4)
#define vdpg ((val&0x00f0)  )
#define vdpb ((val&0x0f00)>>4)	
extern void __n64_memset_ASM(void *dest, const int val, int count);
extern void __n64_memcpy_ASM(void *dest, const void *src, int count);

  
#define SP_MEM_ADDR_REG 0xA4040000
#define SP_DRAM_ADDR_REG 0xA4040004
#define SP_RD_LEN_REG 0xA4040008
#define SP_WR_LEN_REG 0xA404000C
#define SP_STATUS_REG 0xA4040010
#define SP_DMA_FULL_REG 0x04040014
#define SP_DMA_BUSY_REG 0x04040018
#define SP_DMA_SEM_REG 0x0404001C
    // MIPS addresses
    #define KSEG0 0x80000000
    #define KSEG1 0xA0000000

    // Memory translation stuff
    #define	PHYS_TO_K1(x)       ((uint32_t)(x)|KSEG1)
    #define	IO_WRITE(addr,data) (*(volatile uint32_t *)PHYS_TO_K1(addr)=(uint32_t)(data))
    #define	IO_READ(addr)       (*(volatile uint32_t *)PHYS_TO_K1(addr))
void rsp_tile_copy(void *dest, void *src)
{
#if 0
	uint32_t regval_s = 31;
	uint32_t regval_d = (4 << 20) | (7<<12) | 7;
//	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
//	while(IO_READ(SP_DMA_FULL_REG) != 0) {}

	IO_WRITE(SP_MEM_ADDR_REG, 0);
	IO_WRITE(SP_DRAM_ADDR_REG, src | 0x07FFFFFF);
	IO_WRITE(SP_RD_LEN_REG, regval_s);

	IO_WRITE(SP_MEM_ADDR_REG, 0);
	IO_WRITE(SP_DRAM_ADDR_REG, dest | 0x07FFFFFF);
	IO_WRITE(SP_WR_LEN_REG, regval_d);
		
	while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
#endif
}
static inline void D_memcpy(void *dest, const void *src, int count) // 80001ACC
{
	uint8_t	*d, *s;
	int		*ld, *ls;
	int     stopcnt;

	ld = (int *)dest;
	ls = (int *)src;

	if ((((int)ld | (int)ls | count) & 7))
    {
        d = (uint8_t *)dest;
        s = (uint8_t *)src;
        while (count--)
            *d++ = *s++;
    }
    else
    {
        if (count == 0)
            return;

        if(-(count & 31))
        {
            stopcnt = -(count & 31) + count;
            while (stopcnt != count)
            {
                ld[0] = ls[0];
                ld[1] = ls[1];
                ld += 2;
                ls += 2;
                count -= 8;
            }

            if (count == 0)
                return;
        }

        while (count)
        {
            ld[0] = ls[0];
            ld[1] = ls[1];
            ld[2] = ls[2];
            ld[3] = ls[3];
            ld[4] = ls[4];
            ld[5] = ls[5];
            ld[6] = ls[6];
            ld[7] = ls[7];
            ld += 8;
            ls += 8;
            count -= 32;
        }
    }
}	

#if 1
static inline void rsp_copy(void *dest, void *src, size_t n)
{
	if(1 && 	!((((int)dest | (int)src | n) & 7)) && n>16) {

//	!((uint32_t)dest&7) && !((uint32_t)src&7) && !((uint32_t)n&7) && (n>16)) {
#if 1
	int rsp_block_size;

	if (n > 64) rsp_block_size = 64;
	else rsp_block_size = n;
	
	unsigned long blocks = n / (rsp_block_size);
	
//	int destisoff = dest & 7;
	
//	while(IO_READ(SP_DMA_SEM_REG) != 0) {}		// wait for DMA semaphore before anything

	for(int i=0;i<blocks;i++)
	{
		uint32_t regval = (/*(((blocks-1)&0x000000FF) << 12) | */((rsp_block_size-1)& 0x000003FF)) & 0x000FFFFF;
		while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
//		while(IO_READ(SP_DMA_FULL_REG) != 0) {}

// critical section stuff here

		
		IO_WRITE(SP_MEM_ADDR_REG, 2048);
		IO_WRITE(SP_DRAM_ADDR_REG, ((uintptr_t)src | 0xA0000000) + (i*rsp_block_size));
		IO_WRITE(SP_RD_LEN_REG, regval);
		
		// wait for DMA
//		while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		
		
	//	while(IO_READ(SP_DMA_FULL_REG) != 0) {}

		IO_WRITE(SP_MEM_ADDR_REG, 2048);
		IO_WRITE(SP_DRAM_ADDR_REG, ((uintptr_t)dest  | 0xA0000000) + (i*rsp_block_size));// + (rsp_block_size));
		IO_WRITE(SP_WR_LEN_REG, regval);
		
		while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
		
//		src += rsp_block_size;
//		dest += rsp_block_size;
	}

//	IO_WRITE(SP_DMA_SEM_REG, 0); // clear semaphore
	
//		int leftover = n % rsp_block_size;//- (blocks*(rsp_block_size));
//	__n64_memcpy_ASM(dest+(blocks*rsp_block_size), src+(blocks*rsp_block_size), leftover);

		#endif
	}
	else
	{
#if 1
	__n64_memcpy_ASM(dest, src, n);
#endif
	}
}
#endif
//#define rsp_copy D_memcpy


#if 0
#define WORDMASK 7
static inline void D_memset(void *dest, const int val, int count) // 80001A20
{
	uint8_t	*p;
	int		*lp;
	int     v;

	/* round up to nearest word */
	p = dest;
	while ((int)p & WORDMASK)
	{
		if (--count < 0)
            return;
		*p++ = (char)val;
	}

	/* write 8 bytes at a time */
	lp = (int *)p;
	v = (int)(val << 24) | (val << 16) | (val << 8) | val;
	while (count >= 8)
	{
		lp[0] = lp[1] = v;
		lp += 2;
		count -= 8;
	}

	/* finish up */
	p = (uint8_t *)lp;
	while (count--)
		*p++ = (char)val;
}
static inline void D_memcpy(void *dest, const void *src, int count) // 80001ACC
{
	uint8_t	*d, *s;
	int		*ld, *ls;
	int     stopcnt;

	ld = (int *)dest;
	ls = (int *)src;

	if ((((int)ld | (int)ls | count) & 7))
    {
        d = (uint8_t *)dest;
        s = (uint8_t *)src;
        while (count--)
            *d++ = *s++;
    }
    else
    {
        if (count == 0)
            return;

        if(-(count & 31))
        {
            stopcnt = -(count & 31) + count;
            while (stopcnt != count)
            {
                ld[0] = ls[0];
                ld[1] = ls[1];
                ld += 2;
                ls += 2;
                count -= 8;
            }

            if (count == 0)
                return;
        }

        while (count)
        {
            ld[0] = ls[0];
            ld[1] = ls[1];
            ld[2] = ls[2];
            ld[3] = ls[3];
            ld[4] = ls[4];
            ld[5] = ls[5];
            ld[6] = ls[6];
            ld[7] = ls[7];
            ld += 8;
            ls += 8;
            count -= 32;
        }
    }
}
#endif
#if 1
#define D_memcpy __n64_memcpy_ASM
#define D_memset __n64_memset_ASM
#endif

extern uint16_t *m68k_ram16;
uint16_t *vram16;

//struct vdp_s vdp;
uint8_t display_ptr;

uint8_t nt_cells[4] = { 32, 64, 0, 128 };
uint8_t mode_cells[4] = { 32, 40, 0, 40 };

extern void *__safe_buffer[];
display_context_t _dc;

uint16_t *ocr_vram;
static uint8_t __attribute__((aligned(8))) tile_data[sizeof(sprite_t) + (32*8)];
static uint8_t __attribute__((aligned(8))) tile_data_b[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_c[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_d[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_e[sizeof(sprite_t) + (32*32)];

static sprite_t *vdp_sprite_rdp_sprite = NULL;

static sprite_t *tile0_sprite = NULL;
static sprite_t *tile1_sprite = NULL;
static sprite_t *tile2_sprite = NULL;
static sprite_t *tile3_sprite = NULL;

unsigned char *vramp;// = (uint32_t)vdp->vram;

sprite_t *tiles[4];

extern void copy_code_to_rsp();
extern int do_tile(void* dest, void* src, int vf, int hf);
#define SP_PC_REG 0x04080000 

void vdp_init() 
{
	copy_code_to_rsp();
	vdp_vram = (uint8_t*)((uintptr_t)vdp_vram2 | 0xA0000000);
	vdp_cram = (uint16_t*)((uintptr_t)vdp_cram2 | 0xA0000000);
	vdp_vsram=(uint16_t*)((uintptr_t)vdp_vsram2 | 0xA0000000);
	vdp_regs=(uint8_t*)((uintptr_t)vdp_regs2 | 0xA0000000);
	vdp_dc_cram = (uint16_t *)((uintptr_t)vdp_dc_cram2 | 0xA0000000);
	
	tile0_sprite = (sprite_t *)(&tile_data[0]); 
	tile0_sprite->bitdepth = 1;
	tile0_sprite->vslices = 1;
	tile0_sprite->hslices = 1;
	tile0_sprite->width = 32;
	tile0_sprite->height = 8;
	
	tile1_sprite = (sprite_t *)(&tile_data_c[0]); 
	tile1_sprite->bitdepth = 1;
	tile1_sprite->vslices = 1;
	tile1_sprite->hslices = 1;
	tile1_sprite->width = 8;
	tile1_sprite->height = 8;
	
	tile2_sprite = (sprite_t *)(&tile_data_d[0]); 
	tile2_sprite->bitdepth = 1;
	tile2_sprite->vslices = 1;
	tile2_sprite->hslices = 1;
	tile2_sprite->width = 8;
	tile2_sprite->height = 8;

	tile3_sprite = (sprite_t *)(&tile_data_e[0]); 
	tile3_sprite->bitdepth = 1;
	tile3_sprite->vslices = 1;
	tile3_sprite->hslices = 1;
	tile3_sprite->width = 8;
	tile3_sprite->height = 8;

//	tiles[0] = (sprite_t *)(&tile_data[0]);
//	tiles[1] = (sprite_t *)(&tile_data_c[0]);
//	tiles[2] = (sprite_t *)(&tile_data_d[0]);
//	tiles[3] = (sprite_t *)(&tile_data_e[0]);
	
	vdp_sprite_rdp_sprite = (sprite_t *)(&tile_data_b[0]); 
	vdp_sprite_rdp_sprite->bitdepth = 1;
	vdp_sprite_rdp_sprite->vslices = 1;
	vdp_sprite_rdp_sprite->hslices = 1;
	vdp_sprite_rdp_sprite->width = 8;
	vdp_sprite_rdp_sprite->height = 8;
}


uint16_t vdp_control_read(void)
{
//	uint16_t ret = 0x3500;

	vdp_write_pending = 0;

	m68k_set_irq(0);

//	ret |= (vdp_status & 0x00ff);

//	return ret;
	return 0x3500 | (vdp_status&0xff);
}

/* val is big endian */
void vdp_control_write(uint16_t val)
{
	if((val & 0xc000) == 0x8000)
	{
		if(!vdp_write_pending)
		{
			vdp_regs[(val >> 8) & 0x1f] = (val & 0x000000ff);
			switch((val >> 8) & 0x1f)
			{
			case 0x02:	/* BGA */
				vdp_bga = (uint16_t *)(vdp_vram + ((val & 0x38) << 10));
				break;
			case 0x03:	/* WND */
				vdp_wnd = (uint16_t *)(vdp_vram + ((val & 0x3e) << 10));
				break;
			case 0x04:	/* BGB */
				vdp_bgb = (uint16_t *)(vdp_vram + ((val & 0x07) << 13));
				break;
			case 0x05:	/* SAT */
				vdp_sat = (uint64_t *)(vdp_vram + ((val & 0x7f) << 9));
				break;
			case 0x0c:	/* Mode Set #4 */
				vdp_dis_cells = mode_cells[((vdp_regs[12] & 0x01) << 1) |
											(vdp_regs[12] >> 7)];
				break;
			case 0x0d:
				vdp_hs_off = vdp_regs[13] << 10;
				break;
			case 0x10:	/* Scroll size */
				vdp_sc_width = nt_cells[vdp_regs[16] & 0x03];
				vdp_sc_height = nt_cells[(vdp_regs[16] & 0x30) >> 4];
				break;
			}
			vdp_code = 0x0000;
		}
	}
	else
	{
		if (!vdp_write_pending)
		{
			vdp_write_pending = 1;
			vdp_addr = ((vdp_addr & 0xc000) | (val & 0x3fff));
			vdp_code = ((vdp_code & 0x3c) | ((val & 0xc000) >> 14));
		}
		else
		{
			vdp_write_pending = 0;
			vdp_addr = ((vdp_addr & 0x3fff) | ((val & 0x0003) << 14));
			vdp_code = ((vdp_code & 0x03) | ((val & 0x00f0) >> 2));

			if ((vdp_code & 0x20) && (vdp_regs[1] & 0x10))
			{
				/* dma operation */
				if (((vdp_code & 0x30) == 0x20) && !(vdp_regs[23] & 0x80))
				{
					uint16_t len = (vdp_regs[20] << 8) |  vdp_regs[19];
					uint16_t src_off = (vdp_regs[22] << 8) | vdp_regs[21];
					uint16_t *src_mem, src_mask;

					if ((vdp_regs[23] & 0x70) == 0x70)
					{
						src_mem = m68k_ram16;
						src_mask = 0x7fff;
					}
					else if ((vdp_regs[23] & 0x70) < 0x20)
					{
						src_mem = (uint16_t *)(rom + (vdp_regs[23] << 17));
						src_mask = 0xffff;
					}
					else
					{
						//printf("DMA from an unknown block... 0x%02x\n", (vdp_regs[23] << 1));
						while(1);
					}

					/* 68k -> vdp */
					switch(vdp_code & 0x07)
					{
					case 0x01:
/*						if(((vdp_addr & 7) == 0) && (vdp_regs[15] == 2) && ((src_off & src_mask & 7) == 0) && len > 8 && (((len<<1)&7)==0))
						{
							// THIS ONLY VERIFIED TO WORK FOR SONIC 2 (and only for big endian host)
							//D_memcpy(vdp_vram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							rsp_copy(
							vdp_vram + vdp_addr, 
							src_mem + (src_off & src_mask), 
							len<<1);
							
							vdp_addr += len<<1;
							src_off += len;
						}*/
						//else 
						if (vdp_regs[15] == 2) {
							//D_memcpy
							rsp_copy(vdp_vram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							vdp_addr += len<<1;
							src_off += len;
						}
						break;
					case 0x03:
						/* cram */
						if(/*((vdp_addr & 3) == 0) &&*/ (vdp_regs[15] == 2)/* && ((src_off & 1) == 0)*/)
						{
							//if(((vdp_addr & 7) == 0) && ((src_off & src_mask & 7) == 0) &&  len > 8 && (((len<<1)&7)==0)) {
							//rsp_copy(vdp_cram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							//}
							//else { 
							//D_memcpy
							rsp_copy(vdp_cram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							//}
							vdp_addr += len<<1;
							src_off += len;
							uint32_t* start_cramp = (uint32_t *)vdp_dc_cram;
							for(int i=0;i<0x40;i+=2)
							{
								val = vdp_cram[i];
								uint32_t t1 = graphics_make_color(
									vdpr&0xff,
									vdpg&0xff,
									vdpb&0xff,
									((i % 16) == 0) ? 0x00 : 0xFF
								) & 0xFFFF0000;
								val = vdp_cram[i+1];
								uint32_t t2 = graphics_make_color(
									vdpr&0xff,
									vdpg&0xff,
									vdpb&0xff,
									((i+1 % 16) == 0) ? 0x00 : 0xFF
								) & 0x0000FFFF;
								*start_cramp++ = (t1 | t2);
							}
						}
						else
						{
							while(--len)
							{
								uint32_t sh_addr = vdp_addr >> 1;
								val = src_mem[src_off & src_mask];
								vdp_cram[sh_addr] = val;
								vdp_dc_cram[sh_addr] = graphics_make_color(
									vdpr&0xff,
									vdpg&0xff,
									vdpb&0xff,
									((sh_addr % 16) == 0) ? 0x00 : 0xFF
								);
								vdp_addr += vdp_regs[15];
								src_off += 1;

								if(vdp_addr > 0x7f)
								{
									break;
								}
							}
						}
						
//						wb_per_frame+=128;
//						data_cache_hit_writeback_invalidate(vdp_dc_cram, 64*2);
#ifndef NORDP
						rdp_load_palette(0,64,vdp_dc_cram);
						
/*						while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
						IO_WRITE(SP_MEM_ADDR_REG, 1024);
						IO_WRITE(SP_DRAM_ADDR_REG, vdp_dc_cram);
						IO_WRITE(SP_RD_LEN_REG, (1<<12) | 63);	

						while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA*/
#endif
						break;
					case 0x05:
/*						if(((vdp_addr & 7) == 0) && (vdp_regs[15] == 2) && ((src_off & src_mask & 7) == 0) && len > 8 && (((len<<1)&7)==0))
						{
							//D_memcpy(vdp_vsram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							rsp_copy(vdp_vsram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							vdp_addr += len<<1;
							src_off += len;
						}*/
						//else 
							if(/*((vdp_addr & 3) == 0) &&*/ (vdp_regs[15] == 2) /*&& ((src_off & 1) == 0)*/)
						{
							//D_memcpy(vdp_vsram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							rsp_copy(vdp_vsram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							vdp_addr += len<<1;
							src_off += len;
						}
						else
						{
							/* vsram */
							while(--len)
							{
								val = src_mem[src_off & src_mask];
								vdp_vsram[vdp_addr >> 1] = val;
								vdp_addr += vdp_regs[15];
								src_off += 1;

								if(vdp_addr > 0x7f)
								{
									break;
								}
							}
						}
						break;
					default:
						//printf("68K->VDP DMA Error, code %d.", vdp_code);
						break;
					}

					vdp_regs[19] = vdp_regs[20] = 0;
					vdp_regs[21] = src_off & 0xff;
					vdp_regs[22] = (src_off >> 8) & 0xFF;;
				}
			}
		}
	}
}

uint16_t vdp_data_read(void)
{
//	uint16_t ret = 0x0000;

	vdp_write_pending = 0;

uint16_t *p;

 	switch(vdp_code)
	{
	case 0x00:
		p = (uint16_t*)vdp_vram;//ret = ((uint16_t *)vdp_vram)[vdp_addr >> 1];
		break;
	case 0x04:
		p = vdp_vsram;//ret = vdp_vsram[vdp_addr >> 1];
		break;
	case 0x08:
		p = vdp_cram; //ret = vdp_cram[vdp_addr >> 1];
		break;
	}

//	if (debug)
//		printf("VDP D -> %04x\n", ret);
	vdp_addr += 2;

	return p[vdp_addr>>1];//ret;
}

void vdp_data_write(uint16_t val)
{
	vdp_write_pending = 0;

//	if (debug)
//		printf("VDP D <- %04x\n", val);

	switch(vdp_code)
	{
	case 0x01:
	{
		//if (vdp_addr & 0x01)
		//	val = SWAPBYTES16(val);

		((uint16_t *)vdp_vram)[vdp_addr >> 1] = val;

		vdp_addr += 2;

		break;
	}
	case 0x03:
	{
		vdp_cram[(vdp_addr >> 1) & 0x3f] = val;
		vdp_dc_cram[(vdp_addr >> 1) & 0x3f] = 
			graphics_make_color(
				vdpr&0xff,
				vdpg&0xff,
				vdpb&0xff,
				(((vdp_addr >> 1) & 0x3F) % 16) == 0 ? 0x00 : 0xFF
			);

//		wb_per_frame+=128;
//		data_cache_hit_writeback_invalidate(vdp_dc_cram, 64*2);
#ifndef NORDP
		rdp_load_palette(0,64,vdp_dc_cram);
/*		while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA
		IO_WRITE(SP_MEM_ADDR_REG, 1024);
		IO_WRITE(SP_DRAM_ADDR_REG, vdp_dc_cram);//& 0x07FFFFFF));
		IO_WRITE(SP_RD_LEN_REG, (1<<12) | 63);	

		while(IO_READ(SP_DMA_BUSY_REG) != 0) {}		// wait for DMA*/
#endif

		vdp_addr += 2;
		break;
	}
	case 0x05:
	{
		vdp_vsram[(vdp_addr >> 1) & 0x3f] = val;

		vdp_addr += 2;
		break;
	}
	default:
	{
		if ((vdp_code & 0x20) && (vdp_regs[1] & 0x10))
		{
			if (((vdp_code & 0x30) == 0x20) && ((vdp_regs[23] & 0xc0) == 0x80))
			{
				/* vdp fill */
				uint16_t len = ((vdp_regs[20] << 8) | vdp_regs[19]);
				vdp_vram[vdp_addr] = val & 0xff;
				val = (val >> 8) & 0xff;

				//D_memset(vdp_vram + vdp_addr, val, len);
				
				while(--len)
				{
					vdp_vram[vdp_addr ^ 1] = val;
					vdp_addr += vdp_regs[15];
				}
				vdp_addr += vdp_regs[15]*len;
			}
			else if ((vdp_code == 0x30) && ((vdp_regs[23] & 0xc0) == 0xc0))
			{
				/* vdp copy */
				uint16_t len = (vdp_regs[20] << 8) | vdp_regs[19];
				uint16_t addr = (vdp_regs[22] << 8) | vdp_regs[21];

				while(--len)
				{
					vdp_vram[vdp_addr] = vdp_vram[addr++];
					vdp_addr += vdp_regs[15];
				}
				//D_memcpy(vdp_vram + vdp_addr, vdp_vram + addr, len);
				//vdp_addr += vdp_regs[15]*len;
			}
			else
			{
				//printf("VDP DMA Error, code %02x, r23 %02x\n", vdp_code, vdp_regs[23]);
			}
		}
	}
	}
}

uint16_t vdp_hv_read(void)
{
	uint16_t h, v;

	v = vdp_scanline;
	if ( v > 0xea)
	{
		v -= 6;
	}

	h = (uint16_t)((m68k_cycles_run() * 70082) / 100000); // * 0.70082f);
	if (h > 0xe9)
	{
		h -= 86;
	}

	return(((v & 0xff) << 8) | (h & 0xff));
}

void vdp_interrupt(int line)
{
	uint8_t h_int_pending = 0;

	vdp_scanline = line;

	if (vdp_h_int_counter == 0)
	{
		vdp_h_int_counter = vdp_regs[10];
		if (vdp_regs[0] & 0x10)
		{
			h_int_pending = 1;
		}
	}

	if (line < 224)
	{
		if (line == 0)
		{
			vdp_h_int_counter = vdp_regs[10];
			vdp_status &= 0x0001;
		}

		if (h_int_pending)
		{
			m68k_set_irq(4);
		}
	}
	else if (line == 224)
	{
		z80_set_irq_line(0, PULSE_LINE);
		if (h_int_pending)
		{
			m68k_set_irq(4);
		}
		else
		{
			if (vdp_regs[1] & 0x20)
			{
				vdp_status |= 0x0080;
				m68k_set_irq(6);
			}
		}
	}
	else
	{
		vdp_h_int_counter = vdp_regs[10];
		vdp_status |= 0x08;
	}

	vdp_h_int_counter--;
}

#define spr_start (vdp_vram + sn)
int list_ordered[2][80] = {-1}; 

void render_all_sprites(/*int unused_priority*/) {
	uint32_t spr_ent_bot,spr_ent_top;
	uint32_t c=0, cells=64, i=0, j, k, h,v, sp, sl=0, sh, sv, sn, /*sc,*/ shf, svf;
	uint32_t dis_line=16;
	uint32_t ppl=0;
	uint32_t dis_ppl=256;
	uint32_t sol=0;
	sint32_t sx, sy;
	//uint64_t spr_ent;
	uint32_t last_sn = 0xFFFFFFFF;
	uint32_t last_sh = 0xFFFFFFFF;
	uint32_t last_sv = 0xFFFFFFFF;
	uint32_t last_svf = 0xFFFFFFFF;
	uint32_t last_shf = 0xFFFFFFFF;
	uint8_t pixel;
	uint16_t *pal;
	uint32_t data;
	uint32_t sx_plus_shifted_h;

	uint32_t *dp2 = (uint32_t *)((uintptr_t)vdp_sprite_rdp_sprite->data|0xA0000000);
	//rdp_load_palette(0,64,vdp_dc_cram);

	if (!(vdp_dis_cells == 32))
	{
		cells = 80;
		dis_line=20;
		dis_ppl=320;
	}

	vdp_status &= 0x0040; // not too sure about this...
	
	for(int priority=0;priority<2;priority++)
	{
//		int priority = 0;
	sl = 0;
		sp = 0;
		c = 0;
		for(i=0;i<cells;++i)
		{
			uint64_t spr_ent = vdp_sat[c];

			spr_ent_top = (spr_ent >> 32);
			spr_ent_bot = (spr_ent & 0x00000000ffffffff);

			sp = (spr_ent_bot & 0x80000000) >> 31;

			if(sp == priority)
			{
				list_ordered[priority][( 79 - ( (cells==64)?16:0 ) - i )]=c;
			}

			sl = (spr_ent_top & 0x0000007F);
			if(sl)
			{
				c = sl;
			}
			else
			{
				break;
			}
		}
	}
	for(int priority=0;priority<2;priority++)
	{
//		int priority = 0;
	for(i=0;i<cells;i++)
	{
		int next_index = list_ordered[priority][i];
		if( next_index > -1 )
		{
			list_ordered[priority][i] = -1;
			uint64_t spr_ent = vdp_sat[next_index];
			spr_ent_top = (spr_ent >> 32);
			spr_ent_bot = (spr_ent & 0x00000000ffffffff);

			sp = (spr_ent_bot & 0x80000000) >> 31;
//			if(sp == priority)
//			{
				sh  = ((spr_ent_top & 0x00000C00) >> 10) + 1;
				sv  = ((spr_ent_top & 0x00000300) >>  8) + 1;
				svf = ((spr_ent_bot & 0x10000000) >> 28);
				shf = ((spr_ent_bot & 0x08000000) >> 27);
				sn  = ((spr_ent_bot & 0x07FF0000) >> 11);
				sy  = ((spr_ent_top & 0x03FF0000) >> 16) - 128;
				if(sy < 0 || sy > 224) continue;
				sx  = ((spr_ent_bot & 0x000003FF)) - 128;
				if(sx < 0 || sx > 320) continue;
				vdp_sprite_rdp_sprite->width = sh<<3;
				vdp_sprite_rdp_sprite->height = sv<<3;
			
				if(last_sn != sn || last_shf != shf || last_svf != svf || last_sv != sv || last_sh != sh) 
				{
					uint8_t palnum = ((spr_ent_bot & 0x60000000) >> 25);

					for(v = 0; v < sv; v++)
					{
						for(h = 0; h < sh; h++)
						{
							uint8_t *tp;
							if(svf && !shf)
							{
								tp = (uint8_t*)(spr_start  + (((sv*h)+(sv-v-1))<<5));
							}
							if(shf && !svf)
							{
								tp = (uint8_t*)(spr_start  + (((sv*(sh-h-1))+(v))<<5));
							}
							else if(svf && shf)
							{
								tp = (uint8_t*)(spr_start  + (((sv*(sh-h-1))+(sv-v-1))<<5));
							}
							else
							{
								tp = (uint8_t *)(spr_start  + (((sv*h)+v)<<5));
							}

							for(int nny=0;nny<8;nny++)
							{
								uint32_t *tp2;
								if(!svf)
								{
									tp2 = (uint32_t *)(tp + (nny<<2));	
								}
								else
								{
									tp2 = (uint32_t *)(tp + ((7-nny)<<2));
								}

								uint32_t eight_pixels = *tp2;
								uint32_t half_one_pixels, half_two_pixels;
								uint8_t p1,p2,p3,p4;
								p1 = ((eight_pixels >> 28)&0xf) | palnum;
								p2 = ((eight_pixels >> 24)&0xf) | palnum;
								p3 = ((eight_pixels >> 20)&0xf) | palnum;
								p4 = ((eight_pixels >> 16)&0xf) | palnum;
		
								if(!shf)
								{
									half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
								}
								else
								{
									half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
								}
								p1 = ((eight_pixels >> 12)&0xf) | palnum;
								p2 = ((eight_pixels >> 8)&0xf) | palnum;
								p3 = ((eight_pixels >> 4)&0xf) | palnum;
								p4 = ((eight_pixels >> 0)&0xf) | palnum;
								
								if(!shf)
								{
									half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
								}
								else
								{
									half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
								}

								if(!shf)
								{
									dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)] = half_one_pixels;
									dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)+1] = half_two_pixels;
								}
								else
								{
									dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)] = half_two_pixels;
									dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)+1] = half_one_pixels;
								}
							}
						}
					}
					last_sn = sn;
					last_shf = shf;
					last_svf = svf;
					last_sv = sv;
					last_sh = sh;

#ifndef NORDP
//					wb_per_frame += sv*sh*64;
//					data_cache_hit_writeback_invalidate(vdp_sprite_rdp_sprite->data, 64*vdp_sprite_rdp_sprite->width * vdp_sprite_rdp_sprite->height);
#if DOSTATS
					texload_per_frame++;
#endif
#if MULTITEX
					rdp_load_texture(vdp_sprite_rdp_sprite,0);
#else
					rdp_load_texture(vdp_sprite_rdp_sprite,0);
#endif
#endif
				}
				
#ifndef NORDP
#if DOSTATS
				rect_per_frame++;
#endif
				rdp_draw_textured_rectangle_scaled(
#if MULTITEX
					0,
#else
					0,
#endif
					sx, 
					sy, 
					sx + ((sh<<3)-1), 
					sy + ((sv<<3)-1), 
					1.0, 
					1.0, 
					0
				);
#endif
		}
	}
	}
}
extern void rsp_combine_tiles(int,int, int);
uint8_t __attribute__((aligned(8))) zeros[32] = {0};
#define tileskippix 16
void render_a_whole_plane(/*int priority*/) {
	int			row, pixrow, i, j, row2, pixrow2;
	sint16_t	hscroll2 = 0;
	sint16_t	hscroll = 0;
	sint8_t		col_off, pix_off, pix_tmp, 
				col_off2, pix_off2, pix_tmp2;
	int			line,line2;
	int			hf,vf;
	int			last_tilenum = -1;
	int			last_tilenum2 = -1;
	int last_tilenum3 = -1; int last_tilenum4 = -1;
	int			last_blank = 1;
	int last_blank2 = 1;
	int last_blank3=1,last_blank4=1;
	int			last_hf = -1;
	int			last_vf = -1;
	int last_hf2 = -1;
	int last_vf2 = -1;
int last_combined=0,last_combined2=0;
	int iii = 0;
	int draw,draw2,draw3,draw4;
uint8_t palnum,palnum2,palnum3,palnum4;
	for(int y=0;y<224;y+=tileskippix) //8
	{
		line = y;
		line2 = y;
		
		switch(vdp_regs[11] & 0x03) {
		case 0x0:
			hscroll = ((uint16_t *)vdp_vram)[(vdp_hs_off + 2) >> 1];
			hscroll2 = ((uint16_t *)vdp_vram)[(vdp_hs_off + 0) >> 1];
			break;
		case 0x1:
			hscroll = ((uint16_t *)vdp_vram)[(vdp_hs_off + ((line & 0x7) << 1) + 2) >> 1];
			hscroll2 = ((uint16_t *)vdp_vram)[(vdp_hs_off + ((line2 & 0x7) << 1) + 0) >> 1];
			break;
		case 0x2:
			hscroll = ((uint16_t *)vdp_vram)[(vdp_hs_off + ((line & ~0x7) << 1) + 2) >> 1];
			hscroll2 = ((uint16_t *)vdp_vram)[(vdp_hs_off + ((line2 & ~0x7) << 1) + 0) >> 1];
			break;
		case 0x3:
			hscroll = ((uint16_t *)vdp_vram)[(vdp_hs_off + (line << 2) + 2) >> 1];
			hscroll2 = ((uint16_t *)vdp_vram)[(vdp_hs_off + (line2 << 2) + 0) >> 1];
			break;
		}

		hscroll = (0x400 - hscroll) & 0x3ff;
		col_off = hscroll >> 3;
		pix_off = hscroll & 0x7;
		hscroll2 = (0x400 - hscroll2) & 0x3ff;
		col_off2 = hscroll2 >> 3;
		pix_off2 = hscroll2 & 0x7;
	
		if ((vdp_regs[11] & 0x04) == 0) {
			line = (line + (vdp_vsram[1] & 0x3ff)) % (vdp_sc_height << 3);
			line2 = (line2 + (vdp_vsram[0] & 0x3ff)) % (vdp_sc_height << 3);
		}
		row = (line / 8) * vdp_sc_width;
		row2 = (line2 / 8) * vdp_sc_width;
		
		for(int x=0;x<(vdp_dis_cells<<3);x+=tileskippix)//8)
		{
			if((iii>3) && ((iii%4) == 0))// && (iii%8))
			{
				int iy = y;
				int ix;
				if (x == 0) 
				{
					iy-=tileskippix;
					ix=(vdp_dis_cells<<3) - 32;
//				goto skip_draw;
//
				}
				else {
					ix = x - 32;
				}
			//if(iy > (223-8)	|| ix > (320-8)) {
			//	goto skip_draw;
			//}
		//while(IO_READ(SP_DMA_BUSY_REG) != 0) {}	
				rdp_load_texture(tile0_sprite,0);
//			rdp_sync(SYNC_TILE);
				rdp_draw_textured_rectangle_scaled(
					0,
					ix,
					iy,
					ix+31,
					iy+7,
					1.0,
					1.0,
					0
				);
/*
				rdp_load_texture(tiles[1],0);
		//	rdp_sync(SYNC_TILE);
				rdp_draw_textured_rectangle_scaled(
					0,
					ix+8,
					iy,
					ix+8+7,
					iy+7,
					1.0,
					1.0,
					0
				);

				rdp_load_texture(tiles[2],0);
			//rdp_sync(SYNC_TILE);
				rdp_draw_textured_rectangle_scaled(
					0,
					ix+16,
					iy,
					ix+16+7,
					iy+7,
					1.0,
					1.0,
					0
				);

				rdp_load_texture(tiles[3],0);
			//rdp_sync(SYNC_TILE);
				rdp_draw_textured_rectangle_scaled(
					0,
					ix+24,
					iy,
					ix+24+7,
					iy+7,
					1.0,
					1.0,
					0
				);*/
			}
			
			uint16_t name_ent;
			uint16_t name_ent2;
skip_draw:
			name_ent = vdp_bgb[row + ((col_off + ((/*pix_off +*/ x) >> 3)) % vdp_sc_width)];//(ent_32 >> 16)&0xffff;//vdp_bgb[row + ((col_off + ((pix_off + x) >> 3)) % vdp_sc_width)];
			name_ent2 = vdp_bga[row2 + ((col_off2 + ((/*pix_off2 +*/ x) >> 3)) % vdp_sc_width)];
			
			if (((name_ent >> 15) == 1) && ((name_ent2 >> 15)==0)) {
				uint16_t tmp = name_ent2;
				name_ent2 = name_ent;
				name_ent = tmp;
				tmp = pix_off2;
				pix_off2 = pix_off;
				pix_off = tmp;
			}

			uint32_t tilenum = ((name_ent & 0x7ff) << 5);
			uint32_t tilenum2 = ((name_ent2 & 0x7ff) << 5);	
//			if(tilenum2 != last_tilenum2 || tilenum != last_tilenum) /*last_vf2 != rsp_vf1 || last_hf2 != rsp_vf1 || last_vf != rsp_vf0 || last_hf != rsp_hf0 ||*/
			{			
				rsp_sprite_data = (uintptr_t)tile0_sprite->data | 0xA0000000;
/*if(x == 0 || x == 1 || x == 38 || x == 39 || y == 0 || y == 1 || y == 27 || y == 28) {
rsp_addr0 = (uint32_t)zeros | 0xA0000000;
rsp_addr1 = (uint32_t)zeros | 0xA0000000;
				last_tilenum = -1;
				last_tilenum2 = -1;
}	
else*/// {
				rsp_addr0 = (uint32_t)vdp_vram + tilenum;
				rsp_addr1 = (uint32_t)vdp_vram + tilenum2;
				last_tilenum = tilenum;
				last_tilenum2 = tilenum2;
//}
				rsp_pn0 = (uint32_t)((name_ent >> 9) & 0x30);
				rsp_hf0 = (uint32_t)((name_ent >> 11) & 0x1);
				rsp_vf0 = (uint32_t)((name_ent >> 12) & 0x1);
				rsp_pn1 = (uint32_t)((name_ent2 >> 9) & 0x30);
				rsp_hf1 = (uint32_t)((name_ent2 >> 11) & 0x1);
				rsp_vf1 = (uint32_t)((name_ent2 >> 12) & 0x1);

				rsp_combine_tiles(iii,0,0);//pix_off,pix_off2);//
				//pix_off, pix_off2);
				//rdp_load_texture(tile0_sprite, 0);

//				last_tilenum = tilenum;
//				last_tilenum2 = tilenum2;
			}
	//		else {
		//		rsp_sprite_data = (uintptr_t)tile0_sprite->data | 0xA0000000;
			//	rsp_addr0 = 0x00003333;
			////	
			//	rsp_combine_tiles(iii,0,0);
		//	}
			iii+=tileskippix/8;
		}
		iii+=((vdp_dis_cells<<3)*((tileskippix/8)-1));
	}
	
//	rdp_sync(SYNC_PIPE);
}
