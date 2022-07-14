#ifndef Z80_H
#define Z80_H

#include "cpuintrf.h"

#define INLINE static __inline__

/*************************************
 *
 *      Z80 daisy chain
 *
 *************************************/

/* daisy-chain link */
typedef struct
{
        void (*reset)(int);                     /* reset callback         */
        int  (*interrupt_entry)(int);   /* entry callback         */
        void (*interrupt_reti)(int);    /* reti callback          */
        int irq_param;                                  /* callback paramater */
} Z80_DaisyChain;

#define Z80_MAXDAISY    4               /* maximum of daisy chan device */

#define Z80_INT_REQ     0x01    /* interrupt request mask               */
#define Z80_INT_IEO     0x02    /* interrupt disable mask(IEO)  */

#define Z80_VECTOR(device,state) (((device)<<8)|(state))


enum {
	Z80_PC=1, Z80_SP, Z80_AF, Z80_BC, Z80_DE, Z80_HL,
	Z80_IX, Z80_IY,	Z80_AF2, Z80_BC2, Z80_DE2, Z80_HL2,
	Z80_R, Z80_I, Z80_IM, Z80_IFF1, Z80_IFF2, Z80_HALT,
	Z80_NMI_STATE, Z80_IRQ_STATE, Z80_DC0, Z80_DC1, Z80_DC2, Z80_DC3
};

enum {
	Z80_TABLE_op,
	Z80_TABLE_cb,
	Z80_TABLE_ed,
	Z80_TABLE_xy,
	Z80_TABLE_xycb,
	Z80_TABLE_ex	/* cycles counts for taken jr/jp/call and interrupt latency (rst opcodes) */
};

extern int z80_ICount;              /* T-state count                        */

extern void z80_init(void);
extern void z80_reset (void *param);
extern void z80_exit (void);
extern int z80_execute(int cycles);
extern void z80_burn(int cycles);
extern unsigned z80_get_context (void *dst);
extern void z80_set_context (void *src);
extern const void *z80_get_cycle_table (int which);
extern void z80_set_cycle_table (int which, void *new_tbl);
extern unsigned z80_get_reg (int regnum);
extern void z80_set_reg (int regnum, unsigned val);
extern void z80_set_irq_line(int irqline, int state);
extern void z80_set_irq_callback(int (*irq_callback)(int));
extern const char *z80_info(void *context, int regnum);
extern unsigned z80_dasm(char *buffer, unsigned pc);
extern UINT8 z80_read_mem(UINT32 addr);
extern void z80_write_mem(UINT32 addr, UINT8 val);
extern UINT8 z80_read_port(UINT32 addr);
extern void z80_write_port(UINT32 addr, UINT8 val);

#endif

