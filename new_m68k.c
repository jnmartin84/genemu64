/* $Id: m68k.c,v 1.12 2002-08-30 19:39:39 jkf Exp $ */

#include <libdragon.h>

#include "gen-emu.h"
#include "cart.h"

#include "m68k.h"
#include "z80.h"

#include "vdp.h"
#include "input.h"

#define NULL 0

uint8_t __attribute__((aligned(8))) m68k_ram[65536];
uint16_t *m68k_ram16 = (uint16_t *)m68k_ram;
uint8_t bank_sram = 0;

static unsigned int m68k_read_whatever_memory_32(unsigned int addr);
void m68k_write_memory_32(unsigned int addr, unsigned int val);

static unsigned int m68k_read_cart_32(unsigned int addr);
static unsigned int m68k_read_unused_32(unsigned int addr);
static unsigned int m68k_read_vdp_32(unsigned int addr);
static unsigned int x32(unsigned int addr);
static unsigned int m68k_read_ram_32(unsigned int addr);

static unsigned int __attribute__((aligned(8))) (*readmem32[16])(unsigned int addr) = {
	m68k_read_cart_32,m68k_read_cart_32,m68k_read_cart_32,m68k_read_cart_32,

	m68k_read_unused_32,m68k_read_unused_32,m68k_read_unused_32,m68k_read_unused_32,
	m68k_read_unused_32,m68k_read_unused_32,

	x32,x32,

	m68k_read_vdp_32,m68k_read_vdp_32,

	m68k_read_ram_32,m68k_read_ram_32
};

static unsigned int m68k_read_cart_16(unsigned int addr);
static unsigned int m68k_read_unused_16(unsigned int addr);
static unsigned int m68k_read_vdp_16(unsigned int addr);
static unsigned int x16(unsigned int addr);
static unsigned int m68k_read_ram_16(unsigned int addr);

static unsigned int __attribute__((aligned(8))) (*readmem16[16])(unsigned int addr) = {
	m68k_read_cart_16,m68k_read_cart_16,m68k_read_cart_16,m68k_read_cart_16,

	m68k_read_unused_16,m68k_read_unused_16,m68k_read_unused_16,m68k_read_unused_16,
	m68k_read_unused_16,m68k_read_unused_16,

	x16,x16,

	m68k_read_vdp_16,m68k_read_vdp_16,

	m68k_read_ram_16,m68k_read_ram_16
};

static unsigned int m68k_read_cart_8(unsigned int addr);
static unsigned int m68k_read_unused_8(unsigned int addr);
static unsigned int m68k_read_vdp_8(unsigned int addr);
static unsigned int x8(unsigned int addr);
static unsigned int m68k_read_ram_8(unsigned int addr);

static unsigned int __attribute__((aligned(8))) (*readmem8[16])(unsigned int addr) = {
	m68k_read_cart_8,m68k_read_cart_8,m68k_read_cart_8,m68k_read_cart_8,

	m68k_read_unused_8,m68k_read_unused_8,m68k_read_unused_8,m68k_read_unused_8,
	m68k_read_unused_8,m68k_read_unused_8,

	x8,x8,

	m68k_read_vdp_8,m68k_read_vdp_8,

	m68k_read_ram_8,m68k_read_ram_8
};

unsigned int m68k_read_memory_32(unsigned int addr) {
if(addr < 0x400000) {
	if(!(addr & 0x3)) {
		return *(uint32_t *)(cart.rom + addr);
	}
	return m68k_read_cart_32(addr);
}
else if(addr >= 0xe00000) {
	if(!(addr & 0x3)) {
		return *(uint32_t *)(m68k_ram + (addr&0xffff));
	}
		return m68k_read_ram_32(addr);
}
	return (*readmem32 [((addr & 0x00FFFFFF) >> 20)] )(addr);
}

static unsigned int x32(unsigned int addr) {
//	return m68k_read_whatever_memory_32(addr);
	uint32_t ret;

	ret = (x16(addr) << 16);
	ret |= x16(addr+2);

	return(ret);
}

