/*
 * Copyright (C) 2011 Panasonic Corporation
 * All Rights Reserved.
 */

#include <common.h>
#include <asm/io.h>

#include "dio.h"
#include "dio-regs.h"
#include "dio_common.h"

void dio_get_toffset_regs(struct dio_toffset_regs *toffset_regs)
{
	int dq, bit;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			toffset_regs->clock[dq][bit] = readw(DIO_WTIM(dq / 2, dq % 2, bit / 8, bit % 8)) & 0x7f;
			toffset_regs->data[dq][bit]  = readw(DIO_WTIM(dq / 2, dq % 2, bit / 8, bit % 8)) >>   8;
		}

		for (bit = 0; bit < 2; bit++) {
			toffset_regs->mclock[dq][bit] = readw(DIO_WTIM_DQM(dq / 2, dq % 2, bit)) & 0x7f;
			toffset_regs->mdata[dq][bit]  = readw(DIO_WTIM_DQM(dq / 2, dq % 2, bit)) >>   8;
		}
	}

	return;
}

void  dio_set_toffset_regs(const struct dio_toffset_regs *toffset_regs)
{
	int dq, bit;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			writew((u16)(toffset_regs->data[dq][bit]) << 8 | toffset_regs->clock[dq][bit], DIO_WTIM(dq / 2, dq % 2, bit / 8, bit % 8));
		}

		for (bit = 0; bit < 2; bit++) {
			writew((u16)(toffset_regs->mdata[dq][bit]) << 8 | toffset_regs->mclock[dq][bit], DIO_WTIM_DQM(dq / 2, dq % 2, bit));
		}
	}

	for (dq = 0; dq < DQ_BLOCKS; dq += 2) {
		dio_update_force(dq);
	}

	return;
}

static inline void  set_wtim_dqm(int ch, int dq, int delay)
{
	int byte;

	for (byte = 0; byte < 2; byte++)
		writew(delay << 8 | delay, DIO_WTIM_DQM(dq / 2, dq % 2, byte));

	dio_update_force(dq);

	return;
};

static inline void  set_wtim_dq(int ch, int dq, int delay)
{
	int bit;

	for (bit = 0; bit < 16; bit++)
		writew(delay << 8 | delay, DIO_WTIM(dq / 2, dq % 2, bit / 8, bit % 8));

	dio_update_force(dq);

	return;
};

//calibration of T-offset of DQM.
static void  calib_toffset_dqm(struct dio_toffset *toffset, int ch, int dq, u32 addr, int bus_mode)
{

	int delay, byte, i;
	u16 *log;

	log = toffset->mlog[dq];

	for (delay = toffset->range_min; delay <= toffset->range_max; delay++) {
		u16 diff = 0;

		if (! (toffset->skip_flag && delay >= toffset->skip_min && delay <= toffset->skip_max) ) {

			set_wtim_dqm(ch, dq, delay);

			for (i = 0; i < toffset->iteration; i++) {
				u32 addr0, addr1, addr2;
				u16 exp_data0, exp_data1, exp_data2;
				addr0 = addr;
				addr1 = addr + 2;
				addr2 = addr + 4;
				exp_data0 =  readw(addr0);
				exp_data1 = ~readw(addr1);
				exp_data2 =  readw(addr2);

				while(addr2 < addr + 16 || (bus_mode == DIO_BUS32  && addr2 < addr + 32)) {
					u16 data0, data1, data2;

					writew(exp_data1, addr1);

					data0 = readw(addr0);
					data1 = readw(addr1);
					data2 = readw(addr2);

					diff |= exp_data0 ^ data0;
					diff |= exp_data1 ^ data1;
					diff |= exp_data2 ^ data2;

					addr0 = addr1;
					addr1 = addr2;
					addr2 += 2;
					if (bus_mode == DIO_BUS32 && (addr2 - addr) % 16 == 8)
						addr2 += 8;

					exp_data0 =  data1;
					exp_data1 = ~data2;
					exp_data2 = readw(addr2);
				}

				if (~diff == 0)
					break;
			}

		}

		log[delay] = diff;
	}

	for (byte = 0; byte < 2; byte++) {
		u16 mask;
		int last_fail, min_minus1, width;

		mask = 0xff << 8 * byte;
		last_fail = min_minus1 = toffset->range_min - 1;
		width = 0;

		for (delay = toffset->range_min; delay <= toffset->range_max; delay++) {

			if (log[delay] & mask) {
				last_fail = delay;
			}
			else {
				int cur_width;

				cur_width = delay - last_fail;
				if (cur_width > width) {
					min_minus1 = last_fail;
					width = cur_width;
				}
			}
		}

		toffset->mwidth[dq][byte]  = width;
		toffset->mcenter[dq][byte] = min_minus1 + 1 + width / 2;

	}

	return;
}

