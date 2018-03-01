/*
 * Copyright (C) 2011 Panasonic Corporation
 * All Rights Reserved.
 */

#include <common.h>
#include <asm/io.h>

#include "dio.h"
#include "dio-regs.h"
#include "dio_common.h"

/* data pattern for DQSEN adjust */
static const u32 data_pattern[] = {
	0x5555aaaa, 0xffff0000, 0xaaaa5555, 0xcccc3333
};

#define DATA_LENGTH  (sizeof(data_pattern) / sizeof(data_pattern[0]))

void dio_get_dqsen_regs(struct dio_dqsen_regs *dqsen_regs)
{
	int dq, byte;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			dqsen_regs->adj[dq][byte] = readw(DIO_DQSEN_ADJ(dq / 2, dq % 2, byte)) & 0xf;
			dqsen_regs->ctl[dq][byte] = readw(DIO_CTLTIM(dq / 2, dq % 2, byte));
		}
	}

	return;
}

void dio_set_dqsen_regs(const struct dio_dqsen_regs *dqsen_regs)
{
	int dq, byte;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			u16 tmp;
			tmp = readw(DIO_DQSEN_ADJ(dq / 2, dq % 2, byte));
			tmp &= ~0xf;
			tmp |= dqsen_regs->adj[dq][byte];
			writew(tmp, DIO_DQSEN_ADJ(dq / 2, dq % 2, byte));
			writew(dqsen_regs->ctl[dq][byte], DIO_CTLTIM(dq / 2, dq % 2, byte));
		}
	}

	for (dq = 0; dq < DQ_BLOCKS; dq += 2) {
		dio_update_force(dq);
	}

	return;
}

static inline void set_adj_dq(int dq, int adj)
{
	int byte;

	for (byte = 0; byte < 2; byte++) {
		u16 tmp;
		tmp = readw(DIO_DQSEN_ADJ(dq / 2, dq % 2, byte));
		tmp &= ~0xf;
		tmp |= adj;
		writew(tmp, DIO_DQSEN_ADJ(dq / 2, dq % 2, byte));
	}
	return;
}

static inline void set_ctltim_dq(int dq, int delay)
{
	int byte;

	for (byte = 0; byte < 2; byte++)
		writew(delay, DIO_CTLTIM(dq / 2, dq % 2, byte));

	dio_update_force(dq);

	return;
}

static inline void set_fifo_pointer_readable(int dq)
{
	int byte;

	for (byte = 0; byte < 2; byte++) {
		u16 tmp;
		tmp = readw(DIO_DQSFBKEN(dq / 2, dq % 2, byte));
		tmp |= 0x2;
		writew(tmp, DIO_DQSFBKEN(dq / 2, dq % 2, byte));
	}
	return;
}

static inline void set_fifo_pointer_unreadable(int dq)
{
	int byte;

	for (byte = 0; byte < 2; byte++) {
		u16 tmp;
		tmp = readw(DIO_DQSFBKEN(dq / 2, dq % 2, byte));
		tmp &= ~0x2;
		writew(tmp, DIO_DQSFBKEN(dq / 2, dq % 2, byte));
	}
	return;
}

static inline u32 check_fifo_pointer(int dq)
{
	int bit;
	u32 ret;

	ret = 0;

	for (bit = 0; bit < 16; bit++) {

		if (readw(DIO_DQSFBK(dq / 2, dq % 2, bit / 8, bit % 8)) != 0x0008) {
			ret |= 1 << bit;
		}
	}

	return ret;
}

