#include <libdragon.h>

#include "gen-emu.h"
#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#include "input.h"

void gen_init(void)
{
//	vdp_init();
	ctlr_init();


//printf("finished gen_init()\n");
}

void gen_reset(void)
{
	m68k_pulse_reset();
//	z80_init();

	ctlr_reset();
//printf("finished gen_reset()\n");

}