//calibration of T-offset of DQ.
static void  calib_toffset_dq(struct dio_toffset *toffset, int ch, int dq, u32 addr, int bus_mode)
{
	int delay, bit, i;
	u16 *log;

	log = toffset->log[dq];

	for (delay = toffset->range_min; delay <= toffset->range_max; delay++) {
		u32 diff = 0;

		if (! (toffset->skip_flag && delay >= toffset->skip_min && delay <= toffset->skip_max) ) {

			set_wtim_dq(ch, dq, delay);

			for (i = 0; i < toffset->iteration; i++) {

				dio_datacpy(addr, toffset->pattern_data, toffset->pattern_length, bus_mode);

				diff |= dio_datacmp(addr, toffset->pattern_data, toffset->pattern_length, bus_mode);

				if (~diff == 0)
					break;
			}

		}
		log[delay] = diff >> 16 | diff;
	}

	for (bit = 0; bit < 16; bit++) {
		u16 mask;
		int last_fail, min_minus1, width;

		mask = 1 << bit;
		last_fail = min_minus1 = toffset->range_min - 1;
		width = 0;

		for (delay = toffset->range_min; delay <= toffset->range_max; delay++) {

			if (log[delay] & mask) {
				last_fail = delay;
			}
			else {
				int cur_width;

				cur_width = delay - last_fail;
				if (cur_width > width) {
					min_minus1 = last_fail;
					width = cur_width;
				}
			}
		}

		toffset->width[dq][bit]  = width;
		toffset->center[dq][bit] = min_minus1 + 1 + width / 2;
	}

#ifdef DEBUG
	printascii("result (dq = %d)\n", dq);

	for (bit = 0; bit < 16; bit++) {
		u16 mask;
                mask = 1 << bit;

		if (bit < 10)
			printch(' ');
		printsigned(bit); printch(':');

		for (delay = toffset->range_min; delay <= toffset->range_max; delay++) {
			if (delay == toffset->center[dq][bit])
				printch('|');
			else if (toffset->log[dq][delay] & mask)
				printch('.');
			else
				printch('o');
		}
		printsigned(toffset->width[dq][bit]), printch(',');
		printsigned(toffset->center[dq][bit]), printch('\n');
	}
#endif

	return;
}

void  dio_calib_toffset(struct dio_toffset *toffset)
{
	int dq, byte, bit;
	struct dio_toffset_regs toffset_regs;

#ifdef DEBUG
	printf("Toffset Calibration Begin\n");
	printf("data_pattern = ");
	{
		int i;
		for (i = 0; i < toffset->pattern_length * 4; i++) {
			printhex8(toffset->pattern_data[i]);
			printch(',');
		}
		printch('\n');
	}
#endif

	//Ch0
	calib_toffset_dqm(toffset, 0, 0, SDRAM_BB_BASEADDR_CH0,     DIO_BUS32);
	calib_toffset_dqm(toffset, 0, 1, SDRAM_BB_BASEADDR_CH0 + 8, DIO_BUS32);

	//Ch1
	calib_toffset_dqm(toffset, 1, 2, SDRAM_BB_BASEADDR_CH1,     DIO_BUS16);

#if CONFIG_DDR_NUM_CH2 > 0
	umc_ddr_ch_set(2); //Ch1 -> Ch2

	//Ch2
	calib_toffset_dqm(toffset, 2, 3, SDRAM_BB_BASEADDR_CH2,     DIO_BUS16);

	umc_ddr_ch_set(1); //Ch2 -> Ch1

#endif

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			toffset_regs.mclock[dq][byte] = toffset->mcenter[dq][byte];
			toffset_regs.mdata[ dq][byte] = toffset->mcenter[dq][byte];
		}
	}

	dio_set_toffset_regs(&toffset_regs);

	//Ch0
	calib_toffset_dq(toffset, 0, 0, SDRAM_BB_BASEADDR_CH0,     DIO_BUS32);
	calib_toffset_dq(toffset, 0, 1, SDRAM_BB_BASEADDR_CH0 + 8, DIO_BUS32);

	//Ch1
	calib_toffset_dq(toffset, 1, 2, SDRAM_BB_BASEADDR_CH1,     DIO_BUS16);

#if CONFIG_DDR_NUM_CH2 > 0
	umc_ddr_ch_set(2); //Ch1 -> Ch2

	//Ch2
	calib_toffset_dq(toffset, 2, 3, SDRAM_BB_BASEADDR_CH2,     DIO_BUS16);

	umc_ddr_ch_set(1); //Ch2 -> Ch1

#endif

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (bit = 0; bit < 16; bit++) {
			toffset_regs.clock[dq][bit] = toffset->center[dq][bit];
			toffset_regs.data[ dq][bit] = toffset->center[dq][bit];
		}
	}

	dio_set_toffset_regs(&toffset_regs);

	return;
}
