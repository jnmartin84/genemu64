/* $Id: vdp.c,v 1.18 2002-08-30 20:23:20 crt0 Exp $ */

#include <libdragon.h>
#include <stdio.h>
#include "gen-emu.h"
#include "vdp.h"
#include "m68k.h"
#include "z80.h"
#include "cart.h"

#define high 0
// enable palette (tlut)
#define EN_TLUT 0x00800000000000
// enable atomic prim, 1st primitive bandwitdh save
#define ATOMIC_PRIM 0x80000000000000
// enable perspective correction
#define PERSP_TEX_EN 0x08000000000000
// select alpha dither
#define ALPHA_DITHER_SEL_PATTERN 0x00000000000000
#define ALPHA_DITHER_SEL_PATTERNB 0x00001000000000
#define ALPHA_DITHER_SEL_NOISE 0x00002000000000
#define ALPHA_DITHER_SEL_NO_DITHER 0x00003000000000
// select rgb dither
#define RGB_DITHER_SEL_MAGIC_SQUARE_MATRIX 0x00000000000000
#define RGB_DITHER_SEL_STANDARD_BAYER_MATRIX 0x00004000000000
#define RGB_DITHER_SEL_NOISE 0x00008000000000
#define RGB_DITHER_SEL_NO_DITHER 0x0000C000000000
// enable texture filtering
#define SAMPLE_TYPE 0x00200000000000
#define FIELD_SKIP 5


#define vdpr ((val&0x000f)<<4)
#define vdpg ((val&0x00f0)  )
#define vdpb ((val&0x0f00)>>4)	


static uint64_t RDP_CONFIG = /*ATOMIC_PRIM |*/ ALPHA_DITHER_SEL_NO_DITHER | RGB_DITHER_SEL_NO_DITHER;

extern uint16_t *m68k_ram16;
uint16_t *vram16;

struct vdp_s vdp;
uint8_t display_ptr;

uint8_t nt_cells[4] = { 32, 64, 0, 128 };
uint8_t mode_cells[4] = { 32, 40, 0, 40 };

extern void *__safe_buffer[];
display_context_t _dc;

uint16_t *ocr_vram;

//uint16_t rdp_pal[4*16];

uint16_t vdp_control_read(void)
{
	uint16_t ret = 0x3500;

	vdp.write_pending = 0;

	m68k_set_irq(0);

	ret |= (vdp.status & 0x00ff);

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
		if(!vdp.write_pending)
		{
			vdp.regs[(val >> 8) & 0x1f] = (val & 0xff);
			switch((val >> 8) & 0x1f)
			{
			case 0x02:	/* BGA */
				vdp.bga = (uint16_t *)(vdp.vram + ((val & 0x38) << 10));
				break;
			case 0x03:	/* WND */
				vdp.wnd = (uint16_t *)(vdp.vram + ((val & 0x3e) << 10));
				break;
			case 0x04:	/* BGB */
				vdp.bgb = (uint16_t *)(vdp.vram + ((val & 0x07) << 13));
				break;
			case 0x05:	/* SAT */
				vdp.sat = (uint64_t *)(vdp.vram + ((val & 0x7f) << 9));
				break;
			case 0x0c:	/* Mode Set #4 */
				vdp.dis_cells = mode_cells[((vdp.regs[12] & 0x01) << 1) |
											(vdp.regs[12] >> 7)];
				break;
			case 0x0d:
				vdp.hs_off = vdp.regs[13] << 10;
				break;
			case 0x10:	/* Scroll size */
				vdp.sc_width = nt_cells[vdp.regs[16] & 0x03];
				vdp.sc_height = nt_cells[(vdp.regs[16] & 0x30) >> 4];
				break;
			}
			vdp.code = 0x0000;
		}
	}
	else
	{
		if (!vdp.write_pending)
		{
			vdp.write_pending = 1;
			vdp.addr = ((vdp.addr & 0xc000) | (val & 0x3fff));
			vdp.code = ((vdp.code & 0x3c) | ((val & 0xc000) >> 14));
		}
		else
		{
			vdp.write_pending = 0;
			vdp.addr = ((vdp.addr & 0x3fff) | ((val & 0x0003) << 14));
			vdp.code = ((vdp.code & 0x03) | ((val & 0x00f0) >> 2));

			if ((vdp.code & 0x20) && (vdp.regs[1] & 0x10))
			{
				/* dma operation */
				if (((vdp.code & 0x30) == 0x20) && !(vdp.regs[23] & 0x80))
				{
					uint16_t len = (vdp.regs[20] << 8) |  vdp.regs[19];
					uint16_t src_off = (vdp.regs[22] << 8) | vdp.regs[21];
					uint16_t *src_mem, src_mask;

					if ((vdp.regs[23] & 0x70) == 0x70)
					{
						src_mem = m68k_ram16;
						src_mask = 0x7fff;
					}
					else if ((vdp.regs[23] & 0x70) < 0x20)
					{
						src_mem = (uint16_t *)(cart.rom + (vdp.regs[23] << 17));
						src_mask = 0xffff;
					}
					else
					{
						printf("DMA from an unknown block... 0x%02x\n", (vdp.regs[23] << 1));
						while(1);
						//exit(-1);
					}

					/* 68k -> vdp */
					switch(vdp.code & 0x07)
					{
					case 0x01:
						//if(((vdp.addr & 3) == 0) && (vdp.regs[15] == 2) && ((src_off & 1) == 0))
						{
							// THIS ONLY VERIFIED TO WORK FOR SONIC 2 (and only for big endian host)
							__n64_memcpy_ASM(vdp.vram + vdp.addr, src_mem + (src_off & src_mask), len<<1);
							vdp.addr += len<<1;
							src_off += len;
						}
						break;
					case 0x03:
						/* cram */
						if(/*((vdp.addr & 3) == 0) &&*/ (vdp.regs[15] == 2)/* && ((src_off & 1) == 0)*/)
						{
							__n64_memcpy_ASM(vdp.cram + vdp.addr, src_mem + (src_off & src_mask), len<<1);
							vdp.addr += len<<1;
							src_off += len;
							for(int i=0;i<0x40;i++)
							{
								val = vdp.cram[i];
								vdp.dc_cram[i] = graphics_make_color(
									vdpr&0xff,
									vdpg&0xff,
									vdpb&0xff,
									((i % 16) == 0) ? 0x00 : 0xFF
								);
							}
						}
						else
						{
							while(--len)
							{
								uint32_t sh_addr = vdp.addr >> 1;
								val = src_mem[src_off & src_mask];
								vdp.cram[sh_addr] = val;
								vdp.dc_cram[sh_addr] = graphics_make_color(
									vdpr&0xff,
									vdpg&0xff,
									vdpb&0xff,
									((sh_addr % 16) == 0) ? 0x00 : 0xFF
								);
								vdp.addr += vdp.regs[15];
								src_off += 1;

								if(vdp.addr > 0x7f)
								{
									break;
								}
							}
						}
						data_cache_hit_writeback_invalidate(vdp.dc_cram, 64*2);
						rdp_load_palette(0,64,vdp.dc_cram);
						break;
					case 0x05:
						if(/*((vdp.addr & 3) == 0) &&*/ (vdp.regs[15] == 2) /*&& ((src_off & 1) == 0)*/)
						{
							__n64_memcpy_ASM(vdp.vsram + vdp.addr, src_mem + (src_off & src_mask), len<<1);
							vdp.addr += len<<1;
							src_off += len;
						}
						else
						{
							/* vsram */
							while(--len)
							{
								val = src_mem[src_off & src_mask];
								vdp.vsram[vdp.addr >> 1] = val;
								vdp.addr += vdp.regs[15];
								src_off += 1;

								if(vdp.addr > 0x7f)
								{
									break;
								}
							}
						}
						break;
					default:
						printf("68K->VDP DMA Error, code %d.", vdp.code);
						break;
					}

					vdp.regs[19] = vdp.regs[20] = 0;
					vdp.regs[21] = src_off & 0xff;
					vdp.regs[22] = src_off >> 8;
				}
			}
		}
	}
}

