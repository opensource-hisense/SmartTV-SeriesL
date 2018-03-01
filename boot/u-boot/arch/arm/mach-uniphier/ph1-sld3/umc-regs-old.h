#ifndef UMC_REGS_OLD_H
#define UMC_REGS_OLD_H

#define UMC_BASE		0x5b800000

#define UMC_DC_BASE(ch)		(UMC_BASE + 0x400000 + 0x200000 * ((ch) % 2) + \
				 0x2000000 * ((ch) / 2))

#define UMCSPCSETA(ch)		(UMC_DC_BASE(ch) + 0x0038)
#define UMCSPCSETB(ch)		(UMC_DC_BASE(ch) + 0x003c)
#define UMCMANSETA(ch)		(UMC_DC_BASE(ch) + 0x0360)
#define UMCMANSETD(ch)		(UMC_DC_BASE(ch) + 0x036c)
#define UMCRDATACTL_D0(ch)	(UMC_DC_BASE(ch) + 0x0600)
#define UMCRDATACTL_D1(ch)	(UMC_DC_BASE(ch) + 0x0608)
#define UMCDEBUGA_D0(ch)	(UMC_DC_BASE(ch) + 0x0710)
#define UMCDICGCTLA(ch)		(UMC_DC_BASE(ch) + 0x0724)
#define UMCDIOCTLA(ch)		(UMC_DC_BASE(ch) + 0x0C00)
#define UMCCLKENDC(ch)		(UMC_DC_BASE(ch) + 0x8000)

#ifndef __ASSEMBLY__

#include <linux/types.h>

static inline void umc_polling(u32 address, u32 expval, u32 mask)
{
	u32 nmask = ~mask;
	u32 data;
	do {
		data = readl(address) & nmask;
	} while (data != expval);
}
#endif

#endif /* UMC_REGS_OLD_H */
