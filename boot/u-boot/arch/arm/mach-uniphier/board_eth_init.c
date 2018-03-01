#include <common.h>
#include <netdev.h>

#if defined(CONFIG_AVE)

extern int mn_avev3_initialize(int dev_num, int base_addr);
#define AVE_REGBASE (0x65000000)
int board_eth_init(bd_t *bis)
{
	return mn_avev3_initialize(0, AVE_REGBASE);
}

#elif defined(CONFIG_MICRO_SUPPORT_CARD) && defined(CONFIG_SMC911X)
#define MICRO_SUPPORT_CARD_BASE		0x43f00000
#define SMC911X_BASE			((MICRO_SUPPORT_CARD_BASE) + 0x00000)

int board_eth_init(bd_t *bis)
{
	return smc911x_initialize(0, SMC911X_BASE);
}
#endif
