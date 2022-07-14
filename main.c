#include <libdragon.h>
#include <stdio.h>
#include <inttypes.h>

#include "gen-emu.h"
#include "m68k.h"
#include "z80.h"
#include "vdp.h"
#define high 0

extern void *__safe_buffer[];

// MIPS addresses
    #define KSEG0 0x80000000
    #define KSEG1 0xA0000000

    // Memory translation stuff
    #define	PHYS_TO_K1(x)       ((uint32_t)(x)|KSEG1)
    #define	IO_WRITE(addr,data) (*(volatile uint32_t *)PHYS_TO_K1(addr)=(uint32_t)(data))
    #define	IO_READ(addr)       (*(volatile uint32_t *)PHYS_TO_K1(addr))
#define RSP_PC 0x04080000 

extern display_context_t lockVideo(int wait);
extern void unlockVideo(display_context_t dc);
extern display_context_t _dc;
extern void TLB_INIT(void);
//char *romname = "/TOEJAM.BIN";
char *romname = "/S1BUILT.BIN";
//char *romname = "/SONIC2.BIN";
//char *romname = "/MM.BIN";
//char *romname = "/EWJ.BIN";
extern int m68k_execute(int num_cycles);

//char *romname = "/MEGABOMB.BIN";
//char *romname = "/SONIC3.BIN";
//char *romname = "/ABEAST.BIN";
//char *romname = "/BALLZ.BIN";
//char *romname = "/GUNSTAR.BIN";
//char *romname = "/MK.BIN";
//char *romname = "/ROADRASH.BIN";
//char *romname = "/UMK3.BIN";
//char *romname = "/VECTOR2.BIN";
//char *romname = "/SONIC1.BIN";
//char *romname = "/VECTRMAN.BIN";
//char *romname = "/EWJ2.BIN";
//char *romname = "/SONIC3D.BIN";
//char *romname = "/TOEJAM2.BIN";
//char *romname = "/WOLF.BIN";
//char *romname = "/EWJ.BIN";

uint8_t debug = 0;
uint8_t quit = 0;
uint8_t dump = 0;
uint8_t pause = 0;

uint32_t rom_load(char *name);
void rom_free(void);
void run_one_field(void);
void gen_init(void);
void gen_reset(void);


#define FIELD_SKIP 2

uint32_t field_count = 0;

uint64_t get_elapsed_seconds()
{
    return (uint64_t)(get_ticks() / COUNTS_PER_SECOND);
}

uint64_t start_time;
uint64_t end_time;
uint64_t total_cycles;

//#define NOVDP
//#define NORDP

int32_t current_cycles;

//extern struct vdp_s vdp;

//extern uint32_t SWAP;

