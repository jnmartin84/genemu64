/*
 * zlib License
 *
 * (C) 2018-2021 Clownacy
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include "kosinski_decompress.h"
//#define DEBUG
#include <stdbool.h>
#include <stddef.h>
#ifdef DEBUG
 #include <stdio.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "memory_stream.h"
//#include "h"
static unsigned int descriptor;
static unsigned int descriptor_bits_remaining;

/*static*/ unsigned long in_file_pointer;
extern uint8_t		*rom;
extern uint32_t	rom_len;
extern uint8_t		*sram;
extern uint32_t	sram_len;
extern uint32_t	sram_start;
extern uint32_t	sram_end;
extern uint32_t	banks[8];
extern uint8_t		banked;
extern uint8_t		sram_banked;

static MemoryStream decompression_buffer;
unsigned int m68k_read_memory_8(unsigned int addr);
extern uint8_t* m68k_ram;

#if 0
static inline unsigned char next_byte()
{
//	in_file_pointer++;
	//if(in_file_pointer < 0x400000) {
		return rom[in_file_pointer++];
	//}
	//else if(in_file_pointer >= 0xff0000) {
	//	return m68k_ram[(in_file_pointer++)&0xffff];
	//}
	//else return m68k_read_memory_8(in_file_pointer++);
}
#endif

static void GetDescriptor(void)
{
	descriptor_bits_remaining = 16;

	const unsigned char byte1 = rom[in_file_pointer++];//next_byte();//m68k_read_memory_8(in_file_pointer++);//*in_file_pointer++;
	const unsigned char byte2 = rom[in_file_pointer++];//next_byte();//m68k_read_memory_8(in_file_pointer++);//*in_file_pointer++;

	descriptor = (byte2 << 8) | byte1;
}

static bool PopDescriptor(void)
{
	const bool result = descriptor & 1;

	descriptor >>= 1;

	if (--descriptor_bits_remaining == 0)
		GetDescriptor();

	return result;
}


size_t KosinskiDecompress(const unsigned long in_file_buffer, const unsigned long out_gen_addr, size_t *out_file_size)
{	
	in_file_pointer = in_file_buffer;

	MemoryStream_Create(&decompression_buffer, out_gen_addr, CC_FALSE);

	GetDescriptor();

	for (;;)
	{
		if (PopDescriptor())
		{
		#ifdef DEBUG
			const size_t position = in_file_pointer - in_file_buffer;
		#endif

			const unsigned char byte = rom[in_file_pointer++];//next_byte();//m68k_read_memory_8(in_file_pointer++);//*in_file_pointer++;

		#ifdef DEBUG
			printf( "%zX - Literal match: At %zX, value %X\n", position, MemoryStream_GetPosition(&decompression_buffer), byte);
		#endif

			MemoryStream_WriteByte(&decompression_buffer, byte);
		}
		else
		{
			unsigned int distance;
			size_t count;

			if (PopDescriptor())
			{
			#ifdef DEBUG
				const size_t position = in_file_pointer - in_file_buffer;
			#endif

				const unsigned char byte1 = rom[in_file_pointer++];//next_byte();//m68k_read_memory_8(in_file_pointer++);//*in_file_pointer++;
				const unsigned char byte2 = rom[in_file_pointer++];//next_byte();//m68k_read_memory_8(in_file_pointer++);//*in_file_pointer++;

				distance = byte1 | ((byte2 & 0xF8) << 5) | 0xE000;
				distance = (distance ^ 0xFFFF) + 1; // Convert from negative two's-complement to positive
				count = byte2 & 7;

				if (count != 0)
				{
					count += 2;

				#ifdef DEBUG
					printf( "%zX - Full match: At %zX, src %zX, len %zX\n", position, MemoryStream_GetPosition(&decompression_buffer), MemoryStream_GetPosition(&decompression_buffer) - distance, count);
				#endif
				}
				else
				{
					count = //*in_file_pointer++ 
					//m68k_read_memory_8(in_file_pointer++) + 1;
rom[in_file_pointer++]
					//next_byte() 
+ 1;
					if (count == 1)
					{
					#ifdef DEBUG
						printf( "%zX - Terminator: At %zX, src %zX\n", position, MemoryStream_GetPosition(&decompression_buffer), MemoryStream_GetPosition(&decompression_buffer) - distance);
					#endif
						break;
					}
					else if (count == 2)
					{
					#ifdef DEBUG
						printf( "%zX - 0xA000 boundary flag: At %zX, src %zX\n", position, MemoryStream_GetPosition(&decompression_buffer), MemoryStream_GetPosition(&decompression_buffer) - distance);
					#endif
						continue;
					}
					else
					{
					#ifdef DEBUG
						printf( "%zX - Extended full match: At %zX, src %zX, len %zX\n", position, MemoryStream_GetPosition(&decompression_buffer), MemoryStream_GetPosition(&decompression_buffer) - distance, count);
					#endif
					}
				}
			}
			else
			{
				count = 2;

				if (PopDescriptor())
					count += 2;
				if (PopDescriptor())
					count += 1;

			#ifdef DEBUG
				const size_t position = in_file_pointer - in_file_buffer;
			#endif

				distance = ( //m68k_read_memory_8(in_file_pointer++)
				//*in_file_pointer++ 
				//next_byte()
rom[in_file_pointer++]				
				^ 0xFF) + 1; // Convert from negative two's-complement to positive

			#ifdef DEBUG
				printf( "%zX - Inline match: At %zX, src %zX, len %zX\n", position, MemoryStream_GetPosition(&decompression_buffer), MemoryStream_GetPosition(&decompression_buffer) - distance, count);
			#endif
			}

			const size_t dictionary_index = MemoryStream_GetPosition(&decompression_buffer) - distance;

			for (size_t i = 0; i < count; ++i)
				MemoryStream_WriteByte(&decompression_buffer, MemoryStream_GetBuffer(&decompression_buffer,dictionary_index + i));
		}
	}

//	if (out_file_buffer != NULL)
	//	*out_file_buffer = MemoryStream_GetBuffer(&decompression_buffer);

	if (out_file_size != NULL)
		*out_file_size = MemoryStream_GetPosition(&decompression_buffer);

	MemoryStream_Destroy(&decompression_buffer);

	return in_file_pointer - in_file_buffer;
}
