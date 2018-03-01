/*
 * Copyright (C) 2011 Panasonic Corporation
 * All Rights Reserved.
 */

#include <common.h>
#include <asm/io.h>

#include "dio.h"
#include "dio-regs.h"
#include "dio_common.h"

void dio_get_roffset_regs(struct dio_roffset_regs *roffset_regs)
{
	int dq, bit;
	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			roffset_regs->clockp[dq][bit] =	readw(DIO_RTIM_CLOCK(dq / 2, dq % 2, bit / 8, bit % 8)) & 0xff;
			roffset_regs->clockn[dq][bit] = readw(DIO_RTIM_CLOCK(dq / 2, dq % 2, bit / 8, bit % 8)) >>   8;
			roffset_regs->data[dq][bit]   = readw(DIO_RTIM_DATA( dq / 2, dq % 2, bit / 8, bit % 8));
		}
	}

	return;
}

void  dio_set_roffset_regs(const struct dio_roffset_regs *roffset_regs)
{
	int dq, bit;
	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			writew((u16)(roffset_regs->clockn[dq][bit]) << 8
				| roffset_regs->clockp[dq][bit],
				DIO_RTIM_CLOCK(dq / 2, dq % 2, bit / 8, bit % 8));
			writew(roffset_regs->data[dq][bit] ,
			DIO_RTIM_DATA( dq / 2, dq % 2, bit / 8, bit % 8));
		}
	}

	for (dq = 0; dq < DQ_BLOCKS; dq += 2) {
		dio_update_force(dq);
	}

	return;
}

static inline void  set_rtim_clock_dq(int ch, int dq, int delay)
{
	int byte;

	for (byte = 0; byte < 2; byte++)
		writew(delay << 8 | delay , DIO_RTIM_CLOCK_BYTE(dq / 2, dq % 2, byte));

	dio_update_force(dq);

	return;
}

static inline void  set_rtim_data_dq(int ch, int dq, int delay)
{
	int byte;

	for (byte = 0; byte < 2; byte++)
		writew(delay, DIO_RTIM_DATA_BYTE(dq / 2, dq % 2, byte));

	dio_update_force(dq);

	return;
}

/*
 * roffset calibration
 * MAIN ROUTINE
 */
static void  calib_roffset_dq(struct dio_roffset *roffset, int ch, int dq, u32 addr, int bus_mode)
{
	int delay, bit, i;
	u32 *log;

	log = roffset->log[dq];

	set_rtim_data_dq(ch, dq, roffset->data_delay);

	for (delay = roffset->range_min; delay <= roffset->range_max; delay++) {
		u32 diff = 0;

		if (! (roffset->skip_flag && delay >= roffset->skip_min &&
						delay <= roffset->skip_max) ) {

			set_rtim_clock_dq(ch, dq, delay);

			for (i = 0; i < roffset->iteration; i++) {

				diff |= dio_datacmp(addr, roffset->pattern_data,
					roffset->pattern_length, bus_mode);

				if (~diff == 0)
					break;
			}

		}
		log[delay] = diff;
	}

	for (bit = 0; bit < 32; bit++) {
		u32 mask;
		int last_fail, min_minus1, width;

		mask = 1 << bit;

		last_fail = min_minus1 = roffset->range_min - 1;
		width = 0;

		for (delay = roffset->range_min; delay <= roffset->range_max;
								delay++) {
			if (log[delay] & mask) {
				last_fail = delay;
			} else {
				int cur_width;

				cur_width = delay - last_fail;
				if (cur_width > width) {
					min_minus1 = last_fail;
					width = cur_width;
				}
			}
		}

		roffset->width[dq][bit]  = width;
		roffset->center[dq][bit] = min_minus1 + 1 + width / 2
						- roffset->data_delay;
	}

#ifdef DEBUG
	printascii("result (dq =");
	printsigned(dq);
	printascii(")\n");

	for (bit = 0; bit < 16; bit++) {
		int is_neg;

		for (is_neg = 0; is_neg < 2; is_neg++) {
			int bit_pos = is_neg ? bit : bit + 16;
			u32 mask = 1 << bit_pos;

			if (bit < 10)
				printch(' ');
			printsigned(bit); printch(is_neg ? 'N' : 'P'); printch(':');

			for (delay = roffset->range_min; delay <= roffset->range_max; delay++) {
				if (delay == roffset->center[dq][bit_pos] + roffset->data_delay)
					printch('|');
				else if (roffset->skip_flag && delay >= roffset->skip_min && delay <= roffset->skip_max)
					printch('-');
				else if (roffset->log[dq][delay] & mask)
					printch('.');
				else
					printch('o');
			}
			printsigned(roffset->width[dq][bit_pos]), printch(',');
			printsigned(roffset->center[dq][bit_pos]), printch('\n');
                }
	}
#endif

	return;
}

void  dio_calib_roffset(struct dio_roffset *roffset)
{
	int dq, bit;
	struct dio_roffset_regs roffset_regs;

#ifdef DEBUG
	printascii("Roffset Calibration Begin\n");
	printascii("data_pattern = ");
	{
		int i;
		for (i = 0; i < roffset->pattern_length * 4; i++) {
			printhex8(roffset->pattern_data[i]);
			printch(',');
		}
		printch('\n');
	}
#endif

	dio_datacpy(SDRAM_BB_BASEADDR_CH0,     roffset->pattern_data, roffset->pattern_length, DIO_BUS32);
	dio_datacpy(SDRAM_BB_BASEADDR_CH0 + 8, roffset->pattern_data, roffset->pattern_length, DIO_BUS32);
	dio_datacpy(SDRAM_BB_BASEADDR_CH1,     roffset->pattern_data, roffset->pattern_length, DIO_BUS16);

	calib_roffset_dq(roffset, 0, 0, SDRAM_BB_BASEADDR_CH0,     DIO_BUS32);
	calib_roffset_dq(roffset, 0, 1, SDRAM_BB_BASEADDR_CH0 + 8, DIO_BUS32);
	calib_roffset_dq(roffset, 1, 2, SDRAM_BB_BASEADDR_CH1,     DIO_BUS16);

#if CONFIG_DDR_NUM_CH2 > 0
	umc_ddr_ch_set(2);

	dio_datacpy(SDRAM_BB_BASEADDR_CH2, roffset->pattern_data, roffset->pattern_length, DIO_BUS16);

	calib_roffset_dq(roffset, 2, 3, SDRAM_BB_BASEADDR_CH2,     DIO_BUS16);

	umc_ddr_ch_set(1);
#endif

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			roffset_regs.clockn[dq][bit] = max(roffset->center[dq][bit     ] + ROFFSET_NEGBASE, 0);
			roffset_regs.clockp[dq][bit] = max(roffset->center[dq][bit + 16] + ROFFSET_POSBASE, 0);
			roffset_regs.data[  dq][bit] = 0;
		}
	}

	dio_set_roffset_regs(&roffset_regs);

	return;
}