void busy_wait(int seconds)
{
	start_time = get_elapsed_seconds();

	int ticker = 0;

	while(get_elapsed_seconds() - start_time < seconds)
	{
		ticker++;
	}
}
extern uint8_t m68k_ram2[];
extern uint8_t *m68k_ram;
extern uint16_t *m68k_ram16;
int main(int argc, char *argv[])
{
	init_interrupts();
	rdp_init();
	controller_init();
#if high
	display_init(RESOLUTION_640x480, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
#else 
	display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
#endif
	console_init();

	if (dfs_init( DFS_DEFAULT_LOCATION ) != DFS_ESUCCESS)
	{
		printf("Could not initialize filesystem!\n");
		while(1);
	}

	m68k_ram = (uint8_t*)((uintptr_t)m68k_ram2 | 0xA0000000);
	//(0x00E00000);
	m68k_ram16 = (uint16_t *)((uintptr_t)m68k_ram2 | 0xA0000000);
	
	printf("GEN-EMU 64 by JNMARTIN84 and SAYTEN\n");
	printf("https://github.com/jnmartin84/\n");
	printf("built %s %s\n", __DATE__, __TIME__);

//	printf("%02x %02x\n", *(uint8_t *)(0x00e00000),
//	*(uint8_t *)(0x00e00000));
	//*((uint8_t*)(m68k_ram)));
	
	int available_memory_size = *(int *)(0x80000318);

	if(available_memory_size != 0x800000)
	{
	        printf("\n***********************************\n");
	        printf("Expansion Pak not found.\n");
	        printf("It is required to run 64Doom.\n");
	        printf("Please turn off the Nintendo 64,\ninstall Expansion Pak,\nand try again.\n");
	        printf("***********************************\n");
	        return -1;
	}

//	while(1) {}
	gen_init();

	rom_load(romname);
	TLB_INIT();

/*	// get time at start
	start_time = get_elapsed_seconds();

	int ticker = 0;

	while(get_elapsed_seconds() - start_time < 5) {

		if(ticker % (1024*12) == 0)
		{
			printf(".");
		}

		ticker++;
	}
	printf("\n");*/
	vdp_init();
	gen_reset();

	// get time at start
	start_time = get_elapsed_seconds();
	
	do {
	run_one_field();
	} while (!quit);
//	} while ((127856 * (field_count*FIELD_SKIP)) < (1048576 * 20 * 60));
//display_close();
	//display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
	//console_init();

	// get time at end
	end_time = get_elapsed_seconds();

	// multiply field_count by 488 by 262 lines -- total cycles emulated
	total_cycles = (127856 * field_count) ;

	// divide (total cycles emulated) by (end - start)
	double emulated_MHz = (total_cycles / 1048576.0) / (end_time - start_time);

	printf("Elapsed seconds: %lld\n", (end_time - start_time));
	printf("Total cycles: %lld\n", total_cycles);
	printf("Emulated MHz: %f\n", emulated_MHz);
while(1) {
	//printf(
}
	rom_free();

	return 0;
}

// enable palette (tlut)
#define EN_TLUT 0x00800000000000
// enable atomic prim, 1st primitive bandwitdh save
#define ATOMIC_PRIM 0x80000000000000
// enable perspective correction
#define PERSP_TEX_EN 0x08000000000000
// select alpha dither
#define ALPHA_DITHER_SEL_PATTERN 0x00000000000000
#define ALPHA_DITHER_SEL_PATTERNB 0x00001000000000
#define ALPHA_DITHER_SEL_NOISE 0x00002000000000
#define ALPHA_DITHER_SEL_NO_DITHER 0x00003000000000
// select rgb dither
#define RGB_DITHER_SEL_MAGIC_SQUARE_MATRIX 0x00000000000000
#define RGB_DITHER_SEL_STANDARD_BAYER_MATRIX 0x00004000000000
#define RGB_DITHER_SEL_NOISE 0x00008000000000
#define RGB_DITHER_SEL_NO_DITHER 0x0000C000000000
// enable texture filtering
#define SAMPLE_TYPE 0x00200000000000
uint64_t RDP_CONFIG = ATOMIC_PRIM | SAMPLE_TYPE | ALPHA_DITHER_SEL_NO_DITHER | RGB_DITHER_SEL_NO_DITHER;	
#define HLE 0
extern int HLE_running;
extern int HLE_cycles_remaining;
int in_vdp_interrupt = 1;
int fine_grained_cycles = 0;
extern int kos_cycles_used;
extern int nem_cycles_used;
int rect_per_frame;
int wb_per_frame;
int texload_per_frame;
int code_started = 0;
void run_one_field(void)
{
	const uint32_t field_skip = //!((field_count %1) ==0 || (field_count %2) == 0);//
	(field_count % FIELD_SKIP);// == 0;
	uint32_t line;
//rect_per_frame = 0;
//wb_per_frame = 0;
//texload_per_frame = 0;
#ifndef NOVDP
	if(field_skip == 0) 
	{
#ifndef NORDP
		_dc = lockVideo(1);
//	IO_WRITE(0x04000000 + 768, (uintptr_t)__safe_buffer[(_dc)-1]);
		
		rdp_attach_display(_dc);
		//rdp_sync(SYNC_PIPE);
		rdp_set_clipping(0,0,320+(320*high),224);//240);//224 + (224*high));
		rdp_enable_primitive_fill();
		uint16_t color;
//		if(field_count > 10)
//		{
//		}
//		else
//		{
//			rdp_set_fill_color(0x88, 0x88, 0x88, 0xff);
//			color = graphics_make_color(0x88, 0x88, 0x88, 0xff)
//		}
//		rdp_set_fill_color((color>>11)&0xff, (color>>6)&0xff, (color >>1)&0xff, 0xff);
//		rdp_set_fill_color(0xff, 0xff, 0xff, 0xff);
		//rect_per_frame++;
	//	rdp_sync(SYNC_PIPE);
#endif	
		
		if(field_count > 1)
		{
			color = vdp_dc_cram[vdp_regs[7] & 0x3f];
			rdp_set_fill_color((color>>11)&0xff, (color>>6)&0xff, (color >>1)&0xff, 0xff);
		rdp_draw_filled_rectangle(0,0,319+(320*high),223 + (224*high));
//			if(!code_started) {
//				copy_code_to_rsp();
//				code_started = 1;
//			}
#ifndef NORDP
//	rdp_texture_copy(RDP_CONFIG);// | SAMPLE_TYPE);
	rdp_texture_copy(RDP_CONFIG | EN_TLUT);// | SAMPLE_TYPE);

#endif
			render_a_whole_plane(/*1,0*/);
//			render_a_whole_plane(/*0,*/0);
//	rdp_texture_copy(RDP_CONFIG | EN_TLUT);// | SAMPLE_TYPE);
			render_all_sprites(/*0*/);
//			render_a_whole_plane(/*1,*/1);
//			render_a_whole_plane(/*0,*/1);
		//	render_all_sprites(1);
		}
	}
#endif

//		m68k_execute(488*262);

	for(line = 0; line < 262; line++)
	{
#if HLE
		if(HLE_running)
		{
			if(HLE_cycles_remaining < 1)
			{
				m68k_execute(488);
			}
			else
			{
				HLE_cycles_remaining -= 488;
			}
		}
		else
		{
			m68k_execute(488);
		}

		in_vdp_interrupt = 1;
		vdp_interrupt(line);//line*4);
		in_vdp_interrupt = 0;
#else
		m68k_execute(488);
		vdp_interrupt(line);//line*4);
#endif

		//		if (z80_enabled()) {
//			z80_execute(228);
////			z80_burn(228);
//		}

//#ifndef NOVDP
#if 0
		if(field_skip == 0) 
		{
			if (line < 224) {
				// render scanline to display context
				vdp_render_scanline(line);
			}
		}
#endif
	}

#ifndef NOVDP
	if(field_skip == 0) 
	{
#ifndef NORDP		
		rdp_detach_display();
#if 0
		char str[256];
#if 1
/* if(good)
    {
        graphics_draw_box(_dc, 0, 16, 320, 24, graphics_make_color(0x00,0x00,0xFF,0x00));
    }
    else
 */   {
        graphics_draw_box(_dc, 0, 16, 320, 24/*+24*/, graphics_make_color(0x00,0x00,0x00,0x00));
    }
#endif
graphics_set_color(graphics_make_color(0xFF,0xFF,0xFF,0x00), graphics_make_color(0x00,0x00,0x00,0x00));
//		sprintf(str, "texload last frame: %d\n", texload_per_frame);
		sprintf(str, "RSP PC: %04X\n", IO_READ(RSP_PC));
	graphics_draw_text(_dc, 16, 24, str);
	/*	sprintf(str, "rect last frame: %d\n", rect_per_frame);
    graphics_draw_text(_dc, 16, 32, str);
		sprintf(str, "diff last frame: %d\n", rect_per_frame-texload_per_frame);
    graphics_draw_text(_dc, 16, 40, str);
		sprintf(str, "load per tile last frame: %d percent\n", (texload_per_frame * 100) / rect_per_frame);
    graphics_draw_text(_dc, 16, 48, str);*/
#endif
	unlockVideo(_dc);
#endif
	}
#endif

	field_count += 1;
}