uint16_t vdp_data_read(void)
{
	uint16_t ret = 0x0000;

	vdp.write_pending = 0;

	switch(vdp.code)
	{
	case 0x00:
		ret = ((uint16_t *)vdp.vram)[vdp.addr >> 1];
		break;
	case 0x04:
		ret = vdp.vsram[vdp.addr >> 1];
		break;
	case 0x08:
		ret = vdp.cram[vdp.addr >> 1];
		break;
	}

//	if (debug)
//		printf("VDP D -> %04x\n", ret);

	vdp.addr += 2;

	return ret;
}

void vdp_data_write(uint16_t val)
{
	vdp.write_pending = 0;

//	if (debug)
//		printf("VDP D <- %04x\n", val);

	switch(vdp.code)
	{
	case 0x01:
	{
		//if (vdp.addr & 0x01)
		//	val = SWAPBYTES16(val);

		((uint16_t *)vdp.vram)[vdp.addr >> 1] = val;

		vdp.addr += 2;

		break;
	}
	case 0x03:
	{
		vdp.cram[(vdp.addr >> 1) & 0x3f] = val;

		vdp.dc_cram[(vdp.addr >> 1) & 0x3f] = graphics_make_color(
			vdpr&0xff,
			vdpg&0xff,
			vdpb&0xff,
			(((vdp.addr >> 1) & 0x3F) % 16) == 0 ? 0x00 : 0xFF
		);
		
		data_cache_hit_writeback_invalidate(vdp.dc_cram, 64*2);

		vdp.addr += 2;
		break;
	}
	case 0x05:
	{
		vdp.vsram[(vdp.addr >> 1) & 0x3f] = val;

		vdp.addr += 2;
		break;
	}
	default:
	{
		if ((vdp.code & 0x20) && (vdp.regs[1] & 0x10))
		{
			if (((vdp.code & 0x30) == 0x20) && ((vdp.regs[23] & 0xc0) == 0x80))
			{
				/* vdp fill */
				uint16_t len = ((vdp.regs[20] << 8) | vdp.regs[19]);
				vdp.vram[vdp.addr] = val & 0xff;
				val = (val >> 8) & 0xff;

//				__n64_memset_ASM(vdp.vram + vdp.addr, val, len);
				
				while(--len)
				{
					vdp.vram[vdp.addr ^ 1] = val;
					vdp.addr += vdp.regs[15];
				}
				//vdp.addr += vdp.regs[15]*len;
			}
			else if ((vdp.code == 0x30) && ((vdp.regs[23] & 0xc0) == 0xc0))
			{
				/* vdp copy */
				uint16_t len = (vdp.regs[20] << 8) | vdp.regs[19];
				uint16_t addr = (vdp.regs[22] << 8) | vdp.regs[21];

				while(--len)
				{
					vdp.vram[vdp.addr] = vdp.vram[addr++];
					vdp.addr += vdp.regs[15];
				}
				//__n64_memcpy_ASM(vdp.vram + vdp.addr, vdp.vram + addr, len);
				//vdp.addr += vdp.regs[15]*len;
			}
			else
			{
				printf("VDP DMA Error, code %02x, r23 %02x\n", vdp.code, vdp.regs[23]);
			}
		}
	}
	}
}

uint16_t vdp_hv_read(void)
{
	uint16_t h, v;

	v = vdp.scanline;
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

	vdp.scanline = line;

	if (vdp.h_int_counter == 0)
	{
		vdp.h_int_counter = vdp.regs[10];
		if (vdp.regs[0] & 0x10)
		{
			h_int_pending = 1;
		}
	}

	if (line < 224)
	{
		if (line == 0)
		{
			vdp.h_int_counter = vdp.regs[10];
			vdp.status &= 0x0001;
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
			if (vdp.regs[1] & 0x20)
			{
				vdp.status |= 0x0080;
				m68k_set_irq(6);
			}
		}
	}
	else
	{
		vdp.h_int_counter = vdp.regs[10];
		vdp.status |= 0x08;
	}

	vdp.h_int_counter--;
}

void vdp_render_plane(int line, int plane, int priority)
{
	int row, pixrow, i, j;
	sint16_t hscroll = 0;
	sint8_t  col_off, pix_off, pix_tmp;
	uint16_t *p;

	p = plane ? vdp.bgb : vdp.bga;

	switch(vdp.regs[11] & 0x03)
	{
	case 0x0:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (line << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}

	hscroll = (0x400 - hscroll) & 0x3ff;
	col_off = hscroll >> 3;
	pix_off = hscroll & 0x7;
	pix_tmp = pix_off;

	if ((vdp.regs[11] & 0x04) == 0)
	{
		line = (line + (vdp.vsram[(plane ? 1 : 0)] & 0x3ff)) % (vdp.sc_height << 3);
	}

	row = (line >> 3) * vdp.sc_width;
	pixrow = line % 8;

	i = 0;
	uint32_t vdc3 = (vdp.dis_cells << 3);
	while (i < vdc3)
	{
		uint16_t name_ent = p[row + ((col_off + ((pix_off + i) >> 3)) % vdp.sc_width)];

		if ((name_ent >> 15) == priority)
		{
			uint32_t data;
			sint32_t pal, pixel;

			pal = (name_ent >> 9) & 0x30;

			if ((name_ent >> 12) & 0x1)
			{
				data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (28 - (pixrow * 4)));
			}
			else
			{
				data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (pixrow * 4));
			}
#if 0
			if(SWAP)
			{
				data = SWAPWORDS(data);
			}
#endif
			if ((name_ent >> 11) & 0x1)
			{
				for (j = 0; j < 8; j++)
				{
					pixel = data & 0x0f;
					data >>= 4;
					if (pix_tmp > 0)
					{
						pix_tmp--;
						continue;
					}
					if (pixel)
					{
						ocr_vram[i] = vdp.dc_cram[pal | pixel];
					}
					i++;
				}
			}
			else {
				for (j = 0; j < 8; j++)
				{
					pixel = data >> 28;
					data <<= 4;
					if (pix_tmp > 0)
					{
						pix_tmp--;
						continue;
					}
					if (pixel)
					{
						ocr_vram[i] = vdp.dc_cram[pal | pixel];
					}
					i++;
				}
			}
		}
		else
		{
			if (pix_tmp > 0)
			{
				i += (8 - pix_tmp);
				pix_tmp = 0;
			}
			else
			{
				i += 8;
			}
		}
	}
}

