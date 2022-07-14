#ifndef _GEN_EMU_H_
#define _GEN_EMU_H_

#include <libdragon.h>

typedef signed char sint8_t;
typedef signed short sint16_t;
typedef signed long sint32_t;

extern uint8_t debug;
extern uint8_t quit;
extern uint8_t dump;

#define SWAPWORDS(x) ( ((x << 16)&0xFFFF0000) | ((x >> 16)&0x0000FFFF) )
#define SWAPBYTES16(x) (((x>>8) | (x<<8)))

#define SWAP_WORDS SWAPWORDS


/*#define SWAPBYTES64(x)   (  	(((x >> 56) & 0x00000000000000FF)      ) | \
				(((x >> 48) & 0x00000000000000FF) <<  8) | \
				(((x >> 40) & 0x00000000000000FF) << 16) | \
				(((x >> 32) & 0x00000000000000FF) << 24) | \
				(((x >> 24) & 0x00000000000000FF) << 32) | \
				(((x >> 16) & 0x00000000000000FF) << 40) | \
				(((x >>  8) & 0x00000000000000FF) << 48) | \
				(((x      ) & 0x00000000000000FF) << 56) )*/
/*uint64_t SWAPBYTES64(uint64_t LL)
{
	uint64_t SS;
	uint8_t B0,B1,B2,B3,B4,B5,B6,B7;

B0 = (LL >> 56) & 0x00000000000000FF;
B1 = (LL >> 48) & 0x00000000000000FF;
B2 = (LL >> 40) & 0x00000000000000FF;
B3 = (LL >> 32) & 0x00000000000000FF;
B4 = (LL >> 24) & 0x00000000000000FF;
B5 = (LL >> 16) & 0x00000000000000FF;
B6 = (LL >>  8) & 0x00000000000000FF;
B7 = (LL      ) & 0x00000000000000FF;

SS = (B7 << 56) | (B6 << 48) | (B5 << 40) | (B4 << 32) | (B3 << 24) | (B2 << 16) | (B1 << 8) | B0;

	return SS;
}*/

#endif /* _GEN_EMU_H_ */