static unsigned int m68k_read_vdp_32(unsigned int addr) {
	uint32_t ret;

	ret = (m68k_read_vdp_16(addr) << 16);
	ret |= m68k_read_vdp_16(addr+2);

	return(ret);
//	return m68k_read_whatever_memory_32(addr);
}

static unsigned int m68k_read_unused_32(unsigned int addr) {
	return 0xFFFFFFFF;
}

static unsigned int m68k_read_cart_32(unsigned int addr) {
	if(!(addr & 0x3)) {
		return *(uint32_t *)(cart.rom + addr);
	}
	else {
	uint32_t ret;

	ret = (m68k_read_cart_16(addr) << 16);
	ret |= m68k_read_cart_16(addr+2);

	return(ret);
	}
}

static unsigned int m68k_read_ram_32(unsigned int addr) {
	if(!(addr & 0x3)) {
		return *(uint32_t *)(m68k_ram + (addr&0xffff));
	}
	else {
	uint32_t ret;

	ret = (m68k_read_ram_16(addr) << 16);
	ret |= m68k_read_ram_16(addr+2);

	return(ret);
	}
}

unsigned int m68k_read_memory_16(unsigned int addr) {
if (addr < 0x400000) {
		return ((uint16_t *)cart.rom)[addr >> 1];
	} else
	if (addr >= 0xe00000) {
//		returm68k_ram[addr & 0xffff] = val;
return m68k_ram16[(addr & 0xffff) >> 1];
	} 
	else	return (*readmem16 [((addr & 0x00FFFFFF) >> 20)] )(addr);
}

static unsigned int x16(unsigned int addr) {

        uint16_t ret = 0xFFFF;

	if ((addr >= 0xa00000) && (addr <= 0xa0ffff)) {
		if (z80_running && z80_busreq) {
			ret = z80_read_mem(addr & 0x7fff);
			ret |= ret << 8;
		}
	} else
	if ((addr >= 0xa10000) && (addr <= 0xa1001f)) {
		/* Controller I/O addresses. */
		switch(addr & 0x1f) {
		case 0x00:
			ret = 0xa0a0;
			break;

		case 0x02:		/* Data reg, port A */
			ret = ctlr_data_reg_read(0);
			ret |= ret << 8;
			break;
		case 0x04:		/* Data reg, port B */
			ret = ctlr_data_reg_read(1);
			ret |= ret << 8;
			break;
		case 0x06:		/* Data reg, port C */
			ret = ctlr_data_reg_read(2);
			ret |= ret << 8;
			break;

		case 0x08:		/* Cntl reg, port A */
			ret = ctlr_ctrl_reg_read(0);
			ret |= ret << 8;
			break;
		case 0x0a:		/* Cntl reg, port B */
			ret = ctlr_ctrl_reg_read(1);
			ret |= ret << 8;
			break;
		case 0x0c:		/* Cntl reg, port C */
			ret = ctlr_ctrl_reg_read(2);
			ret |= ret << 8;
			break;

		/* Port A */
		case 0x0e:		/* TxData reg */
			ret = 0xffff;
			break;
		case 0x10:		/* RxData reg */
			ret = 0x0000;
			break;
		case 0x12:		/* S-Ctrl reg */
			ret = 0x0000;
			break;

		/* Port B */
		case 0x14:		/* TxData reg */
			ret = 0xffff;
			break;
		case 0x16:		/* RxData reg */
			ret = 0x0000;
			break;
		case 0x18:		/* S-Ctrl reg */
			ret = 0x0000;
			break;

		/* Port C */
		case 0x1a:		/* TxData reg */
			ret = 0xffff;
			break;
		case 0x1c:		/* RxData reg */
			ret = 0x0000;
			break;
		case 0x1e:		/* S-Ctrl reg */
			ret = 0x0000;
			break;
		}
	} else
	switch(addr) {
	case 0xa11100:
		ret = (!(z80_busreq && z80_running)) << 8;
		break;
	case 0xa11200:
		ret = z80_running << 8;
		break;
	default:
		printf("Unhandled memory read from %06x\n", addr);
		quit = 1;
	}

	return ret;
}

