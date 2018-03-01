#ifndef DIO_COMMON_H
#define DIO_COMMON_H

#include <common.h>
#include <asm/io.h>

#include "umc-regs-old.h"
#include "dio-regs.h"

static inline void  dio_update_force(int dq){

	u16 misc1;

	misc1 = readw(DIO_MDLL_MISC1(dq / 2));
	writew(misc1 & ~DIO_UPDATE_FORCE_MASK, DIO_MDLL_MISC1(dq / 2));
	writew(misc1 |  DIO_UPDATE_FORCE_MASK, DIO_MDLL_MISC1(dq / 2));
	writew(misc1 & ~DIO_UPDATE_FORCE_MASK, DIO_MDLL_MISC1(dq / 2));

	readw(DIO_MDLL_MISC1(dq / 2));

	return;
}

static inline void  dio_enable_manual_refresh_mode(int ch){
	u32 tmp;
	tmp = readl(UMCSPCSETB(ch));
	tmp |= 0x2;
	writel(tmp, UMCSPCSETB(ch));

	return;
}

static inline void  dio_disable_manual_refresh_mode(int ch){
	u32 tmp;
	tmp = readl(UMCSPCSETB(ch));
	tmp &= ~0x2;
	writel(tmp, UMCSPCSETB(ch));

	return;
}


static inline void  dio_issue_refresh(int ch){
	u32 tmp;
	tmp = readl(UMCSPCSETA(ch));
	tmp |= 0x1;
	writel(tmp, UMCSPCSETA(ch));
	if(readl(UMCSPCSETA(ch)) & 0x1)
		;

	return;
}


void umc_ddr_ch_set(int ch);

#endif
