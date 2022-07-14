#include <stdint.h>

uint32_t flip(uint32_t eight_pixels, int hf) {
	if(hf) {
		uint8_t byte0 = (eight_pixels>>24)&0xff;
		uint8_t byte1 = (eight_pixels>>16)&0xff;
		uint8_t byte2 = (eight_pixels>> 8)&0xff;
		uint8_t byte3 = (eight_pixels    )&0xff;

		// swap nybbles in bytes
		byte0 = ((byte0 & 0xf)<<4) | (byte0 >> 4) & 0xf;
		byte1 = ((byte1 & 0xf)<<4) | (byte1 >> 4) & 0xf;
		byte2 = ((byte2 & 0xf)<<4) | (byte2 >> 4) & 0xf;
		byte3 = ((byte3 & 0xf)<<4) | (byte3 >> 4) & 0xf;
		
		// swap byte order
		eight_pixels = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
	}
	
	return eight_pixels;
}


///home/Jason/n64dev/n64inst/bin/mips64-elf-gcc -mno-shared -mno-abicalls -mno-branch-likely -mno-llsc -mno-check-zero-division -std=gnu99 -mabi=o64 -march=vr4300 -mtune=vr4300 -mips3  -G4 -O2 -I/home/Jason/n64dev/n64inst/mips64-elf/include -I/home/Jason/n64dev/n64inst/include  -s flip.c -o flip.S