static unsigned int m68k_read_vdp_16(unsigned int addr) {

	unsigned int ret = 0xFFFF;

 // vdp  & psg
                if ((addr & 0xe700e0) == 0xc00000) {
                        switch(addr & 0x1f) {
                        case 0x00:
                        case 0x02:
                                ret = vdp_data_read();
                                break;
                        case 0x04:
                        case 0x06:
                                ret = vdp_control_read();
                                break;
                        case 0x08:
                        case 0x0a:
                        case 0x0c:
                        case 0x0e:
                                ret = vdp_hv_read();
                                break;
                        }
                }

	return ret;
}

static unsigned int m68k_read_unused_16(unsigned int addr) {
	return 0xFFFF;
}

static unsigned int m68k_read_cart_16(unsigned int addr) {

        uint16_t ret = 0xFFFF;
/*
	if ((cart.sram_len > 0) && ((addr >= cart.sram_start) && (addr <= cart.sram_end)))
	{
		if (!cart.sram_banked) {
                	ret = ((uint16_t *)cart.sram)[(addr & (cart.sram_len - 1)) >> 1];
                }
		else
		{
			if (!bank_sram) {
				ret = ((uint16_t *)cart.rom)[addr >> 1];
			}
			else
			{
	                	ret = ((uint16_t *)cart.sram)[(addr & (cart.sram_len - 1)) >> 1];
			}
	        }
        }
	else*/ {
		ret = ((uint16_t *)cart.rom)[addr >> 1];
        }

	return ret;
}

static unsigned int m68k_read_ram_16(unsigned int addr) {
	return m68k_ram16[(addr & 0xffff) >> 1];
}

unsigned int m68k_read_memory_8(unsigned int addr) {
	return (*readmem8 [((addr & 0x00FFFFFF) >> 20)] )(addr);
}

static unsigned int x8(unsigned int addr) {

    uint8_t ret = 0xFF;

	if ((addr >= 0xa00000) && (addr <= 0xa0ffff)) {
		if (z80_running && z80_busreq)
			ret = z80_read_mem(addr & 0x7fff);
	} else
	if ((addr >= 0xa10000) && (addr <= 0xa1001f)) {
		/* Controller I/O addresses. */
		switch(addr & 0x1f) {
		case 0x00:
		case 0x01:
			ret = 0xa0;
			break;

		case 0x02:
		case 0x03:		/* Data reg, port A */
			ret = ctlr_data_reg_read(0);
			break;
		case 0x04:
		case 0x05:		/* Data reg, port B */
			ret = ctlr_data_reg_read(1);
			break;
		case 0x06:
		case 0x07:		/* Data reg, port C */
			ret = ctlr_data_reg_read(2);
			break;

		case 0x08:
		case 0x09:		/* Cntl reg, port A */
			ret = ctlr_ctrl_reg_read(0);
			break;
		case 0x0a:
		case 0x0b:		/* Cntl reg, port B */
			ret = ctlr_ctrl_reg_read(1);
			break;
		case 0x0c:
		case 0x0d:		/* Cntl reg, port C */
			ret = ctlr_ctrl_reg_read(2);
			break;

		/* Port A */
		case 0x0e:
		case 0x0f:		/* TxData reg */
			ret = 0xff;
			break;
		case 0x10:
		case 0x11:		/* RxData reg */
			ret = 0x00;
			break;
		case 0x12:
		case 0x13:		/* S-Ctrl reg */
			ret = 0x00;
			break;

		/* Port B */
		case 0x14:
		case 0x15:		/* TxData reg */
			ret = 0xff;
			break;
		case 0x16:
		case 0x17:		/* RxData reg */
			ret = 0x00;
			break;
		case 0x18:
		case 0x19:		/* S-Ctrl reg */
			ret = 0x00;
			break;

		/* Port C */
		case 0x1a:
		case 0x1b:		/* TxData reg */
			ret = 0xff;
			break;
		case 0x1c:
		case 0x1d:		/* RxData reg */
			ret = 0x00;
			break;
		case 0x1e:
		case 0x1f:		/* S-Ctrl reg */
			ret = 0x00;
			break;
		}
	} else
	switch(addr) {
	case 0xa11100:
		ret = !(z80_busreq && z80_running);
		break;
	case 0xa11200:
		ret = z80_running;
		break;
	default:
		printf("Unhandled memory read from %06x\n", addr);
		quit = 1;
	}

	return ret;
}