static void calib_dqsen_dq(struct dio_dqsen *dqsen, int ch, int dq, u32 base, int bus_mode)
{
	int adj, ctl, byte, i;
	u32 addr, base_flip = 0;
	int flip = 0;

	int *width;
	int last_fail[2], min_minus1[2];

	u32 continue_flag = 0xffffffff;

	const int width_for_break = 64;

#ifdef DEBUG
	printf("DQSEN data_pattern = ");
	for (i = 0; i < DATA_LENGTH; i++) {
		printhex8(data_pattern[i]);
		printch(',');
	}
	printf("\n");
#endif

	width = dqsen->width[dq];

	for (byte = 0; byte < 2; byte++) {
		width[byte] = 0;
		last_fail[byte] = min_minus1[byte] = (dqsen->range_min << 6) - 1;
	}

	if (dqsen->use_data_compare) {
		for (i = 0, addr = base; i < DATA_LENGTH; i += 2) {
			writel(data_pattern[i    ], addr); addr += 4;
			writel(data_pattern[i + 1], addr); addr += 4;
			if (bus_mode == DIO_BUS32)
				addr += 8;
		}

		base_flip = addr;

		for (i = 0; i < DATA_LENGTH; i += 2) {
			writel(~data_pattern[i    ], addr); addr += 4;
			writel(~data_pattern[i + 1], addr); addr += 4;
			if (bus_mode == DIO_BUS32)
				addr += 8;
		}
	}

	set_fifo_pointer_readable(dq);
	dio_enable_manual_refresh_mode(ch);

	for (adj = dqsen->range_min; adj < 16 && continue_flag; adj++) {
		set_adj_dq(dq, adj);

		for (ctl = 0; ctl < 64 && continue_flag; ctl++) {
			u32 diff = 0;

			set_ctltim_dq(dq, ctl);

			for (i = 0; i < dqsen->iteration; i++) {

				dio_issue_refresh(ch);	/* reset fifo pointer */

				if (dqsen->use_data_compare) {
					if (flip)
						diff |= readl(base      + 4) ^ data_pattern[1];
					else
						diff |= readl(base_flip + 4) ^ ~data_pattern[1];

					flip = ~flip;
				} else {
					readl(base);
				}

				diff |= check_fifo_pointer(dq);

				if ((diff & 0x00ff00ff) && (diff & 0xff00ff00))
					break;
			}

			for (byte = 0; byte < 2; byte++) {
				u32 mask;

				mask = 0x00ff00ff << 8 * byte;

				if (continue_flag & mask) {

					if (diff & mask) {
						last_fail[byte] = (adj << 6 | ctl);
						if (width[byte] > width_for_break)
							continue_flag &= ~mask;
					} else {
						int cur_width;

						cur_width = (adj << 6 | ctl) - last_fail[byte];
						if (cur_width > width[byte]) {
							min_minus1[byte] = last_fail[byte];
							width[byte] = cur_width;
						}
					}
				}

			}

		}
	}

	for (byte = 0; byte < 2; byte++) {
		if (!dqsen->use_data_compare) {
			/*
			 * unable to decide width precisely without data compare.
			 * forced to 128.
			 */
			width[byte] = 128;
		}
		dqsen->center[dq][byte] = min_minus1[byte] + 1 + width[byte] / 2;
	}

	set_fifo_pointer_unreadable(dq);
	dio_disable_manual_refresh_mode(ch);

	return;
}

void dio_calib_dqsen(struct dio_dqsen *dqsen)
{
	int dq, byte;
	struct dio_dqsen_regs dqsen_regs;

	calib_dqsen_dq(dqsen, 0, 0, SDRAM_BB_BASEADDR_CH0, DIO_BUS32);
	calib_dqsen_dq(dqsen, 0, 1, SDRAM_BB_BASEADDR_CH0 + 8, DIO_BUS32);

	calib_dqsen_dq(dqsen, 1, 2, SDRAM_BB_BASEADDR_CH1, DIO_BUS16);

#if CONFIG_DDR_NUM_CH2 > 0
	umc_ddr_ch_set(2);

	calib_dqsen_dq(dqsen, 2, 3, SDRAM_BB_BASEADDR_CH2, DIO_BUS16);

	umc_ddr_ch_set(1);
#endif

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			dqsen_regs.adj[dq][byte] = dqsen->center[dq][byte] >> 6;
			dqsen_regs.ctl[dq][byte] = dqsen->center[dq][byte] & 0x3f;
		}
	}

	dio_set_dqsen_regs(&dqsen_regs);

	return;
}
