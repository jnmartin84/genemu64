/* $Id: z80.c,v 1.5 2002-08-29 19:45:22 jkf Exp $ */

#include <libdragon.h>

#include "gen-emu.h"
#include "cart.h"
#include "vdp.h"
//#include "SN76489.h"

#include "z80.h"
#include "m68k.h"

#define NULL 0

UINT8 z80_read_port(UINT32 addr);
void z80_write_port(UINT32 addr, UINT8 val);
UINT8 z80_read_mem(UINT32 addr);
void z80_write_mem(UINT32 addr, UINT8 val);

uint8_t __attribute__((aligned(8))) z80_ram[0x2000];	/* 8KB */

uint32_t z80_bank_base;
uint8_t z80_bank_shift;
uint8_t z80_running;
uint8_t z80_busreq;

uint32_t z80init(void)
{
	z80_init();

	z80_reset(NULL);

	return 0;
}

UINT8 z80_read_port(UINT32 addr)
{
	return 0xff;
}

void z80_write_port(UINT32 addr, UINT8 val)
{
}

UINT8 z80_read_mem(UINT32 addr)
{
	UINT8 ret = 0xff;
	addr &= 0xffff;

	if (addr < 0x4000) {
		ret = z80_ram[addr & 0x1fff];
	} else
	if (addr >= 0x8000) {
		ret = m68k_read_memory_8(addr & 0x7fff);
	} else
	if ((addr >= 0x7f00) && (addr < 0x7f20)) {
		/* vdp */
		switch(addr & 0x1f) {
		case 0x00:
		case 0x02:
			ret = (vdp_data_read() >> 8);
			break;
		case 0x01:
		case 0x03:
			ret = (vdp_data_read() & 0xff);
			break;
		case 0x04:
		case 0x06:
			ret = (vdp_control_read() >> 8);
			break;
		case 0x05:
		case 0x07:
			ret = (vdp_control_read() & 0xff);
			break;
		case 0x08:
		case 0x0a:
		case 0x0c:
		case 0x0e:
//			printf("hv counter read.\n");
			ret = (vdp_hv_read() >> 8);
			break;
		case 0x09:
		case 0x0b:
		case 0x0d:
		case 0x0f:
//			printf("hv counter read.\n");
			ret = (vdp_hv_read() & 0xff);
			break;
		}
	} else
	if ((addr >= 0x4000) && (addr < 0x6000)) {
		switch(addr & 0x3) {
		case 0x0:
			ret = 0x00;
			break;
		}
	}

//	if (debug)
//		printf("Z80   %04x -> %02x\n", addr, ret);

	return(ret);
}

void z80_write_mem(UINT32 addr, UINT8 val)
{
	addr &= 0xffff;

//	if ((addr == 0) && (val == 2)) {
//		z80_dump_mem();
//		asm("trapa #0x20");
//	}

//	if (debug)
//		printf("Z80   %04x <- %02x\n", addr, val);

	if (addr < 0x4000) {
		z80_ram[addr & 0x1fff] = val;
	}
	else if (addr >= 0x8000) {
		m68k_write_memory_8(addr & 0x7fff, val);
	}
	else if ((addr >= 0x7f00) && (addr <= 0x7f1f)) {
		/* vdp */
			switch(addr & 0x1f) {
			case 0x00:
			case 0x01:
			case 0x02:
			case 0x03:
				vdp_data_write((val << 8) | val);
				break;
			case 0x04:
			case 0x05:
			case 0x06:
			case 0x07:
				vdp_control_write((val << 8) | val);
				break;
/*			case 0x11:
			case 0x13:
			case 0x15:
			case 0x17:
//				Write76489(&PSG,val);
			break;*/
			}
	}
/*	else if ((addr >= 0x4000) && (addr < 0x6000)) {
		switch(addr & 0x3) {
		case 0x0:
			break;
		case 0x1:
			break;
		case 0x2:
			break;
		case 0x3:
			break;
		}
	}*/
	else if ((addr >= 0x6000) && (addr <= 0x60ff)) {
		static uint32_t tmp_bank_base;
		tmp_bank_base |= ((val & 0x01) << z80_bank_shift++);
		if (z80_bank_shift == 9) {
			z80_bank_base = tmp_bank_base << 15;
			z80_bank_shift = 0;
		}
	}
}

uint32_t z80_enabled(void)
{
	return (z80_running && !z80_busreq);
}