static unsigned int m68k_read_vdp_8(unsigned int addr) {

	unsigned int ret = 0xFF;
	if ((addr & 0xe700e0) == 0xc00000) {
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
				ret = (vdp_hv_read() >> 8);
				break;
			case 0x09:
			case 0x0b:
			case 0x0d:
			case 0x0f:
				ret = (vdp_hv_read() & 0xff);
				break;
			}
		}
	return ret;
}

static unsigned int m68k_read_unused_8(unsigned int addr) {
	return 0xFF;
}

static unsigned int m68k_read_cart_8(unsigned int addr) {

        uint16_t ret = 0xFF;

/*/(	if ((cart.sram_len > 0) &&
			((addr >= cart.sram_start) &&
			 (addr <= cart.sram_end))) {
			if (!cart.sram_banked) {
				ret = cart.sram[addr & (cart.sram_len - 1)];
			} else {
				if (!bank_sram) {
					ret = cart.rom[addr];
				} else {
					ret = cart.sram[addr & (cart.sram_len - 1)];
				}
			}
		} else*/ {
			ret = cart.rom[addr];
		}

	return ret;
}

static unsigned int m68k_read_ram_8(unsigned int addr) {

	return m68k_ram[addr & 0xffff];
}

static unsigned int m68k_read_whatever_memory_32(unsigned int addr)
{
	uint32_t ret;

	ret = (m68k_read_memory_16(addr) << 16);
	ret |= m68k_read_memory_16(addr+2);

	return(ret);
}

//
//
//
// WRITE
//
//
//

