#include <stddef.h>
#include <stdint.h>

//Trigonometry tables
const unsigned short sine_table[] =
{
	  0x00,   0x06,   0x0C,   0x12,   0x19,   0x1F,   0x25,   0x2B,
	  0x31,   0x38,   0x3E,   0x44,   0x4A,   0x50,   0x56,   0x5C,
	  0x61,   0x67,   0x6D,   0x73,   0x78,   0x7E,   0x83,   0x88,
	  0x8E,   0x93,   0x98,   0x9D,   0xA2,   0xA7,   0xAB,   0xB0,
	  0xB5,   0xB9,   0xBD,   0xC1,   0xC5,   0xC9,   0xCD,   0xD1,
	  0xD4,   0xD8,   0xDB,   0xDE,   0xE1,   0xE4,   0xE7,   0xEA,
	  0xEC,   0xEE,   0xF1,   0xF3,   0xF4,   0xF6,   0xF8,   0xF9,
	  0xFB,   0xFC,   0xFD,   0xFE,   0xFE,   0xFF,   0xFF,   0xFF,
	 0x100,   0xFF,   0xFF,   0xFF,   0xFE,   0xFE,   0xFD,   0xFC,
	  0xFB,   0xF9,   0xF8,   0xF6,   0xF4,   0xF3,   0xF1,   0xEE,
	  0xEC,   0xEA,   0xE7,   0xE4,   0xE1,   0xDE,   0xDB,   0xD8,
	  0xD4,   0xD1,   0xCD,   0xC9,   0xC5,   0xC1,   0xBD,   0xB9,
	  0xB5,   0xB0,   0xAB,   0xA7,   0xA2,   0x9D,   0x98,   0x93,
	  0x8E,   0x88,   0x83,   0x7E,   0x78,   0x73,   0x6D,   0x67,
	  0x61,   0x5C,   0x56,   0x50,   0x4A,   0x44,   0x3E,   0x38,
	  0x31,   0x2B,   0x25,   0x1F,   0x19,   0x12,   0x0C,   0x06,
	  0x00,  -0x06,  -0x0C,  -0x12,  -0x19,  -0x1F,  -0x25,  -0x2B,
	 -0x31,  -0x38,  -0x3E,  -0x44,  -0x4A,  -0x50,  -0x56,  -0x5C,
	 -0x61,  -0x67,  -0x6D,  -0x75,  -0x78,  -0x7E,  -0x83,  -0x88,
	 -0x8E,  -0x93,  -0x98,  -0x9D,  -0xA2,  -0xA7,  -0xAB,  -0xB0,
	 -0xB5,  -0xB9,  -0xBD,  -0xC1,  -0xC5,  -0xC9,  -0xCD,  -0xD1,
	 -0xD4,  -0xD8,  -0xDB,  -0xDE,  -0xE1,  -0xE4,  -0xE7,  -0xEA,
	 -0xEC,  -0xEE,  -0xF1,  -0xF3,  -0xF4,  -0xF6,  -0xF8,  -0xF9,
	 -0xFB,  -0xFC,  -0xFD,  -0xFE,  -0xFE,  -0xFF,  -0xFF,  -0xFF,
	-0x100,  -0xFF,  -0xFF,  -0xFF,  -0xFE,  -0xFE,  -0xFD,  -0xFC,
	 -0xFB,  -0xF9,  -0xF8,  -0xF6,  -0xF4,  -0xF3,  -0xF1,  -0xEE,
	 -0xEC,  -0xEA,  -0xE7,  -0xE4,  -0xE1,  -0xDE,  -0xDB,  -0xD8,
	 -0xD4,  -0xD1,  -0xCD,  -0xC9,  -0xC5,  -0xC1,  -0xBD,  -0xB9,
	 -0xB5,  -0xB0,  -0xAB,  -0xA7,  -0xA2,  -0x9D,  -0x98,  -0x93,
	 -0x8E,  -0x88,  -0x83,  -0x7E,  -0x78,  -0x75,  -0x6D,  -0x67,
	 -0x61,  -0x5C,  -0x56,  -0x50,  -0x4A,  -0x44,  -0x3E,  -0x38,
	 -0x31,  -0x2B,  -0x25,  -0x1F,  -0x19,  -0x12,  -0x0C,  -0x06,
	  0x00,   0x06,   0x0C,   0x12,   0x19,   0x1F,   0x25,   0x2B,
	  0x31,   0x38,   0x3E,   0x44,   0x4A,   0x50,   0x56,   0x5C,
	  0x61,   0x67,   0x6D,   0x73,   0x78,   0x7E,   0x83,   0x88,
	  0x8E,   0x93,   0x98,   0x9D,   0xA2,   0xA7,   0xAB,   0xB0,
	  0xB5,   0xB9,   0xBD,   0xC1,   0xC5,   0xC9,   0xCD,   0xD1,
	  0xD4,   0xD8,   0xDB,   0xDE,   0xE1,   0xE4,   0xE7,   0xEA,
	  0xEC,   0xEE,   0xF1,   0xF3,   0xF4,   0xF6,   0xF8,   0xF9,
	  0xFB,   0xFC,   0xFD,   0xFE,   0xFE,   0xFF,   0xFF,   0xFF
};

