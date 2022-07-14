/* $Id: m68k.h,v 1.1 2002-05-18 19:46:16 jkf Exp $ */

#ifndef _M68K_H_
#define _M68K_H_

//#include "m68k/m68k.h"

/*extern uint32_t m68k_read_memory_8(uint32_t addr);
extern uint32_t m68k_read_memory_16(uint32_t addr);
extern uint32_t m68k_read_memory_32(uint32_t addr);

extern void m68k_write_memory_8(uint32_t addr, uint32_t val);
extern void m68k_write_memory_16(uint32_t addr, uint32_t val);
extern void m68k_write_memory_32(uint32_t addr, uint32_t val);*/


/* Read from anywhere */
extern unsigned int  m68k_read_memory_8(unsigned int address);
extern unsigned int  m68k_read_memory_16(unsigned int address);
extern unsigned int  m68k_read_memory_32(unsigned int address);

/* Write to anywhere */
extern void m68k_write_memory_8(unsigned int address, unsigned int value);
extern void m68k_write_memory_16(unsigned int address, unsigned int value);
extern void m68k_write_memory_32(unsigned int address, unsigned int value);



extern uint8_t* m68k_ram;//[65536];

#endif /* _M68K_H_ */