void m68k_write_memory_8(unsigned int addr, unsigned int val)
{
	val &= 0xff;

	if (debug)
		printf("M68K  %06x <- %02x\n", addr, val);

	if (addr < 0x400000) {
		if ((cart.sram_len > 0) &&
			((addr >= cart.sram_start) &&
			 (addr <= cart.sram_end))) {
			if (!cart.sram_banked) {
				cart.sram[addr & (cart.sram_len - 1)] = val;
			} else {
				if (bank_sram) {
					cart.sram[addr & (cart.sram_len - 1)] = val;
				}
			}
		}
	} else
	if (addr >= 0xe00000) {
		m68k_ram[addr & 0xffff] = val;
	} else
	if ((addr >= 0xc00000) && (addr <= 0xdfffff)) {
		/* vdp & psg */
		if ((addr & 0xe700e0) == 0xc00000) {
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
	} else
	if ((addr >= 0xa00000) && (addr <= 0xa0ffff)) {
		if (z80_running && z80_busreq)
			z80_write_mem(addr & 0x7fff, val & 0xff);
	} else
	if ((addr >= 0xa10000) && (addr <= 0xa1001f)) {
		/* Controller I/O addresses. */
		switch(addr & 0x1f) {
		case 0x02:
		case 0x03:		/* Data reg, port A */
			ctlr_data_reg_write(0, val);
			break;
		case 0x04:
		case 0x05:		/* Data reg, port B */
			ctlr_data_reg_write(1, val);
			break;
		case 0x06:
		case 0x07:		/* Data reg, port C */
			ctlr_data_reg_write(2, val);
			break;

		case 0x08:
		case 0x09:		/* Cntl reg, port A */
			ctlr_ctrl_reg_write(0, val);
			break;
		case 0x0a:
		case 0x0b:		/* Cntl reg, port B */
			ctlr_ctrl_reg_write(1, val);
			break;
		case 0x0c:
		case 0x0d:		/* Cntl reg, port C */
			ctlr_ctrl_reg_write(2, val);
			break;
		}
	} else
	switch(addr) {
	case 0xa11100:
		z80_busreq = val & 0x01;
		break;
	case 0xa11200:
		z80_running = val & 0x01;
		if (z80_running) {
//			if (!z80_busreq)
//				z80_dump_mem();
			z80_reset(NULL);
			/* XXX reset ym2612 */
		}
		break;
	case 0xa130f1:
		bank_sram = val & 0x01;
		break;
	case 0xa130f3:
	case 0xa130f5:
	case 0xa130f7:
	case 0xa130f9:
	case 0xa130fb:
	case 0xa130fd:
	case 0xa130ff:
		if (cart.banked)
			cart.banks[(addr & 0xe) >> 1] = (uint32_t)(cart.rom + (val << 19));
		break;
	default:
		printf("Unhandled memory write to %06x, value %02x\n", addr, val);
		quit = 1;
	}
}

void m68k_write_memory_16(unsigned int addr, unsigned int val)
{
	val &= 0xffff;

	if (debug)
		printf("M68K  %06x <- %04x\n", addr, val);

	if (addr < 0x400000) {
		if ((cart.sram_len > 0) &&
			((addr >= cart.sram_start) &&
			 (addr <= cart.sram_end))) {
			if (!cart.sram_banked) {
//				val = SWAPBYTES16(val);
				((uint16_t *)cart.sram)[(addr & (cart.sram_len - 1)) >> 1] = val;
			} else {
				if (bank_sram) {
//					val = SWAPBYTES16(val);
					((uint16_t *)cart.sram)[(addr & (cart.sram_len - 1)) >> 1] = val;
				}
			}
		}
	} else
	if (addr >= 0xe00000) {
//		val = SWAPBYTES16(val);
		m68k_ram16[(addr & 0xffff) >> 1] = val;
	} else
	if ((addr >= 0xc00000) && (addr <= 0xdfffff)) {
		/* vdp & psg */
		if ((addr & 0xe700e0) == 0xc00000) {
			switch(addr & 0x1f) {
			case 0x00:
			case 0x02:
				vdp_data_write(val);
				break;
			case 0x04:
			case 0x06:
				vdp_control_write(val);
				break;
/*			case 0x10:
			case 0x12:
			case 0x14:
			case 0x16:
//				Write76489(&PSG,val&0xff);
				break;*/
			}
		}
	} else
	if ((addr >= 0xa00000) && (addr <= 0xa0ffff)) {
		if (z80_running && z80_busreq)
			z80_write_mem(addr & 0x7fff, (val >> 8));
	} else
	if ((addr >= 0xa10000) && (addr <= 0xa1001f)) {
		/* I/O write */
	} else
	switch(addr) {
	case 0xa11100:
		z80_busreq = (val >> 8) & 0x01;
		break;
	case 0xa11200:
		z80_running = (val >> 8) & 0x01;
		if (z80_running) {
			if (!z80_busreq) {
//				z80_dump_mem();
			}
			z80_reset(NULL);
			/* XXX reset ym2612 */
		}
		break;
	case 0xa130f2:
	case 0xa130f4:
	case 0xa130f6:
	case 0xa130f8:
	case 0xa130fa:
	case 0xa130fc:
	case 0xa130fd:
		if (cart.banked)
			cart.banks[(addr & 0xe) >> 1] = (uint32_t)(cart.rom + ((val & 0xff) << 19));
		break;
	default:
		printf("Unhandled memory write to %06x, value %04x\n", addr, val);
		quit = 1;
	}
}

void m68k_write_memory_32(unsigned int addr, unsigned int val)
{
	m68k_write_memory_16(addr, (val >> 16));
	m68k_write_memory_16(addr + 2, (val & 0xffff));
}