static const unsigned char atan_table[] =
{
	0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x04,
	0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05,
	0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x06, 0x06,
	0x06, 0x06, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07,
	0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x09,
	0x09, 0x09, 0x09, 0x09, 0x09, 0x0A, 0x0A, 0x0A,
	0x0A, 0x0A, 0x0A, 0x0A, 0x0B, 0x0B, 0x0B, 0x0B,
	0x0B, 0x0B, 0x0B, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
	0x0C, 0x0C, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D, 0x0D,
	0x0D, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E, 0x0E,
	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x10,
	0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x11, 0x11,
	0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x12, 0x12,
	0x12, 0x12, 0x12, 0x12, 0x12, 0x13, 0x13, 0x13,
	0x13, 0x13, 0x13, 0x13, 0x13, 0x14, 0x14, 0x14,
	0x14, 0x14, 0x14, 0x14, 0x14, 0x15, 0x15, 0x15,
	0x15, 0x15, 0x15, 0x15, 0x15, 0x15, 0x16, 0x16,
	0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x17, 0x17,
	0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x17, 0x18,
	0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
	0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19, 0x19,
	0x19, 0x19, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A, 0x1A,
	0x1A, 0x1A, 0x1A, 0x1B, 0x1B, 0x1B, 0x1B, 0x1B,
	0x1B, 0x1B, 0x1B, 0x1B, 0x1B, 0x1C, 0x1C, 0x1C,
	0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C, 0x1C,
	0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D,
	0x1D, 0x1D, 0x1D, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
	0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1F, 0x1F,
	0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F,
	0x1F, 0x1F, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
	0x20, 0x00
};

//Trigonometry
void CalcSine(uint8_t angle, int16_t *sin, int16_t *cos)
{
	if (sin != NULL)
		*sin = sine_table[angle];
	if (cos != NULL)
		*cos = sine_table[angle + 0x40];
}

int16_t GetSin(uint8_t angle)
{
	return sine_table[angle];
}

int16_t GetCos(uint8_t angle)
{
	return sine_table[angle + 0x40];
}

uint16_t CalcAngle(int16_t x, int16_t y)
{
	//If x and y is 0, return 90 degrees
	if ((x | y) == 0)
		return 0x40;
	
	//Get absolute X and Y
	int16_t abs_x = (x < 0) ? -x : x;
	int16_t abs_y = (y < 0) ? -y : y;
	
	//Get our absolute angle
	uint8_t angle;
	if (abs_x > abs_y)
		angle = atan_table[(abs_y << 8) / abs_x];
	else
		angle = 0x40 - atan_table[(abs_x << 8) / abs_y];
	
	//Invertion if negative
	if (x < 0)
		angle = -angle + 0x80;
	if (y < 0)
		angle = -angle + 0x100;
	return angle;
}
/*
//Random number generation
dword_u random_seed;

uint32_t RandomNumber()
{
	//Re-seed if 0
	if (random_seed.v == 0)
		random_seed.v = 0x2A6D365A;
	
	//Scramble our current seed
	dword_u result = {.v = random_seed.v};
	random_seed.v <<= 2;
	random_seed.v += result.v;
	random_seed.v <<= 3;
	random_seed.v += result.v;
	result.f.l = random_seed.f.l;
	
	//switch to random_seed.f.u because of a swap
	result.f.l += random_seed.f.u;
	random_seed.f.u = result.f.l;
	
	return result.v;
}*/