/* $Id: z80.h,v 1.2 2002-06-10 22:07:49 jkf Exp $ */

#ifndef _Z80_H_
#define _Z80_H_

#include "z80/z80.h"

extern uint8_t z80_ram[8192];
extern uint32_t z80_bank_base;
extern uint8_t z80_bank_shift;
extern uint8_t z80_running;
extern uint8_t z80_busreq;

extern uint32_t z80init(void);
extern uint32_t z80_enabled(void);
extern void z80_dump_mem(void);
extern UINT8 z80_read_port(UINT32);
extern void z80_write_port(UINT32, UINT8);
extern UINT8 z80_read_mem(UINT32);
extern void z80_write_mem(UINT32, UINT8);

#endif /* _Z80_H_ */
