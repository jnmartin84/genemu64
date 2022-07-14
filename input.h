/* $Id: input.h,v 1.2 2002-07-11 19:39:26 jkf Exp $ */

#ifndef _INPUT_H_
#define _INPUT_H_

extern void ctlr_init(void);
extern void ctlr_reset(void);
extern uint8_t ctlr_data_reg_read(int);
extern uint8_t ctlr_ctrl_reg_read(int);
extern void ctlr_data_reg_write(int, uint8_t);
extern void ctlr_ctrl_reg_write(int, uint8_t);

#endif /* _INPUT_H_ */
