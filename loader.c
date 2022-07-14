
/* $Id: loader.c,v 1.7 2002-08-29 19:40:53 jkf Exp $ */

#include <libdragon.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#include "cart.h"

#include "gen-emu.h"

#include "md5.h"


/* Super Street Fighter 2 Game IDs. */
#define	SSF2_JP		"GM T-12043 -00"
#define SSF2_US		"GM T-12056 -00"

 uint8_t		*rom;
 uint32_t	rom_len;
 uint8_t		*sram;
 uint32_t	sram_len;
 uint32_t	sram_start;
 uint32_t	sram_end;
 uint32_t	banks[8];
 uint8_t		banked;
 uint8_t		sram_banked;

unsigned char *cromp;

uint32_t rom_load(char *name)
{
	uint32_t fd, len, i;
	uint8_t *rom2 = NULL;
//	uint8_t md5sum[16];
//	MD5_CTX ctx;

	//printf("Loading rom %s ... ", name);
	
    fd = dfs_open(name);

	if (fd == -1) {
		printf("rom_load(): fs_open() failed.\n");
		goto error;
	}

	/* Get file length. */
    len = dfs_size(fd);

	if (strstr(name, ".BIN") != NULL) {
		/* Allocate ROM memory. */
		rom2 = malloc(len + 0x3ffff);//15);
		if (rom2 == NULL) {
			printf("rom_load(): malloc() failed.\n");
			goto error;
		}

		//printf("ROM: %08X\n", rom);
		rom2 = (uint8_t *)(((uintptr_t)rom2+0x3ffff) & ~ (uintptr_t)0x3ffff);
		dfs_read(rom2, 1, len, fd);
	} else
	if (strstr(name, ".SMD") != NULL) {
		uint8_t buf[512];

		/* Read file header and check for SMD signature. */
        dfs_read(buf, 1, 512, fd);
		if ((buf[8] == 0xaa) && (buf[9] == 0xbb)) {
			uint32_t blocks;
			uint8_t *blk, *tmp;

			len -= 512;
			if (len % 16384) {
				printf(".smd rom corrupt. Not a multiple of 16KB.\n");
				goto error;
			}
			blocks = len / 16384;

			/* Allocate ROM memory. */
			rom2 = malloc(len);
			if (rom2 == NULL) {
				printf("rom_load(): malloc() failed.\n");
				goto error;
			}

			/* Allocate temp block. */
			blk = malloc(16384);
			if (blk == NULL) {
				printf("rom_load(): malloc() failed.\n");
				goto error;
			}

			/* Load/decode encoded blocks. */
			tmp = rom2;
			for(i = 0; i < blocks; i++) {
				uint32_t j;

				dfs_read(blk, 1, 16384, fd);
				for(j = 0; j < 8192; j++) {
					*tmp++ = blk[j+8192];
					*tmp++ = blk[j];
				}
			}

			/* Free temp block. */
			free(blk);
		} else {
			printf("rom_load(): Invalid .smd file loaded.\n");
			goto error;
		}
	} else {
		printf("ROM is not a recognized format.\n");
		goto error;
	}

	dfs_close(fd);

//	printf("Done.\n");

	/* Add ROM to  */
	rom = (uint8_t *)((uintptr_t)rom2);// | 0xA0000000);
	rom_len = len;
cromp = rom;
	/* Check if this cart has save ram. */
/*	if (rom[0x1b0] == 'R' && rom[0x1b1] == 'A') {
		uint32_t sr_start, sr_end, sr_len;
		uint8_t *sram = NULL;

		// Extract saveram details from ROM header.
		sr_start = ((uint32_t*)rom)[0x1b4/4];
		sr_end = ((uint32_t*)rom)[0x1b8/4];
		SWAPBYTES32(sr_start);
		SWAPBYTES32(sr_end);
		if (sr_start % 1)
			sr_start -= 1;
		if (!(sr_end % 1))
			sr_end += 1;
		sr_len = sr_end - sr_start + 1;
		if (sr_len) {
			sram = malloc(sr_len);
			if (sram == NULL) {
				printf("rom_load(): malloc() failed.\n");
				goto error;
			}
			bzero(sram, sr_len);
		}

		// Save details in cart struct.
		sram = sram;
		sram_len = sr_len;
		sram_start = sr_start;
		sram_end = sr_end;
		if (sram_start < rom_len)
			sram_banked = 1;
	}*/

	/* Check the cart to see if it is Super Street Fighter 2. */
	if ((memcmp(&rom[0x180], SSF2_US, (sizeof(SSF2_US) - 1)) == 0) ||
		(memcmp(&rom[0x180], SSF2_JP, (sizeof(SSF2_JP) - 1)) == 0)) {
		/* Enable banking and init default bank values. */
		banked = 1;
		banks[0] = (uint32_t)(rom + 0x000000);
		banks[1] = (uint32_t)(rom + 0x080000);
		banks[2] = (uint32_t)(rom + 0x100000);
		banks[3] = (uint32_t)(rom + 0x180000);
		banks[4] = (uint32_t)(rom + 0x200000);
		banks[5] = (uint32_t)(rom + 0x280000);
		banks[6] = (uint32_t)(rom + 0x300000);
		banks[7] = (uint32_t)(rom + 0x380000);
	} else {
		banked = 0;
	}

#if 0
	/* Calculate MD5 checksum of ROM. */
	MD5Init(&ctx);
	MD5Update(&ctx, rom, len);
	MD5Final(md5sum, &ctx);

	/* Display the checksum. */
	printf("ROM MD5 = ");
	for(i = 0; i < 16; i++)
		printf("%02x", md5sum[i]);
	printf("\n");


	printf("Cart details...\n");
	printf("ROM Length: 0x%x (%d) bytes\n", rom_len, rom_len);
	if (sram_len)
		printf("RAM Length: 0x%x (%d) bytes%s\n", sram_len, sram_len,
			   (sram_banked ? ", banked" : ""));
	if (banked)
		printf("Detected banked cartridge. Enabling banking.\n");
#endif

	return 1;

error:
	if (rom)
		free(rom);
//	if (fd)
//		fs_close(fd);
	if (fd)
		dfs_close(fd);
//	return 0;
        while(1);
}

void rom_free(void)
{
	if (rom) {
		free(rom);
		rom = NULL;
		rom_len = 0;
	}
	if (sram) {
		free(sram);
		sram = NULL;
		sram_len = 0;
		sram_start = 0;
		sram_end = 0;
		sram_banked = 0;
	}
}
