/* $Id: cart.h,v 1.3 2002-08-29 19:40:53 jkf Exp $ */

#ifndef _CART_H_
#define _CART_H_

//typedef struct cart_s {
extern uint8_t		*rom;
extern uint32_t	rom_len;
extern uint8_t		*sram;
extern uint32_t	sram_len;
extern uint32_t	sram_start;
extern uint32_t	sram_end;
extern uint32_t	banks[8];
extern uint8_t		banked;
extern uint8_t		sram_banked;

//} cart_t;

//extern cart_t cart;

#endif /* _CART_H_ */
