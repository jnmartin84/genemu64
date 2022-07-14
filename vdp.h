/* $Id: vdp.h,v 1.7 2002-08-07 04:01:23 jkf Exp $ */

#ifndef __gen_vdp_h
#define __gen_vdp_h

#include "gen-emu.h"

//struct vdp_s
//{

extern	uint8_t*  vdp_vram;
/*extern	uint16_t vdp_cram[64];
extern	uint16_t vdp_vsram[64];		// Only first 40 used. rest are address padding. 
*/
extern	uint16_t* vdp_dc_cram;	// cram in dc format 
extern	uint8_t*  vdp_regs;		// Only first 24 used, rest are address padding. 
extern	uint16_t vdp_addr;
/*
extern	uint32_t vdp_control;
extern	uint16_t *vdp_bga;
extern	uint16_t *vdp_bgb;
extern	uint16_t *vdp_wnd;
extern	uint64_t *vdp_sat;

extern	uint16_t vdp_status;
extern	uint16_t vdp_scanline;
extern	uint16_t vdp_hv;

extern	uint16_t vdp_hs_off;
extern	uint8_t vdp_code;
extern	uint8_t vdp_h_int_counter;
extern	uint8_t vdp_write_pending;
extern	uint8_t vdp_sc_width;
extern	uint8_t vdp_sc_height;
extern	uint8_t vdp_dis_cells;
//};
*/

uint16_t vdp_control_read(void);
uint16_t vdp_data_read(void);
void vdp_control_write(uint16_t);
void vdp_data_write(uint16_t);
void vdp_interrupt(int line);
uint16_t vdp_hv_read(void);
void vdp_init(void);
void vdp_render_cram(void);
void vdp_render_scanline(int);

#endif // __gen_vdp_h
