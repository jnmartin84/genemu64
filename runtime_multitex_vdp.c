/* $Id: vdp_c,v 1.18 2002-08-30 20:23:20 crt0 Exp $ */


#include <libdragon.h>
#include <stdio.h>
#include <stdlib.h>
#include "gen-emu.h"
//#include "vdp.h"
#include "m68k.h"
#include "z80.h"
//#include "h"

//#define NORDP

int MULTITEX=0;
//#define MULTITEX 0

static int p00_slot=0;
static int p01_slot=0;
static int p02_slot=0;
static int p03_slot=0;
static int sprite_slot = 0;

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

uint8_t *vdp_vram;
static uint8_t __attribute__((aligned(8))) vdp_vram2[65536];
uint16_t *vdp_cram;
static uint16_t __attribute__((aligned(8))) vdp_cram2[64];
uint16_t *vdp_vsram;
static uint16_t __attribute__((aligned(8))) vdp_vsram2[64];		/* Only first 40 used. rest are address padding. */
uint16_t __attribute__((aligned(8))) vdp_dc_cram[64];	/* cram in dc format */
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




sprite_t *paltile[64];
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
static uint8_t __attribute__((aligned(8))) tile_data[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_b[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_c[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_d[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_e[sizeof(sprite_t) + (32*32)];

static sprite_t *a_new_sprite = NULL;
static sprite_t *b_new_sprite = NULL;
static sprite_t *c_new_sprite = NULL;
static sprite_t *d_new_sprite = NULL;
static sprite_t *e_new_sprite = NULL;
unsigned char *vramp;// = (uint32_t)vdp->vram;
void vdp_init() 
{
	vdp_vram = (uint8_t*)((uintptr_t)vdp_vram2 | 0xA0000000);
vdp_cram = (uint16_t*)((uintptr_t)vdp_cram2 | 0xA0000000);
vdp_vsram=(uint16_t*)((uintptr_t)vdp_vsram2 | 0xA0000000);
vdp_regs=(uint8_t*)((uintptr_t)vdp_regs2 | 0xA0000000);

	
	//	vramp = (unsigned char *)vdp_vram;
	for(int i=0;i<64;i++) {
		paltile[i] = (sprite_t*)malloc(sizeof(sprite_t) + 64);
		D_memset((void*)(&paltile[i]->data), i, 64);
		data_cache_hit_writeback_invalidate((void*)(&paltile[i]->data), 64);
		paltile[i]->bitdepth = 1;
		paltile[i]->vslices = 1;
		paltile[i]->hslices = 1;
	paltile[i]->width = 8;
	paltile[i]->height = 8;
		}

		a_new_sprite = (sprite_t *)(&tile_data[0]); 
		a_new_sprite->bitdepth = 1;
		a_new_sprite->vslices = 1;
		a_new_sprite->hslices = 1;
	a_new_sprite->width = 8;
	a_new_sprite->height = 8;

	
			c_new_sprite = (sprite_t *)(&tile_data_c[0]); 
		c_new_sprite->bitdepth = 1;
		c_new_sprite->vslices = 1;
		c_new_sprite->hslices = 1;
	c_new_sprite->width = 8;
	c_new_sprite->height = 8;
	
		d_new_sprite = (sprite_t *)(&tile_data_d[0]); 
		d_new_sprite->bitdepth = 1;
		d_new_sprite->vslices = 1;
		d_new_sprite->hslices = 1;
	d_new_sprite->width = 8;
	d_new_sprite->height = 8;

	b_new_sprite = (sprite_t *)(&tile_data_b[0]); 
b_new_sprite->bitdepth = 1;
b_new_sprite->vslices = 1;
b_new_sprite->hslices = 1;
b_new_sprite->width = 8;
	b_new_sprite->height = 8;

	e_new_sprite = (sprite_t *)(&tile_data_e[0]); 
e_new_sprite->bitdepth = 1;
e_new_sprite->vslices = 1;
e_new_sprite->hslices = 1;
e_new_sprite->width = 8;
	e_new_sprite->height = 8;

}


uint16_t vdp_control_read(void)
{
	uint16_t ret = 0x3500;

	vdp_write_pending = 0;

	m68k_set_irq(0);

	ret |= (vdp_status & 0x00ff);

//	if (debug)
//		printf("VDP C -> %04x\n", ret);

	return ret;
}

/* val is big endian */
void vdp_control_write(uint16_t val)
{
//	if (debug)
//		printf("VDP C <- %04x\n", val);

	if((val & 0xc000) == 0x8000)
	{
		if(!vdp_write_pending)
		{
			vdp_regs[(val >> 8) & 0x1f] = (val & 0xff);
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
						//exit(-1);
					}

					/* 68k -> vdp */
					switch(vdp_code & 0x07)
					{
					case 0x01:
						//if(((vdp_addr & 3) == 0) && (vdp_regs[15] == 2) && ((src_off & 1) == 0))
						{
							// THIS ONLY VERIFIED TO WORK FOR SONIC 2 (and only for big endian host)
							D_memcpy(vdp_vram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							vdp_addr += len<<1;
							src_off += len;
						}
						break;
					case 0x03:
						/* cram */
						if(/*((vdp_addr & 3) == 0) &&*/ (vdp_regs[15] == 2)/* && ((src_off & 1) == 0)*/)
						{
							D_memcpy(vdp_cram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
							vdp_addr += len<<1;
							src_off += len;
							uint32_t* start_cramp = (uint32_t *)((uintptr_t)vdp_dc_cram | 0xA0000000);
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
							uint16_t* start_cramp = (uint16_t *)((uintptr_t)vdp_dc_cram | 0xA0000000);
							while(--len)
							{
								uint32_t sh_addr = vdp_addr >> 1;
								val = src_mem[src_off & src_mask];
								vdp_cram[sh_addr] = val;
								start_cramp[sh_addr] = graphics_make_color(
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
#endif
						break;
					case 0x05:
						if(/*((vdp_addr & 3) == 0) &&*/ (vdp_regs[15] == 2) /*&& ((src_off & 1) == 0)*/)
						{
							D_memcpy(vdp_vsram + vdp_addr, src_mem + (src_off & src_mask), len<<1);
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
					vdp_regs[22] = src_off >> 8;
				}
			}
		}
	}
}

uint16_t vdp_data_read(void)
{
	uint16_t ret = 0x0000;

	vdp_write_pending = 0;

	switch(vdp_code)
	{
	case 0x00:
		ret = ((uint16_t *)vdp_vram)[vdp_addr >> 1];
		break;
	case 0x04:
		ret = vdp_vsram[vdp_addr >> 1];
		break;
	case 0x08:
		ret = vdp_cram[vdp_addr >> 1];
		break;
	}

//	if (debug)
//		printf("VDP D -> %04x\n", ret);

	vdp_addr += 2;

	return ret;
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
		*((uint16_t *)(((uintptr_t)&vdp_dc_cram[(vdp_addr >> 1) & 0x3f]) | 0xA0000000)) = 
		//vdp_dc_cram[(vdp_addr >> 1) & 0x3f] = 
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

	uint32_t *dp2 = (uint32_t *)((uintptr_t)b_new_sprite->data|0xA0000000);

	if (!(vdp_dis_cells == 32))
	{
		cells = 80;
		dis_line=20;
		dis_ppl=320;
	}

	vdp_status &= 0x0040; // not too sure about this...
	
	for(int priority=0;priority<2;priority++)
	{
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
				b_new_sprite->width = sh<<3;
				b_new_sprite->height = sv<<3;
			
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
//					data_cache_hit_writeback_invalidate(b_new_sprite->data, 64*b_new_sprite->width * b_new_sprite->height);
					texload_per_frame++;
if(MULTITEX) sprite_slot = 0; else sprite_slot = 0;
					rdp_load_texture(b_new_sprite,sprite_slot);
#endif
				}
				
#ifndef NORDP
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
					sprite_slot,
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
int last_combined=0,last_combined2=0;
	
	int draw,draw2,draw3,draw4;

	for(int y=0;y<224;y+=8) //8
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
		
		for(int x=0;x<vdp_dis_cells<<3;x+=8)//8)
		{
			draw=draw2=draw3=draw4=0;
			uint16_t name_ent = vdp_bgb[row + ((col_off + ((pix_off + x) >> 3)) % vdp_sc_width)];
			uint16_t name_ent2 = vdp_bga[row2 + ((col_off2 + ((pix_off2 + x) >> 3)) % vdp_sc_width)];

			// plane B priority 0
			if ((name_ent >> 15) == 0 && !((name_ent2 >> 15)==0))
			{
				draw = 1;
				draw3 = 0;
				uint32_t tilenum = ((name_ent & 0x7ff) << 5);
				if(tilenum != last_tilenum || last_combined)
				{
					last_combined = 0;
					last_blank = 1;
					vf = ((name_ent >> 12) & 0x1);
					hf = ((name_ent >> 11) & 0x1);
					uint8_t palnum = (name_ent >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)a_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						uint32_t eight_pixels;
						uint32_t half_one_pixels, half_two_pixels;
						uint8_t p1,p2,p3,p4;

						eight_pixels = *tp2;

						if(last_blank && eight_pixels) last_blank = 0;

						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						if(!hf)
						{
							half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						if(!hf)
						{
							half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;

						if(!hf)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}

					if(!last_blank) 
					{

if (MULTITEX) {p00_slot = 0;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(a_new_sprite, p00_slot);
} else p00_slot = 0;//#endif
					}
					else
					{
						draw = 0;
					}
					last_tilenum = tilenum;
				}
			}

			// plane A priority 0
			if ((name_ent2 >> 15)==0 && !((name_ent >> 15)==0))
			{
				draw2 = 1;
				draw4 = 0;
				uint32_t tilenum = ((name_ent2 & 0x7ff) << 5);

				if(tilenum != last_tilenum2 || last_combined)
				{
					last_combined = 0;
					last_blank2 = 1;
					vf = ((name_ent2 >> 12) & 0x1);
					hf = ((name_ent2 >> 11) & 0x1);
					uint8_t palnum = (name_ent2 >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)c_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						uint32_t eight_pixels;
						uint32_t half_one_pixels=0x3f3f3f3f, half_two_pixels=0x2f2f2f2f;
						uint8_t p1,p2,p3,p4;

						eight_pixels = *tp2;

						if(last_blank2 && eight_pixels)
						{
							last_blank2 = 0;
						}
						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						if(!hf)
						{
							half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						if(!hf)
						{
							half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;

						if(!hf)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}
					if(!last_blank2)
					{
if (MULTITEX) {p01_slot = 1;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(c_new_sprite, p01_slot);
} else p01_slot = 0;//#endif

					}
					else
					{
						draw2 = 0;
					}

					last_tilenum2 = tilenum;
				}
			}
#if 1
		if (((name_ent >> 15) == 0) && ((name_ent2 >> 15)==0))
			{
			last_blank = 0;
			last_blank2 = 0;
			draw = 1;
				draw2 = 0;
				int vf2,hf2;
				uint32_t tilenum = ((name_ent & 0x7ff) << 5);	
				uint32_t tilenum2 = ((name_ent2 & 0x7ff) << 5);	
				if(tilenum != last_tilenum || tilenum2 != last_tilenum2 || last_combined == 0)
				{
					last_combined = 1;
					vf = ((name_ent >> 12) & 0x1);
					hf = ((name_ent >> 11) & 0x1);
					vf2 = ((name_ent2 >> 12) & 0x1);
					hf2 = ((name_ent2 >> 11) & 0x1);
					uint8_t palnum = (name_ent >> 9) & 0x30;
					uint8_t palnum2 = (name_ent2 >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)a_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;//,*tp21;
						uint32_t *tp3;//,*tp21;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						if(!vf2)
						{
							tp3 = (uint32_t *)(vdp_vram + tilenum2 + (i<<2));	
						}
						else
						{
							tp3 = (uint32_t *)(vdp_vram + tilenum2 + ((7-i)<<2));
						}

						uint32_t eight_pixels;
						uint32_t half_one_pixels, half_two_pixels;
						uint8_t p1,p2,p3,p4;
						uint32_t eight_pixels2;
						uint32_t half_one_pixels2, half_two_pixels2;
						uint8_t p12,p22,p32,p42;
						uint8_t p13,p23,p33,p43;

						eight_pixels = *tp2;
						eight_pixels2 = *tp3;
						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						p12 = ((eight_pixels2 >> 28)&0xf) | palnum2;
						p22 = ((eight_pixels2 >> 24)&0xf) | palnum2;
						p32 = ((eight_pixels2 >> 20)&0xf) | palnum2;
						p42 = ((eight_pixels2 >> 16)&0xf) | palnum2;
						
						p13 = p1;
						p23 = p2;
						p33 = p3;
						p43 = p4;
						
						// the hf tests need to go here :-(
						if((p12 & 0xf))
							p13 = p12;
						if((p22 & 0xf))
							p23 = p22;
						if((p32 & 0xf))
							p33 = p32;
						if((p42 & 0xf))
							p43 = p42;
						
//						half_one_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						if(!hf)
						{
							half_one_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						}
						else
						{
							half_one_pixels = (p43<<24)|(p33<<16)|(p23<<8)|p13;
						}
						//
						
						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						p12 = ((eight_pixels2 >> 12)&0xf) | palnum2;
						p22 = ((eight_pixels2 >> 8)&0xf) | palnum2;
						p32 = ((eight_pixels2 >> 4)&0xf) | palnum2;
						p42 = ((eight_pixels2 >> 0)&0xf) | palnum2;
						
						p13 = p1;
						p23 = p2;
						p33 = p3;
						p43 = p4;
						
						if((p12 & 0xf))
							p13 = p12;
						if((p22 & 0xf))
							p23 = p22;
						if((p32 & 0xf))
							p33 = p32;
						if((p42 & 0xf))
							p43 = p42;
						
//						half_two_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						if(!hf)
						{
							half_two_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						}
						else
						{
							half_two_pixels = (p43<<24)|(p33<<16)|(p23<<8)|p13;
						}
						//
						if(!hf && !hf2)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}
if (MULTITEX) {p00_slot = 0;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(a_new_sprite, p00_slot);
} else p00_slot = 0;//#endif

					last_tilenum = tilenum;
					last_tilenum2 = tilenum2;
				}
				}			
#endif			
			// plane B priority 1
			if (/*!draw && */(name_ent >> 15) == 1 && !((name_ent2 >> 15)==1))
			{
				draw3 = 1;
				uint32_t tilenum = ((name_ent & 0x7ff) << 5);	

				if(tilenum != last_tilenum3 || last_combined2)
				{
					last_combined2 = 0;
					last_blank3 = 1;
					vf = ((name_ent >> 12) & 0x1);
					hf = ((name_ent >> 11) & 0x1);
					uint8_t palnum = (name_ent >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)e_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						uint32_t eight_pixels;
						uint32_t half_one_pixels, half_two_pixels;
						uint8_t p1,p2,p3,p4;

						eight_pixels = *tp2;

						if(last_blank3 && eight_pixels != 0x00000000) last_blank3 = 0;

						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						
						//half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						if(!hf)
						{
							half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//
						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						
						if(!hf)
						{
							half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;

						if(!hf)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}

					if(!last_blank3) 
					{
if (MULTITEX) {p02_slot = 2;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(e_new_sprite, p02_slot);
} else p02_slot = 0;//#endif

					}
					else
					{
						draw3 = 0;
					}
					last_tilenum3 = tilenum;
				}
			}

			if (/*!draw2 && */(name_ent2 >> 15)==1 && !((name_ent >> 15)==1))
			{
				draw4 = 1;
				uint32_t tilenum = ((name_ent2 & 0x7ff) << 5);
				if(tilenum != last_tilenum4 || last_combined2)
				{
					last_combined2 == 0;
					last_blank4 = 1;
					vf = ((name_ent2 >> 12) & 0x1);
					hf = ((name_ent2 >> 11) & 0x1);
					uint8_t palnum = (name_ent2 >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)d_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						uint32_t eight_pixels;
						uint32_t half_one_pixels, half_two_pixels;
						uint8_t p1,p2,p3,p4;

						eight_pixels = *tp2;

						if(last_blank4 && eight_pixels != 0x00000000) last_blank4 = 0;

						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						
						if(!hf)
						{
							half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						if(!hf)
						{
							half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
						}
						else
						{
							half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
						}
						//half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;

						if(!hf)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}

					if(!last_blank4) 
					{
if (MULTITEX) {p03_slot = 3;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(d_new_sprite, p03_slot);
} else p03_slot = 0;//#endif
					}
					else
					{
						draw4 = 0;
					}
					last_tilenum4 = tilenum;
				}
			}
#if 1
		if ((!draw && !draw2) && ((name_ent >> 15) == 1) && ((name_ent2 >> 15)==1))
			{
			last_blank3 = 0;
			last_blank4 = 0;
			draw3 = 1;
				draw4 = 0;
				int vf2,hf2;
				uint32_t tilenum = ((name_ent & 0x7ff) << 5);	
				uint32_t tilenum2 = ((name_ent2 & 0x7ff) << 5);	
				if(tilenum != last_tilenum3 || tilenum2 != last_tilenum4 || last_combined2 == 0)
				{
					last_combined2 = 1;
					vf = ((name_ent >> 12) & 0x1);
					hf = ((name_ent >> 11) & 0x1);
					vf2 = ((name_ent2 >> 12) & 0x1);
					hf2 = ((name_ent2 >> 11) & 0x1);
					uint8_t palnum = (name_ent >> 9) & 0x30;
					uint8_t palnum2 = (name_ent2 >> 9) & 0x30;
					uint32_t *dp2 = (uint32_t *)((uintptr_t)e_new_sprite->data|0xA0000000);
					for(i=0;i<8;i++)
					{
						uint32_t *tp2;//,*tp21;
						uint32_t *tp3;//,*tp21;
						if(!vf)
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + (i<<2));	
						}
						else
						{
							tp2 = (uint32_t *)(vdp_vram + tilenum + ((7-i)<<2));
						}
						if(!vf2)
						{
							tp3 = (uint32_t *)(vdp_vram + tilenum2 + (i<<2));	
						}
						else
						{
							tp3 = (uint32_t *)(vdp_vram + tilenum2 + ((7-i)<<2));
						}

						uint32_t eight_pixels;
						uint32_t half_one_pixels, half_two_pixels;
						uint8_t p1,p2,p3,p4;
						uint32_t eight_pixels2;
						uint32_t half_one_pixels2, half_two_pixels2;
						uint8_t p12,p22,p32,p42;
						uint8_t p13,p23,p33,p43;

						eight_pixels = *tp2;
						eight_pixels2 = *tp3;
						p1 = ((eight_pixels >> 28)&0xf) | palnum;
						p2 = ((eight_pixels >> 24)&0xf) | palnum;
						p3 = ((eight_pixels >> 20)&0xf) | palnum;
						p4 = ((eight_pixels >> 16)&0xf) | palnum;
						p12 = ((eight_pixels2 >> 28)&0xf) | palnum2;
						p22 = ((eight_pixels2 >> 24)&0xf) | palnum2;
						p32 = ((eight_pixels2 >> 20)&0xf) | palnum2;
						p42 = ((eight_pixels2 >> 16)&0xf) | palnum2;

						p13 = p1;
						p23 = p2;
						p33 = p3;
						p43 = p4;
						
						// hf tests need to go here :-(
						if((p12 & 0xf))
							p13 = p12;
						if((p22 & 0xf))
							p23 = p22;
						if((p32 & 0xf))
							p33 = p32;
						if((p42 & 0xf))
							p43 = p42;						
						if(!hf)
						{
							half_one_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						}
						else
						{
							half_one_pixels = (p43<<24)|(p33<<16)|(p23<<8)|p13;
						}
						//half_one_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;

						p1 = ((eight_pixels >> 12)&0xf) | palnum;
						p2 = ((eight_pixels >> 8)&0xf) | palnum;
						p3 = ((eight_pixels >> 4)&0xf) | palnum;
						p4 = ((eight_pixels >> 0)&0xf) | palnum;
						p12 = ((eight_pixels2 >> 12)&0xf) | palnum2;
						p22 = ((eight_pixels2 >> 8)&0xf) | palnum2;
						p32 = ((eight_pixels2 >> 4)&0xf) | palnum2;
						p42 = ((eight_pixels2 >> 0)&0xf) | palnum2;
						
						p13 = p1;
						p23 = p2;
						p33 = p3;
						p43 = p4;
						
						if((p12 & 0xf))
							p13 = p12;
						if((p22 & 0xf))
							p23 = p22;
						if((p32 & 0xf))
							p33 = p32;
						if((p42 & 0xf))
							p43 = p42;

						if(!hf)
						{
							half_two_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;
						}
						else
						{
							half_two_pixels = (p43<<24)|(p33<<16)|(p23<<8)|p13;
						}
						//half_two_pixels = (p13<<24)|(p23<<16)|(p33<<8)|p43;

						if(!hf && !hf2)
						{
							dp2[(i<<1) + 0] = half_one_pixels;
							dp2[(i<<1) + 1] = half_two_pixels;
						}
						else
						{
							dp2[(i<<1) + 1] = half_one_pixels;
							dp2[(i<<1) + 0] = half_two_pixels;
						}
					}
if (MULTITEX) {p02_slot = 2;//#if MULTITEX
						texload_per_frame++;
						rdp_load_texture(a_new_sprite, p02_slot);
} else p02_slot = 0;//#endif

					last_tilenum3 = tilenum;
					last_tilenum4 = tilenum2;
				}
			}			
#endif
			int didnt_draw = 1;
			
			if(draw)
			{
				didnt_draw=0; 
#ifndef NORDP	
if (!MULTITEX) {
				texload_per_frame++;
				rdp_load_texture(a_new_sprite, p00_slot);
}
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
					p00_slot,
					x<<high,
					y<<high,
					(x<<high)+(7+(8*high)),
					(y<<high)+(7+(8*high)),
					1.0+high,
					1.0+high,
					0
				);
#endif
			}

			if(draw2)
			{
				didnt_draw =0;
#ifndef NORDP	
if (!MULTITEX) {
				texload_per_frame++;
				rdp_load_texture(c_new_sprite, p01_slot);
}
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
					p01_slot,
				x<<high,
					y<<high,
					(x<<high)+(7+(8*high)),
					(y<<high)+(7+(8*high)),
					1.0+high,
					1.0+high,
					0
				);
#endif
			}

			if(draw3)
			{
				didnt_draw=0;
#ifndef NORDP	
if (!MULTITEX) {
				texload_per_frame++;
				rdp_load_texture(e_new_sprite, p02_slot);
}
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
					p02_slot,
					x<<high,
					y<<high,
					(x<<high)+(7+(8*high)),
					(y<<high)+(7+(8*high)),
					1.0+high,
					1.0+high,
					0
				);
#endif
			}
			if(draw4)
			{
				didnt_draw =0;
#ifndef NORDP	
if (!MULTITEX) {
				texload_per_frame++;
				rdp_load_texture(d_new_sprite, p03_slot);
}
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
					p03_slot,
				x<<high,
					y<<high,
					(x<<high)+(7+(8*high)),
					(y<<high)+(7+(8*high)),
					1.0+high,
					1.0+high,
					0
				);
#endif	
			}

/*
			if(1 && didnt_draw)
			{
#ifndef NORDP	
#if !MULTITEX
				texload_per_frame++;
				rdp_load_texture(paltile[15], 0);
#endif
				rect_per_frame++;
				rdp_draw_textured_rectangle_scaled(
#if MULTITEX			
					0,
#else
					0,
#endif
					x<<high,
					y<<high,
					(x<<high)+(7+(8*high)),
					(y<<high)+(7+(8*high)),
					1.0+high,
					1.0+high,
					0
				);
#endif
			}*/
		}
	}
}