#define spr_start (vdp.vram + sn)
sint32_t list_ordered[80] = {-1}; 

void vdp_render_sprites(int line, int priority)
{
	uint32_t spr_ent_bot,spr_ent_top;
	uint32_t c=0, cells=64, i=0, j, k, h,v, sp, sl=0, sh, sv, sn, /*sc,*/ shf, svf;
	uint32_t dis_line=16;
	uint32_t ppl=0;
	uint32_t dis_ppl=256;
	uint32_t sol=0;
	sint32_t sx, sy;
	//uint64_t spr_ent;

	uint8_t pixel;
	uint16_t *pal;
	uint32_t data;
	uint32_t sx_plus_shifted_h;

//	for(j=0;j<80;++j)
//		list_ordered[j]=-1;
	__n64_memset_ASM(list_ordered, 0xff, 80*sizeof(sint32_t));

	if (!(vdp.dis_cells == 32))
	{
		cells = 80;
		dis_line=20;
		dis_ppl=320;
	}

	vdp.status &= 0x0040; // not too sure about this...

	for(i=0;i<cells;++i)
	{
		uint64_t spr_ent = vdp.sat[c];

		spr_ent_top = (spr_ent >> 32);
		spr_ent_bot = (spr_ent & 0x00000000ffffffff);

#if 0
		if(SWAP)
		{
			uint32_t spr_ent_swap = spr_ent_top;
			spr_ent_top = spr_ent_bot;
			spr_ent_top = spr_ent_swap;

			spr_ent_top = SWAPWORDS(spr_ent_top);
			spr_ent_bot = SWAPWORDS(spr_ent_bot);
		}
#endif

		sy = ((spr_ent_top & 0x03FF0000) >> 16) - 128;
		sh = ((spr_ent_top & 0x00000C00) >> 10) +   1;
		sv = ((spr_ent_top & 0x00000300) >> 8)  +   1;

		if((line >= sy) && (line < (sy + (sv << 3))))
		{
			sp = (spr_ent_bot & 0x80000000) >> 31;

			if(sp == priority)
			{
				list_ordered[i]=c;
			}

			sol++;
			ppl+=(sh<<3);

			if(!(sol < dis_line))
			{
	 			vdp.status |= 0x0040;
				goto end;
			}

			if(!(ppl < dis_ppl))
			{
				goto end;
			}
		}

		sl = (spr_ent_top & 0x0000007F);
		if(sl)
		{
			c = sl;
		}
		else
end:
		{
			break;
		}
	}

	for(i=0;i<cells;i++)
	{
		if( list_ordered[ ( 79 - ( (cells==64)?16:0 ) - i ) ] > -1 )
		{
			uint64_t spr_ent = vdp.sat[list_ordered[(79-((cells==64)?16:0)-i)]];
			spr_ent_top = (spr_ent >> 32);
			spr_ent_bot = (spr_ent & 0x00000000ffffffff);
#if 0
			if(SWAP)
			{
				uint32_t spr_ent_swap = spr_ent_top;
				spr_ent_top = spr_ent_bot;
				spr_ent_top = spr_ent_swap;

				spr_ent_top = SWAPWORDS(spr_ent_top);
				spr_ent_bot = SWAPWORDS(spr_ent_bot);
			}
#endif
			sy  = ((spr_ent_top & 0x03FF0000) >> 16) - 128;
			sh  = ((spr_ent_top & 0x00000C00) >> 10) +   1;
			sv  = ((spr_ent_top & 0x00000300) >>  8) +   1;
			svf = ((spr_ent_bot & 0x10000000) >> 28)	  ;
			shf = ((spr_ent_bot & 0x08000000)  >> 27)	 ;
			sn  = ((spr_ent_bot & 0x07FF0000) >> 11)	  ;
			sx  = ((spr_ent_bot & 0x000003FF)	  ) - 128;
//			sc  = ((spr_ent_bot & 0x60000000) >> 29)	  ;
//			pal = vdp.dc_cram + (sc << 4);

//			sc  = ((spr_ent_bot & 0x60000000) >> 25)	  ;
//			pal = vdp.dc_cram + sc;
			pal  = vdp.dc_cram + ((spr_ent_bot & 0x60000000) >> 25);

			for(v = 0; v < sv; v++)
			{
				for(k = 0; k < 8; k++)
				{
					if((sy + k + (v<<3)) == line)
					{
						for(h = 0; h < sh; ++h)
						{
							if (!svf)
							{
								if(shf)
								{
									data = *(uint32_t *)(spr_start + (((sv*(sh-h-1))+v)<<5) + (k << 2));
								}
								else
								{
									data = *(uint32_t *)(spr_start + (((sv*h)+v)<<5) + (k << 2));
								}
							}
							else
							{
								if(shf)
								{
									data = *(uint32_t *)(spr_start + (((sv*(sh-h-1))+(sv-v-1))<<5) + (28 - (k << 2)));
								}
								else
								{
									data = *(uint32_t *)(spr_start + (((sv*h)+(sv-v-1))<<5) + (28 - (k << 2)));
								}
							}
#if 0
							if(SWAP)
							{
								data = SWAPWORDS(data);
							}
#endif
							sx_plus_shifted_h = sx + (h<<3);

							if (shf)
							{
				for(j=0;j<8;j++)
								{
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[j + sx_plus_shifted_h] = pal[pixel];
									}
								}
/*									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h	] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 1] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 2] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 3] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 4] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 5] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 6] = pal[pixel];
									}
									pixel = data & 0x0f;
									data >>= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 7] = pal[pixel];
									}*/
							}
							else
							{
				for(j=0;j<8;j++)
								{
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
										ocr_vram[j + sx_plus_shifted_h] = pal[pixel];
								}
/*									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h	] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 1] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 2] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 3] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 4] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 5] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 6] = pal[pixel];
									}
									pixel = data >> 28;
									data <<= 4;
									if (pixel)
									{
										ocr_vram[sx_plus_shifted_h + 7] = pal[pixel];
									} */
							}
						}
					}
				}
			}
		}
	}
}

