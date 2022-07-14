#include <libdragon.h>

#include "gen-emu.h"
#include "vdp.h"

extern struct vdp_s vdp;

/* PVR data from vdp.c */
/*extern pvr_poly_hdr_t disp_hdr[2];
extern pvr_ptr_t disp_txr[2];
extern pvr_ptr_t display_txr;
extern pvr_poly_hdr_t cram_hdr;
extern pvr_ptr_t cram_txr;*/
extern uint8_t display_ptr;

void do_frame()
{
/*	pvr_vertex_t vert;*/
	int x, y, w, h;

/*	vert.argb = 0xffffffff;
	vert.oargb = 0x00000000;
	vert.z = 1;

	display_txr = disp_txr[display_ptr];
	display_ptr = (display_ptr ? 0 : 1);

	pvr_wait_ready();
	pvr_scene_begin();
	pvr_list_begin(PVR_LIST_OP_POLY);*/

	/* Main display */
#if 0
	x = 25; y = 25;
	w = 320; h = 240;
#else
	x = 0; y = 0;
	w = 640; h = 480;
#endif

/*	pvr_prim(&disp_hdr[display_ptr], sizeof(pvr_poly_hdr_t));
	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y + h;
	vert.u = 0.0f;
	vert.v = 240.0f/256.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + w;
	vert.y = y + h;
	vert.u = ((vdp.regs[12] & 0x01) ? 320.0f : 256.0f)/512.0f;
	vert.v = 240.0f/256.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));
*/
#if 0
	/* CRAM display */
	x = 550; y = 25;
	w = 64; h = 64;

/*	pvr_prim(&cram_hdr, sizeof(cram_hdr));
	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y + h;
	vert.u = 0.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + w;
	vert.y = y + h;
	vert.u = 1.0f;
	vert.v = 1.0f;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y = y;
	vert.v = 0.0f;
	pvr_prim(&vert, sizeof(vert));*/
#endif

/*	pvr_list_finish();
	pvr_scene_finish();*/
}
