/* $Id: input.c,v 1.4 2002-08-06 16:14:54 jkf Exp $ */

#include <libdragon.h>

#include "gen-emu.h"

static uint32_t controller_present[3];

static uint8_t outputs[3];
static uint8_t outputs_mask[3];

void ctlr_init(void)
{
	int i;

	for(i = 0; i < 3; i++)
	{
		controller_present[i] = 0;
	}

	int cp = get_controllers_present();

	controller_present[0] = cp & CONTROLLER_1_INSERTED;
	controller_present[1] = cp & CONTROLLER_2_INSERTED;

//	printf("%08x\n", cp);
//	printf("%08x %08x %08x %08x\n", cp & CONTROLLER_1_INSERTED, cp & CONTROLLER_2_INSERTED, cp & CONTROLLER_3_INSERTED, cp & CONTROLLER_4_INSERTED);
//	printf("%08x %08x\n", controller_present[0], controller_present[1]);
//	while(1);
}

void ctlr_reset(void)
{
	int i;

	for(i = 0; i < 3; i++)
		outputs[i] = outputs_mask[i] = 0;
}
//extern int MULTITEX;

uint8_t ctlr_data_reg_read(int port)
{
	uint8_t ret = 0;

	if(controller_present[port]) {
		controller_scan();
		struct controller_data keys_pressed = get_keys_held();
		struct SI_condat pressed = keys_pressed.c[port];

//		if (pressed.Z) {
//			MULTITEX=!MULTITEX;
//		}

		if (outputs[port] & 0x40) {
			ret |= (pressed.C_down) ? 0 : 0x20;
			ret |= (pressed.A) ? 0 : 0x10;
			ret |= (pressed.right) ? 0 : 0x08;
			ret |= (pressed.left) ? 0 : 0x04;
			ret |= (pressed.down) ? 0 : 0x02;
			ret |= (pressed.up) ? 0 : 0x01;
		} else {
			ret |= (pressed.start) ? 0 : 0x20;
			ret |= (pressed.B) ? 0 : 0x10;
			ret |= (pressed.down) ? 0 : 0x02;
			ret |= (pressed.up) ? 0 : 0x01;
		}
	}
	else {
		ret = (~outputs_mask[port]) & 0x7f;
		ret |= outputs[port];
	}

	return ret;
}

void ctlr_data_reg_write(int port, uint8_t val)
{
	outputs[port] = val & (outputs_mask[port] | 0x80);
}

uint8_t ctlr_ctrl_reg_read(int port)
{
	return outputs_mask[port];
}

void ctlr_ctrl_reg_write(int port, uint8_t val)
{
	outputs_mask[port] = val;
}