/*uint8_t pixel;
uint16_t *pal;
uint32_t data;
uint32_t spr_ent_bot,spr_ent_top;
uint32_t c=0, cells=64, i=0, j, k, h, sp, sl, sh, sv, sn, sc, shf, svf;
uint32_t ty;
uint32_t byte_offset; // this is for the vertical tile row to read from
sint32_t sx, sy;
uint64_t spr_ent;
*/
#if 0
void vdp_render_sprites_alternate(int line, int priority)
{
	uint8_t pixel;
	uint16_t *pal;
	uint32_t data;
	uint32_t spr_ent_bot,spr_ent_top;
	uint32_t c=0, cells=64, i=0, j, k, h, sp, sl, sh, sv, sn, sc, shf, svf;
	uint32_t ty;
	uint32_t byte_offset; // this is for the vertical tile row to read from
	sint32_t sx, sy;
	uint64_t spr_ent;

	c = 0;
	cells = 64;
	i = 0;

	if (!(vdp.dis_cells == 32))
	{
		cells = 80;
	}

	for(i=0;i<cells;i++)
	{
		spr_ent = vdp.sat[c];

		spr_ent_top = (spr_ent >> 32);
		spr_ent_bot = (spr_ent & 0x00000000ffffffff);

		sy = ((spr_ent_top & 0x03FF0000) >> 16)-128;
		sh = ((spr_ent_top & 0x00000C00) >> 10)+1;
		sv = ((spr_ent_top & 0x00000300) >> 8)+1;

		if((line >= sy) && (line < (sy+(sv<<3))))
		{
			sp = (spr_ent_bot & 0x80000000) >> 31;

			if(sp == priority)
			{
				svf = (spr_ent_bot & 0x10000000) >> 28;

				if(svf)
				{
					ty = ((sv<<3) - 1) - (line - sy);
					k = ty % 8;
					byte_offset = ((sv-1)<<5) - ((ty<<2) & 224);
				}
				else
				{
					ty = line - sy;
					k = ty % 8;
					byte_offset = (ty<<2) & 224;
				}

				shf = (spr_ent_bot & 0x08000000) >> 27;
				sn = (spr_ent_bot & 0x07FF0000) >> 11;
				sx = (spr_ent_bot & 0x000003FF)-128;

				sc = (spr_ent_bot & 0x60000000) >> 29;
				pal = vdp.dc_cram + (sc << 4);

				for(h = 0; h < sh; ++h)
				{
					if(svf)
					{
						if(shf)
						{
							data = *(uint32_t *)(spr_start + byte_offset + ((sv*(sh-h-1))<<5) + (28-(k<<2)));
						}
						else
						{
							data = *(uint32_t *)(spr_start + byte_offset + ((sv*h)<<5) + (28-(k<<2)));
						}
					}
					else
					{
						if(shf)
						{
							data = *(uint32_t *)(spr_start + byte_offset + ((sv*(sh-h-1))<<5) + (k<<2));
						}
						else
						{
							data = *(uint32_t *)(spr_start + byte_offset + ((sv*h)<<5) + (k<<2));
						}
					}

					if (shf)
					{
						for(j=0;j<8;j++)
						{
							pixel = data & 0x0f;
							data >>= 4;
							if (pixel)
							{
								ocr_vram[sx + j + (h<<3)] = pal[pixel];
							}
						}
					}
					else
					{
						for(j=0;j<8;j++)
						{
							pixel = data >> 28;
							data <<= 4;
							if (pixel)
							{
								ocr_vram[sx + j + (h<<3)] = pal[pixel];
							}
						}
					}
				}
			}
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

#endif

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


void vdp_render_scanline(int line)
{
        // RDP TEXTURE MODE
//        rdp_texture_copy(RDP_CONFIG);
//}	
//else {
	//while(1) {}
//}
	
//	if(!(line &1)) 
	{
	int i;
	ocr_vram = &((uint16_t *)__safe_buffer[(_dc)-1])[(line * 320)];

//	uint32_t *ocr_vram_long = (uint32_t*)ocr_vram;
//	uint32_t long_color = (vdp.dc_cram[vdp.regs[7] & 0x3f] << 16) | (vdp.dc_cram[vdp.regs[7] & 0x3f]);
//	for (i = 0; i < (vdp.sc_width << 3); i++)
//	{
//		if(i >= 319) break;
//		ocr_vram/*_long*/[i] = (vdp.dc_cram[vdp.regs[7] & 0x3f])&0xFFFF;
//long_color;
//	}
/*uint16_t color = vdp.dc_cram[vdp.regs[7] & 0x3f];
rdp_set_fill_color((color>>11)&0xff, (color>>6)&0xff, (color >>1)&0xff, 0xff);
rdp_draw_filled_rectangle(0, line, 319, line+1);*/


	/* Prefill the scanline with the backdrop color. */
//	for (i = 0; i < (vdp.sc_width * 8); i++)
//		ocr_vram[i] = vdp.dc_cram[vdp.regs[7] & 0x3f];

	if (vdp.regs[1] & 0x40)
	{
	//	vdp_render_plane(line, 1, 0);
	//	vdp_render_plane(line, 0, 0);
//		vdp_render_sprites_alternate(line, 0);
		vdp_render_sprites(line, 0);
	//	vdp_render_plane(line, 1, 1);
	//	vdp_render_plane(line, 0, 1);
//		vdp_render_sprites_alternate(line, 1);
		vdp_render_sprites(line, 1);
	}
}
/*
	for(i=0;i<vdp.sc_width*8;i++) {
		video_ptr[i] = ocr_vram[i];
	}
*/
//	sq_cpy((((uint16_t *)display_txr) + (line * 512)), ocr_vram, (vdp.dis_cells * 8 * 2));
}
//uint8_t tile_data2[(32*32)];

static uint8_t __attribute__((aligned(8))) tile_data[sizeof(sprite_t) + (32*32)];
static uint8_t __attribute__((aligned(8))) tile_data_b[sizeof(sprite_t) + (32*32)];

static sprite_t *a_new_sprite = NULL;
static sprite_t *b_new_sprite = NULL;

void render_all_sprites(int priority) {
		uint32_t spr_ent_bot,spr_ent_top;
	uint32_t c=0, cells=64, i=0, j, k, h,v, sp, sl=0, sh, sv, sn, /*sc,*/ shf, svf;
	uint32_t dis_line=16;
	uint32_t ppl=0;
	uint32_t dis_ppl=256;
	uint32_t sol=0;
	sint32_t sx, sy;
	//uint64_t spr_ent;
uint32_t last_sn = 0xFFFFFFFF;
uint32_t last_svf = 0xFFFFFFFF;
uint32_t last_shf = 0xFFFFFFFF;
	uint8_t pixel;
	uint16_t *pal;
	uint32_t data;
	uint32_t sx_plus_shifted_h;

//	if(!b_new_sprite) {
b_new_sprite = (sprite_t *)(&tile_data_b[0]); //malloc(sizeof(sprite_t) + (32));
//b_new_sprite->width = 8;
//b_new_sprite->height = 8;
b_new_sprite->bitdepth = 1;
b_new_sprite->vslices = 1;
b_new_sprite->hslices = 1;
//}
uint32_t *dp2 = (uint32_t *)(b_new_sprite->data);
	
//	__n64_memset_ASM(list_ordered, 0xff, 80*sizeof(sint32_t));

	if (!(vdp.dis_cells == 32))
	{
		cells = 80;
		dis_line=20;
		dis_ppl=320;
	}

//	vdp.status &= 0x0040; // not too sure about this...

	for(i=0;i<cells;++i)
{
		uint64_t spr_ent = vdp.sat[c];

		spr_ent_top = (spr_ent >> 32);
		spr_ent_bot = (spr_ent & 0x00000000ffffffff);

		sp = (spr_ent_bot & 0x80000000) >> 31;

		if(sp == priority)
		{
			list_ordered[( 79 - ( (cells==64)?16:0 ) - i )]=c;
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

	for(i=0;i<cells;i++)
	//sl = 0xFFFFFFFF;
	//sl = 0;
	//while(1)
	{
		int next_index = list_ordered[ ( /*79 - ( (cells==64)?16:0 ) -*/ i ) ];
		if( next_index > -1 )
		{
			list_ordered[i] = -1;
			uint64_t spr_ent = vdp.sat[//sl];
			next_index];
			spr_ent_top = (spr_ent >> 32);
			spr_ent_bot = (spr_ent & 0x00000000ffffffff);

		sp = (spr_ent_bot & 0x80000000) >> 31;

//		sl = (spr_ent_top & 0x0000007F);
		/*if(sl)
		{
			c = sl;
		}*/
//		else
//		{
//			return;
			//break;
//		}

		if(sp == priority)
		{		//{
			//continue;
		//}
		
			sy  = ((spr_ent_top & 0x03FF0000) >> 16) - 128;
			sh  = ((spr_ent_top & 0x00000C00) >> 10) +   1;
			sv  = ((spr_ent_top & 0x00000300) >>  8) +   1;
			svf = ((spr_ent_bot & 0x10000000) >> 28)	  ;
			shf = ((spr_ent_bot & 0x08000000)  >> 27)	 ;
			sn  = ((spr_ent_bot & 0x07FF0000) >> 11)	  ;
			sx  = ((spr_ent_bot & 0x000003FF)	  ) - 128;
			 uint8_t palnum = ((spr_ent_bot & 0x60000000) >> 25);
b_new_sprite->width = sh<<3;
b_new_sprite->height = sv<<3;
if(last_sn != sn || last_shf != shf || last_svf != svf) {
//					__n64_memset_ASM(a_new_sprite->data, 0, (a_new_sprite->width*a_new_sprite->height));//a_new_sprite->width*a_new_sprite->height);
			//			#define spr_start (vdp.vram + sn)
			for(v = 0; v < sv; v++)
			{
				for(h = 0; h < sh; h++)
				{

uint8_t *tp;
if(svf && !shf) {
	tp = (uint8_t*)(spr_start  + (((sv*h)+(sv-v-1))<<5));
}
if(shf && !svf) {
	tp = (uint8_t*)(spr_start  + (((sv*(sh-h-1))+(v))<<5));
}
else if(svf && shf) {
	tp = (uint8_t*)(spr_start  + (((sv*(sh-h-1))+(sv-v-1))<<5));
}
else {
tp = (uint8_t *)(spr_start  + (((sv*h)+v)<<5));
}				//					__n64_memset_ASM(a_new_sprite->data, 0, 64);
//					for(int nnn=0;nnn<32;nnn++)
#if 1
	for(int nny=0;nny<8;nny++) {	
// read a 32-bit word
// make two 32-bit words
// write two 32-bit words
uint32_t *tp2;

if(!svf) {
tp2 = (uint32_t *)(tp + (nny<<2));	
}
else {
	tp2 = (uint32_t *)(tp + ((7-nny)<<2));
}

uint32_t eight_pixels = *tp2;
uint32_t half_one_pixels, half_two_pixels;
	
	uint8_t p1,p2,p3,p4;

	p1 = ((eight_pixels >> 28)&0xf) | palnum;
	p2 = ((eight_pixels >> 24)&0xf) | palnum;
	p3 = ((eight_pixels >> 20)&0xf) | palnum;
	p4 = ((eight_pixels >> 16)&0xf) | palnum;
	
if(!shf) {
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
}
else {
	half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
}
	p1 = ((eight_pixels >> 12)&0xf) | palnum;
	p2 = ((eight_pixels >> 8)&0xf) | palnum;
	p3 = ((eight_pixels >> 4)&0xf) | palnum;
	p4 = ((eight_pixels >> 0)&0xf) | palnum;
if(!shf) {	
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
}
else {
	half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
}

if(!shf) {
	dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)] = half_one_pixels;
	dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)+1] = half_two_pixels;
}
else {
	dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)] = half_two_pixels;
	dp2[((nny + (v<<3))*(sh<<1)) + (h<<1)+1] = half_one_pixels;
}
	/*for(int nnx=0;nnx<4;nnx++)
						{
						uint8_t tmp = tp[nnx + (nny<<2)];
						dp[
						(((v<<3)+nny)*b_new_sprite->width)
						+ ((h<<3)+(nnx<<1)+0)]
						= ((tmp >> 4) & 0x0F) | ((spr_ent_bot & 0x60000000) >> 25);

						dp[
						(((v<<3)+nny)*b_new_sprite->width)
						+ ((h<<3)+(nnx<<1)+1)]
						= (tmp & 0x0F ) | ((spr_ent_bot & 0x60000000) >> 25);

					}
					*/
				}
#endif				
			}
		}
		last_sn = sn;
		last_shf = shf;
		last_svf = svf;
			data_cache_hit_writeback_invalidate( b_new_sprite->data, b_new_sprite->width*b_new_sprite->height );
			//rdp_load_palette(0,64,vdp.dc_cram); // upload starting on palette 0, 15 colors upload, point to the palette struct
			rdp_load_texture(b_new_sprite);
}
			rdp_draw_textured_rectangle_scaled(
			(sx<<high), 
			(sy<<high), 
			(sx<<high) + ((sh<<3)-1) + ((sh<<3)*high), 
			(sy<<high) + ((sv<<3)-1) + ((sv<<3)*high), 
			1.0+high, 
			1.0+high, 
			0);
			rdp_sync(SYNC_PIPE);
		}			
//			if(sl == 0) return;			
			//(sx + (h*8))<<high,(sy + (v*8))<<high,((sx + (h*8))<<high) + (7+(8*high)),((sy + (v*8))<<high)+(7+(8*high)),1.0+high,1.0+high,shf);

		}
	}
}


