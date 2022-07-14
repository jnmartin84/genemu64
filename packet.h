#ifndef __RSP_STRUCT_H
#define __RSP_STRUCT_H

/*
datau[0] = (uint32_t)(&datau[0]) & 0x07fffff8;
datau[1] = (uint32_t)rsp_sprite_data & 0x07fffff8;
datau[2] = (uint32_t)rsp_addr0 & 0x07fffff8;
datau[3] = rsp_pn0;
datau[4] = rsp_vf0;
datau[5] = rsp_hf0;
datau[6] = (uint32_t)rsp_addr1 & 0x07fffff8;
datau[7] = rsp_pn1;
datau[8] = rsp_vf1;
datau[9] = rsp_hf1;

datau[10] = 1;
datau[11] = 0;
*/

typedef struct __attribute__((__packed__)) rsp_tile_packet_s {
	void*	dram_this_ptr;
	void* sprite_data_ptr;
	
	void*	tile0_ptr;
	uint32_t pn0;
	uint32_t vf0;
	uint32_t hf0;

	void*	tile1_ptr;
	uint32_t pn1;
	uint32_t vf1;
	uint32_t hf1;

	uint32_t new_request;
	uint32_t finished;
	
} rsp_tile_packet_t;


#endif