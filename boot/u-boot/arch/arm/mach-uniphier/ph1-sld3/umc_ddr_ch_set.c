#include <common.h>
#include <linux/io.h>

#include "../bcu/bcu-regs.h"

#if CONFIG_DDR_NUM_CH2 > 0

void umc_ddr_ch_set(int ch)
{
	u32 tmp;

	switch(ch){
	case 1:
		tmp = readl(BC0PCR);
		tmp &= ~0x3;
		tmp |= 0;
		writel(tmp, BC0PCR);
		readl(BC0PCR);

		/* UMC setting */
		writel(0x00000000, 0x5b800f5c) ;
		writel(0x00021018, 0x5b80010c) ;
		writel(0x0002021e, 0x5b80018c) ;
		readl(0x5b80018c) ;
		break;
	case 2: /* Switch to Ch21 */
		tmp = readl(BC0PCR);
		tmp &= ~0x3;
		tmp |= 2;
		writel(tmp, BC0PCR);
		readl(BC0PCR);

		/* UMC setting */
		writel(0x00000008, 0x5b800f5c) ;
		writel(0x00021000, 0x5b80010c) ;
		writel(0x0002001c, 0x5b80018c) ;
		readl(0x5b80018c) ;
		break;
	default:
		break;
	}
}
#endif