#if 0
void render_a_whole_plane_4wide(int plane, int priority) {
	int row, pixrow, i, j;
	sint16_t hscroll = 0;
	sint8_t  col_off, pix_off, pix_tmp;
	uint16_t *p;
	int line;
int hf,vf;


//if(!a_new_sprite) {
a_new_sprite = (sprite_t *)(&tile_data[0]);
a_new_sprite->bitdepth = 1;
a_new_sprite->vslices = 1;
a_new_sprite->hslices = 1;
//}
a_new_sprite->width = 32;
a_new_sprite->height = 32;

	p = plane ? vdp.bgb : vdp.bga;


	for(int y=0;y<224;y+=16)//2 cells each
	{
		for(int x=0;x<vdp.dis_cells<<3;x+=32) //4 cells each iteration
		{
	//__n64_memset_ASM(a_new_sprite->data, 0, 1024);
			//p[row]
			for(int cellm = 0; cellm < 4; cellm++) {
		line = y + (cellm<<3); 
	switch(vdp.regs[11] & 0x03) {
	case 0x0:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (line << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}

	hscroll = (0x400 - hscroll) & 0x3ff;
	col_off = hscroll >> 3;
	pix_off = hscroll & 0x7;
	pix_tmp = pix_off;

	if ((vdp.regs[11] & 0x04) == 0)
		line = (line + (vdp.vsram[(plane ? 1 : 0)] & 0x3ff)) % (vdp.sc_height << 3);

	row = (line / 8) * vdp.sc_width;
	pixrow = line % 8;

			for(int celln = 0; celln < 4; celln++) {
			uint16_t name_ent = p[row + ((col_off + ((pix_off + x + (celln<<3)) >> 3)) % vdp.sc_width)];

			if ((name_ent >> 15) == priority) {
vf = ((name_ent >> 12) & 0x1);
hf = ((name_ent >> 11) & 0x1);
				
/*				for(i=0;i<32;i++) {
					uint8_t tmp = *(uint8_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + i);
					uint8_t *dp = (uint8_t *)(a_new_sprite->data);
					dp[(i<<1) + 0] = ((tmp >> 4) & 0x0F) |  (name_ent >> 9) & 0x30;
					dp[(i<<1) + 1] = (tmp & 0x0F )|  (name_ent >> 9) & 0x30;
				}*/
				
				uint8_t *dp = (uint8_t *)(a_new_sprite->data
/*+
((cellm<<3)*a_new_sprite->width)				
+
(celln<<3)*/
				);
				uint8_t *tp = (uint8_t *)(vdp.vram + ((name_ent & 0x7ff) << 5));
						//					__n64_memset_ASM(a_new_sprite->data, 0, 64);
//					for(int nnn=0;nnn<32;nnn++)
	uint8_t palnum = (name_ent >> 9) & 0x30;
#if 1
	for(int nny=0;nny<8;nny++) {	

// new algorithm

// read a 32-bit word
// make two 32-bit words
// write two 32-bit words
uint32_t *tp2;
if(!vf) {
tp2 = (uint32_t *)(tp + (nny<<2));	
}
else {
tp2 = (uint32_t *)(tp + ((7-nny)<<2));
}
	uint32_t *dp2 = (uint32_t *)(dp);
uint32_t eight_pixels = *tp2;
uint32_t half_one_pixels, half_two_pixels;
	
	uint8_t p1,p2,p3,p4;

	p1 = ((eight_pixels >> 28)&0xf) | palnum;
	p2 = ((eight_pixels >> 24)&0xf) | palnum;
	p3 = ((eight_pixels >> 20)&0xf) | palnum;
	p4 = ((eight_pixels >> 16)&0xf) | palnum;
if(!hf) {	
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
}
else {
	half_one_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
}
	p1 = ((eight_pixels >> 12)&0xf) | palnum;
	p2 = ((eight_pixels >> 8)&0xf) | palnum;
	p3 = ((eight_pixels >> 4)&0xf) | palnum;
	p4 = ((eight_pixels >> 0)&0xf) | palnum;
	
if(!hf) {
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
}
else {
	half_two_pixels = (p4<<24)|(p3<<16)|(p2<<8)|p1;
}

if(!hf) {
	dp2[((nny + (cellm<<3))<<3) + (celln<<1)] = half_one_pixels;
	dp2[((nny + (cellm<<3))<<3) + (celln<<1)+1] = half_two_pixels;
}
else {
	dp2[((nny + (cellm<<3))<<3) + (celln<<1)] = half_two_pixels;
	dp2[((nny + (cellm<<3))<<3) + (celln<<1)+1] = half_one_pixels;
}
	}
	/*
	for(int nnx=0;nnx<4;nnx++)
						{
			//			uint8_t *dp2 = dp + (nny*a_new_sprite->width) + (nnx<<1);


			uint8_t tmp;
				if (((name_ent >> 12) & 0x1)&&((name_ent >> 11) & 0x1))
				{
					tmp = tp[(4-nnx) + ((7-nny)<<2)];
					tmp = (tmp >> 4) & 0xf | (tmp << 4) & 0xf0;
				}
				else if (((name_ent >> 11) & 0x1) && !((name_ent >> 12) & 0x1))
				{
					tmp = tp[(4-nnx)+ (nny<<2)];
					tmp = (tmp >> 4) & 0xf | (tmp << 4) & 0xf0;
				}
				else if (((name_ent >> 12) & 0x1) && !((name_ent >> 11) & 0x1))
				{
					tmp = tp[nnx+ ((7-nny)<<2)];
				}
				else
				{
					tmp = tp[nnx + (nny<<2)];
				}
								dp[
						((nny + (cellm<<3))*a_new_sprite->width)				
+
((celln<<3) + (nnx<<1)) + 0
						] = ((tmp >> 4) & 0x0F) | (name_ent >> 9) & 0x30;
						dp[
						((nny + (cellm<<3))*a_new_sprite->width)				
+
((celln<<3) + (nnx<<1)) + 1
						] = (tmp & 0x0F ) | (name_ent >> 9) & 0x30;
			}
		}*/
#endif				
	}
			else {
				uint8_t *dp = (uint8_t *)(a_new_sprite->data);
	for(int nny=0;nny<8;nny++) {	
/*for(int nnx=0;nnx<4;nnx++)
						{
										dp[
						(((cellm<<3)+nny)*a_new_sprite->width)
						+ ((celln<<3)+(nnx<<1)+0)]
						= 0 | (name_ent >> 9) & 0x30;

						dp[						(((cellm<<3)+nny)*a_new_sprite->width)
						+ ((celln<<3)+(nnx<<1)+1)]
						= 0 | (name_ent >> 9) & 0x30;
			}*/
uint32_t *dp3 = (uint32_t*)(dp + (((cellm<<3)+nny)*a_new_sprite->width) + (celln<<3));
/*dp3[0]=0;
dp3[1]=0;
dp3[2]=0;
dp3[3]=0;
dp3[4]=0;
dp3[5]=0;
dp3[6]=0;
dp3[7]=0;*/
			__n64_memset_ASM(dp + (((cellm<<3)+nny)*a_new_sprite->width) + (celln<<3), 0, 8);
	}
			}
			}
			}
				data_cache_hit_writeback_invalidate( a_new_sprite->data, 1024 );
				rdp_load_texture(a_new_sprite);
				rdp_draw_textured_rectangle_scaled(x<<high,y<<high,(x<<high)+(31+(32*high)),(y<<high)+(31+(32*high)),1.0+high,1.0+high,0);//(((name_ent >> 11) & 0x1)));

		}
	}
}
#endif


#if 1
void render_a_whole_plane(int plane, int priority) {
	//int plane = 0;
	//int priority = 0;
	int row, //pixrow,
	i, j;
sint16_t hscroll2;
	sint16_t hscroll = 0;
	sint8_t  col_off, pix_off, //pix_tmp, 
	col_off2, pix_off2;//, pix_tmp2;
	uint16_t *p;
	int line;
int hf,vf;
	int last_tilenum=-1;
	//int last_hf = -1;
	//int last_vf = -1;
	//		data_cache_hit_writeback_invalidate(vdp.dc_cram, 64*2);

if(!a_new_sprite) {
a_new_sprite = (sprite_t *)(&tile_data[0]); //malloc(sizeof(sprite_t) + (32));
a_new_sprite->bitdepth = 1;
a_new_sprite->vslices = 1;
a_new_sprite->hslices = 1;
}
a_new_sprite->width = 8;
a_new_sprite->height = 8;

	p = plane ? vdp.bgb : vdp.bga;


	for(int y=0;y<224;y+=8)
	{
		line = y;
	switch(vdp.regs[11] & 0x03) {
	case 0x0:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (line << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}

	hscroll = (0x400 - hscroll) & 0x3ff;
	col_off = hscroll >> 3;
	pix_off = hscroll & 0x7;
//	pix_tmp = pix_off;

	if ((vdp.regs[11] & 0x04) == 0)
		line = (line + (vdp.vsram[(plane ? 1 : 0)] & 0x3ff)) % (vdp.sc_height << 3);

	row = (line / 8) * vdp.sc_width;
//	pixrow = line % 8;
	
		for(int x=0;x<vdp.dis_cells<<3;x+=8)
		{
			//p[row]
			uint16_t name_ent = p[row + ((col_off + ((/*pix_off +*/ x) >> 3)) % vdp.sc_width)];
			if ((name_ent >> 15) == priority) {
vf = ((name_ent >> 12) & 0x1);
hf = ((name_ent >> 11) & 0x1);

			//if(((name_ent & 0x7ff) << 5) != last_tilenum) {
			uint32_t tilenum = ((name_ent & 0x7ff) << 5);	
			//if(last_tilenum != tilenum)
			{
	/*			for(i=0;i<32;i++) {
					uint8_t pal = (name_ent >> 9) & 0x30;
					uint8_t tmp = *(uint8_t *)(vdp.vram + tilenum + i);
					uint16_t *dp = (uint16_t *)(a_new_sprite->data);
					//dp[(i<<1) + 0] = ((tmp >> 4) & 0x0F) |  pal ;
					//dp[(i<<1) + 1] = (tmp & 0x0F ) | pal;
					dp[i] = (((tmp << 4) & 0x0F00) | (tmp & 0x000F)) | ((pal << 8) | pal);
				}*/
	uint32_t *dp2 = (uint32_t *)(a_new_sprite->data);
				for(i=0;i<8;i++) {
/*	switch(vdp.regs[11] & 0x03) {
	case 0x0:
		hscroll2 = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll2 = ((uint16_t *)vdp.vram)[(vdp.hs_off + (((line+i) & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll2 = ((uint16_t *)vdp.vram)[(vdp.hs_off + (((line+i) & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll2 = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line+i) << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}

	hscroll2 = (0x400 - hscroll2) & 0x3ff;
	pix_off2 = hscroll2 & 0x7;
*/					
					uint8_t palnum = (name_ent >> 9) & 0x30;
					uint32_t *tp2 = (uint32_t *)(vdp.vram + tilenum + (i<<2));	
					uint32_t eight_pixels = *tp2;
uint32_t half_one_pixels, half_two_pixels;
	
	uint8_t p1,p2,p3,p4;

/*switch(pix_off) {
case 0:
*/	p1 = ((eight_pixels >> 28)&0xf) | palnum;
	p2 = ((eight_pixels >> 24)&0xf) | palnum;
	p3 = ((eight_pixels >> 20)&0xf) | palnum;
	p4 = ((eight_pixels >> 16)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 12)&0xf) | palnum;
	p2 = ((eight_pixels >> 8)&0xf) | palnum;
	p3 = ((eight_pixels >> 4)&0xf) | palnum;
	p4 = ((eight_pixels >> 0)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
/*	break;
case 1:
	p1 = ((eight_pixels >> 24)&0xf) | palnum;
	p2 = ((eight_pixels >> 20)&0xf) | palnum;
	p3 = ((eight_pixels >> 16)&0xf) | palnum;
	p4 = ((eight_pixels >> 12)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 8)&0xf) | palnum;
	p2 = ((eight_pixels >> 4)&0xf) | palnum;
	p3 = ((eight_pixels >> 0)&0xf) | palnum;
	p4 = ((eight_pixels >> 28)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 2:
	p1 = ((eight_pixels >> 20)&0xf) | palnum;
	p2 = ((eight_pixels >> 16)&0xf) | palnum;
	p3 = ((eight_pixels >> 12)&0xf) | palnum;
	p4 = ((eight_pixels >> 8)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 4)&0xf) | palnum;
	p2 = ((eight_pixels >> 0)&0xf) | palnum;
	p3 = ((eight_pixels >> 28)&0xf) | palnum;
	p4 = ((eight_pixels >> 24)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 3:
	p1 = ((eight_pixels >> 16)&0xf) | palnum;
	p2 = ((eight_pixels >> 12)&0xf) | palnum;
	p3 = ((eight_pixels >> 8)&0xf) | palnum;
	p4 = ((eight_pixels >> 4)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 0)&0xf) | palnum;
	p2 = ((eight_pixels >> 28)&0xf) | palnum;
	p3 = ((eight_pixels >> 24)&0xf) | palnum;
	p4 = ((eight_pixels >> 20)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 4:
	p1 = ((eight_pixels >> 12)&0xf) | palnum;
	p2 = ((eight_pixels >> 8)&0xf) | palnum;
	p3 = ((eight_pixels >> 4)&0xf) | palnum;
	p4 = ((eight_pixels >> 0)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 28)&0xf) | palnum;
	p2 = ((eight_pixels >> 24)&0xf) | palnum;
	p3 = ((eight_pixels >> 20)&0xf) | palnum;
	p4 = ((eight_pixels >> 16)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 5:
	p1 = ((eight_pixels >> 8)&0xf) | palnum;
	p2 = ((eight_pixels >> 4)&0xf) | palnum;
	p3 = ((eight_pixels >> 0)&0xf) | palnum;
	p4 = ((eight_pixels >> 28)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 24)&0xf) | palnum;
	p2 = ((eight_pixels >> 20)&0xf) | palnum;
	p3 = ((eight_pixels >> 16)&0xf) | palnum;
	p4 = ((eight_pixels >> 12)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 6:
	p1 = ((eight_pixels >> 4)&0xf) | palnum;
	p2 = ((eight_pixels >> 0)&0xf) | palnum;
	p3 = ((eight_pixels >> 28)&0xf) | palnum;
	p4 = ((eight_pixels >> 24)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 20)&0xf) | palnum;
	p2 = ((eight_pixels >> 16)&0xf) | palnum;
	p3 = ((eight_pixels >> 12)&0xf) | palnum;
	p4 = ((eight_pixels >> 8)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
case 7:
	p1 = ((eight_pixels >> 0)&0xf) | palnum;
	p2 = ((eight_pixels >> 28)&0xf) | palnum;
	p3 = ((eight_pixels >> 24)&0xf) | palnum;
	p4 = ((eight_pixels >> 20)&0xf) | palnum;
	half_one_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	p1 = ((eight_pixels >> 16)&0xf) | palnum;
	p2 = ((eight_pixels >> 12)&0xf) | palnum;
	p3 = ((eight_pixels >> 4)&0xf) | palnum;
	p4 = ((eight_pixels >> 8)&0xf) | palnum;
	half_two_pixels = (p1<<24)|(p2<<16)|(p3<<8)|p4;
	break;
}*/

	
//if(!hf) {
	dp2[(i<<1) + 0] = half_one_pixels;
	dp2[(i<<1) + 1] = half_two_pixels;
//}
//else {
//	dp2[((nny + (cellm<<3))<<3) + (celln<<1)] = half_two_pixels;
//	dp2[((nny + (cellm<<3))<<3) + (celln<<1)+1] = half_one_pixels;
//}
				}
				#if 0
//__n64_memset_ASM(tile_data2, 15, 64);
//				__n64_memcpy_ASM((void*)a_new_sprite->data,
	//			tex0,
		//		64);
				
				if (((name_ent >> 12) & 0x1)&&((name_ent >> 11) & 0x1))
				{
					for(j=0;j<8;j++)
					{
						for(i=0;i<8;i++) {
							tile_data2[i + (j*8)] = ((uint8_t *)a_new_sprite->data)[(7-i) + ((7-j)*8)];
						}
					}
					__n64_memcpy_ASM((void*)a_new_sprite->data, (void*)&tile_data2[0], 64);

//				__n64_memcpy_ASM((void*)a_new_sprite->data,
	//			tex3,
		//		64);

				}

				else if (((name_ent >> 11) & 0x1) && !((name_ent >> 12) & 0x1))
				{
					for(j=0;j<8;j++)
					{
						for(i=0;i<8;i++)
						{
							tile_data2[i + (j*8)] = ((uint8_t *)a_new_sprite->data)[(7-i) + (j*8)];
						}
					}
					__n64_memcpy_ASM((void*)a_new_sprite->data, tile_data2, 64);

/*				__n64_memcpy_ASM((void*)a_new_sprite->data,
				tex1,
				64);*/

				}

				else if (((name_ent >> 12) & 0x1)&&!((name_ent >> 11) & 0x1))
				{
					for(j=0;j<8;j++)
					{
						for(i=0;i<8;i++) {
							tile_data2[i + (j*8)] = ((uint8_t *)a_new_sprite->data)[(i) + ((7-j)*8)];
						}
					}
					__n64_memcpy_ASM((void*)a_new_sprite->data, (void*)&tile_data2[0], 64);

			//	__n64_memcpy_ASM((void*)a_new_sprite->data,
				//tex2,
				//64);
				}
				

//				__n64_memset_ASM((void*)a_new_sprite->data, (((name_ent >> 12)&0x1)<<1) | ((name_ent >> 11) & 0x1)+1, 64);

				//__n64_memcpy_ASM(a_new_sprite->data, (vdp.vram + ((name_ent & 0x7ff) << 5)), 32);//*32);
/*				for(int n=0;n<8;n++)
				{
					for(int m=0;m<4;m++)
					{
						((uint8_t *)a_new_sprite->data)[m + (n*4)] =
						((uint8_t *)a_new_sprite->data)[n + (m*8)];
					}
				}*/
//				for(int i=0;i<8;i++) {
//					*(uint32_t *)(a_new_sprite->data + (i<<2)) = *(uint32_t*)(a_new_sprite->data + ((7-i)<<2));
//				}
/*				if (((name_ent >> 11) & 0x1))
				{
					__n64_memset_ASM(tile_data2,0,64);
						for(i=0;i<8;i++)
						{
							for(j=0;j<8;j++)
							{
					if ((name_ent >> 12) & 0x1)
					{
								uint8_t tmp = ((uint8_t*)(a_new_sprite->data))[((7-i)*8) + (7-j)];
								*(uint8_t *)(&tile_data2[0] + (i*8) + j) = tmp;
					}
					else 
					{
								uint8_t tmp = *(uint8_t*)(a_new_sprite->data + (((7-i)*8) + (7-j)));
								*(uint8_t *)(&tile_data2[0] + (i*8) + j) = tmp;
					}
							}
						}
					__n64_memcpy_ASM((void*)a_new_sprite->data, (void*)&tile_data2[0], 64);
				}*/
				#endif
//				__n64_memcpy_ASM(a_new_sprite->data, vdp.vram + ((name_ent & 0x7ff) << 5), 32);
				data_cache_hit_writeback_invalidate( a_new_sprite->data, 64 );
//				int pal = (name_ent >> 9) & 0x30;
				// pal = 0x00, 0x10, 0x20, 0x30
//				rdp_load_palette(0,16,vdp.dc_cram + pal); // upload starting on palette 0, 15 colors upload, point to the palette struct
				rdp_load_texture(a_new_sprite);
				//last_tilenum = ((name_ent & 0x7ff) << 5);
			}

			
				//last_tilenum = ((name_ent & 0x7ff) << 5);
				//}
				int flag = 0;
/*				if (((name_ent >> 11) & 0x1)) {
						flag += 1;
				}
				else if (((name_ent >> 12) & 0x1)) {
						flag += 2;
				}*/
				rdp_draw_textured_rectangle_scaled(x<<high,y<<high,(x<<high)+(7+(8*high)),(y<<high)+(7+(8*high)),1.0+high,1.0+high,flag );
			}
		}
	}
	#if 0
	for(int y=0;y<224>>3;y+=4) {
		int line = y<<3;
	/*if ((vdp.regs[11] & 0x04) == 0)
	{
		line = (line + (vdp.vsram[(plane ? 1 : 0)] & 0x3ff)) % (vdp.sc_height << 3);
	}*/
/*	switch(vdp.regs[11] & 0x03)
	{	case 0x0:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (plane ? 2 : 0)) >> 1];
		break;
	case 0x1:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & 0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x2:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + ((line & ~0x7) << 1) + (plane ? 2 : 0)) >> 1];
		break;
	case 0x3:
		hscroll = ((uint16_t *)vdp.vram)[(vdp.hs_off + (line << 2) + (plane ? 2 : 0)) >> 1];
		break;
	}
*/
	hscroll = (0x400 - hscroll) & 0x3ff;
	col_off = hscroll >> 3;
	pix_off = hscroll & 0x7;
	pix_tmp = pix_off;

	row = (line >> 3) * vdp.sc_width;
	//pixrow = line % 8;

	i = 0;
	int vdc3 = (vdp.dis_cells);
	while(i < vdc3)
	{
		uint16_t name_ent = p[row];//row + ((col_off + ((pix_off + i) >> 3)) % vdp.sc_width)];

		if ((name_ent >> 15) == priority)
		{
	//			uint32_t data;
			sint32_t pal, pixel;

			pal = (name_ent >> 9) & 0x30;

			
			
//			if ((name_ent >> 12) & 0x1)
	//		{
				//tile_data = *
				
		//		(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5)
//				+ (28 - (pixrow * 4)));
			//}
			//else
			//{
			//	tile_data = *(uint32_t *)(vdp.vram + ((name_ent & 0x7ff) << 5) + (pixrow * 4));
			//}
for(int j=0;j<32;j++) {
			__n64_memcpy_ASM(a_new_sprite->data + (j*32), vdp.vram + ((name_ent & 0x7ff) << 5), 32);
			
}			data_cache_hit_writeback_invalidate( a_new_sprite->data, 32*32 );


//	rdp_load_palette(0,16,rdp_pal/* + ((pal>>1)*16)*/); // upload starting on palette 0, 15 colors upload, point to the palette struct
		rdp_load_texture(a_new_sprite);
			if ((name_ent >> 11) & 0x1)
	rdp_draw_sprite_scaled(vdc3<<3,y<<3,1.0,1.0,0);
		}
else {
for(int i=0
}	
		}
			i+=4;
	}
//	rdp_load_palette(0,255,pal); // upload starting on palette 0, 15 colors upload, point to the palette struct
}
//free(a_new_sprite);
#endif
}
#endif