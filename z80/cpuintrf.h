/***************************************************************************

	cpuintrf.h

	Core CPU interface functions and definitions.

***************************************************************************/

#ifndef CPUINTRF_H
#define CPUINTRF_H

#include "osd_cpu.h"

#ifdef __cplusplus
extern "C" {
#endif


/*************************************
 *
 *	Interrupt line constants
 *
 *************************************/

enum
{
	/* line states */
	CLEAR_LINE = 0,				/* clear (a fired, held or pulsed) line */
	ASSERT_LINE,				/* assert an interrupt immediately */
	HOLD_LINE,					/* hold interrupt line until acknowledged */
	PULSE_LINE,					/* pulse interrupt line for one instruction */

	/* internal flags (not for use by drivers!) */
	INTERNAL_CLEAR_LINE = 100 + CLEAR_LINE,
	INTERNAL_ASSERT_LINE = 100 + ASSERT_LINE,

	/* interrupt parameters */
	MAX_IRQ_LINES =	16,			/* maximum number of IRQ lines per CPU */
	IRQ_LINE_NMI = 127			/* IRQ line for NMIs */
};



/*************************************
 *
 *	CPU information constants
 *
 *************************************/

/* get_reg/set_reg constants */
enum
{
	MAX_REGS = 128,				/* maximum number of register of any CPU */

	/* This value is passed to activecpu_get_reg to retrieve the previous
	 * program counter value, ie. before a CPU emulation started
	 * to fetch opcodes and arguments for the current instrution. */
	REG_PREVIOUSPC = -1,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * program counter value. */
	REG_PC = -2,

	/* This value is passed to activecpu_get_reg to retrieve the current
	 * stack pointer value. */
	REG_SP = -3,

	/* This value is passed to activecpu_get_reg/activecpu_set_reg, instead of one of
	 * the names from the enum a CPU core defines for it's registers,
	 * to get or set the contents of the memory pointed to by a stack pointer.
	 * You can specify the n'th element on the stack by (REG_SP_CONTENTS-n),
	 * ie. lower negative values. The actual element size (UINT16 or UINT32)
	 * depends on the CPU core. */
	REG_SP_CONTENTS = -4
};


/* endianness constants */
enum
{
	CPU_IS_LE = 0,				/* emulated CPU is little endian */
	CPU_IS_BE					/* emulated CPU is big endian */
};


/* Values passed to the cpu_info function of a core to retrieve information */
enum
{
	CPU_INFO_REG,
	CPU_INFO_FLAGS = MAX_REGS,
	CPU_INFO_NAME,
	CPU_INFO_FAMILY,
	CPU_INFO_VERSION,
	CPU_INFO_FILE,
	CPU_INFO_CREDITS,
	CPU_INFO_REG_LAYOUT,
	CPU_INFO_WIN_LAYOUT
};


#ifdef __cplusplus
}
#endif

#endif	/* CPUINTRF_H */

