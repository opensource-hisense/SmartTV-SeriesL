/*
 * Copyright (C) 2011 Panasonic Corporation
 * All Rights Reserved.
 */

#include <common.h>
#include <asm/io.h>

#include "dio.h"
#include "dio-regs.h"
#include "dio_common.h"
#include "umc-regs-old.h"

void dio_get_wl_regs(struct dio_wl_regs *wl_regs)
{
	int dq, byte;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			wl_regs->clock[dq][byte] = readw(DIO_BYTETIM(dq / 2, dq % 2, byte));
		}
	}

	return;
}

void dio_set_wl_regs(const struct dio_wl_regs *wl_regs)
{
	int dq, byte;

	for (dq = 0; dq < DQ_BLOCKS; dq++) {
		for (byte = 0; byte < 2; byte++) {
			writew(wl_regs->clock[dq][byte], DIO_BYTETIM(dq / 2, dq % 2, byte));
		}
	}

	for (dq = 0; dq < DQ_BLOCKS; dq += 2) {
		dio_update_force(dq);
	}

	return;
}

static inline void set_bytetim_dq(int dq, int delay)
{
	int byte;

	for (byte = 0; byte < 2; byte++)
		writew(delay, DIO_BYTETIM(dq / 2, dq % 2, byte));

	dio_update_force(dq);

	return;
}

static inline void enable_wl_mode(int ch)
{
	u32 tmp;

	/* MANSETA[28:8] = {RAS, CAS, WE, BA[2:0], A[14:0]} */
	writel(0x1c808601, UMCMANSETA(ch));
	tmp = readl(UMCMANSETD(ch));
	tmp |= 0x1;
	writel(tmp, UMCMANSETD(ch));

	return;
}

static inline void disable_wl_mode(int ch)
{
	u32 tmp;
	tmp = readl(UMCMANSETD(ch));
	tmp &= ~0x1;
	writel(tmp, UMCMANSETD(ch));
	writel(0x1c800601, UMCMANSETA(ch));

	return;
}

enum word_locate { LOW_WORD, HIGH_WORD };

static inline u8 get_wl_data(int ch, enum word_locate word_locate)
{
	volatile u8 data;
	u32 tmp;

	tmp = readl(UMCMANSETD(ch));
	tmp |= 0x2;
	writel(tmp, UMCMANSETD(ch));	/* issue DQS pulse */

	while ((data = readb(UMCMANSETD(ch))) & 0x2)
		;

	/*
	 * Take care!
	 * bit order swapped (Hardware specification is quite strange.)
	 * WL_DATA[3] corresponds DQS0
	 * WL_DATA[2] corresponds DQS1
	 * WL_DATA[1] corresponds DQS2
	 * WL_DATA[0] corresponds DQS3
	 */
	if (word_locate == HIGH_WORD)
		return (data >> 3 & 0x2) | (data >> 5 & 0x1);
	else
		return (data >> 5 & 0x2) | (data >> 7 & 0x1);
}

static const u32 clock_gating_disable[2] = {
	0x0000003f, 0x0000003f
};

static inline void get_clock_gating(int ch, u32 clock_gating[])
{

	clock_gating[0] = readl(UMCDICGCTLA(ch));
	clock_gating[1] = readl(UMCCLKENDC(ch));

	return;
}

static inline void set_clock_gating(int ch, const u32 clock_gating[])
{
	writel(clock_gating[0], UMCDICGCTLA(ch));
	writel(clock_gating[1], UMCCLKENDC(ch));

	return;
}

static void calib_wl_dq(struct dio_wl *wl, int ch, int dq, enum word_locate word_locate)
{
	int delay, byte;
	u32 prev_clock_gating[2];
	u8 *log;

	log = wl->log[dq];

	get_clock_gating(ch, prev_clock_gating);
	set_clock_gating(ch, clock_gating_disable);
	enable_wl_mode(ch);

	for (delay = 0; delay < 128; delay++) {

		set_bytetim_dq(dq, delay);

		log[delay] = get_wl_data(ch, word_locate);
	}

	disable_wl_mode(ch);
	set_clock_gating(ch, prev_clock_gating);

	for (byte = 0; byte < 2; byte++) {
		u8 mask;
		int last_low, edge_minus1, width;

		mask = 1 << byte;

		last_low = edge_minus1 = -1;
		width = 0;

		for (delay = 0; delay < 256; delay++) {
			if (log[delay % 128] & mask) {
				int cur_width;

				cur_width = delay - last_low;
				if (cur_width > width) {
					edge_minus1 = last_low;
					width = cur_width;
				}
			} else {
				last_low = delay;
			}
		}

		if (edge_minus1 < wl->valid_max[dq][byte])
			wl->edge[dq][byte] = edge_minus1 + 1;
		else
			wl->edge[dq][byte] = 0;
	}

	return;
}

void dio_calib_wl(struct dio_wl *wl)
{
	int dq, byte;
	struct dio_wl_regs wl_regs;

	calib_wl_dq(wl, 0, 0, LOW_WORD);
	calib_wl_dq(wl, 0, 1, HIGH_WORD);

	calib_wl_dq(wl, 1, 2, LOW_WORD);
#if CONFIG_DDR_NUM_CH2 > 0
	{
	/*
	 * In order to execute write leveling of Ch2,
	 * it is necessary to enable bit0 of UMCMANSETD of both Ch1 and Ch2.
	 */
	u32 tmp;
	tmp = readl(UMCMANSETD(1));
	tmp |= 0x1;
	writel(tmp, UMCMANSETD(1));
	calib_wl_dq(wl, 2, 3, LOW_WORD);
	tmp = readl(UMCMANSETD(1));
	tmp &= ~0x1;
	writel(tmp, UMCMANSETD(1));
	}
#endif

	for (dq = 0; dq < DQ_BLOCKS; dq++)
		for (byte = 0; byte < 2; byte++)
			wl_regs.clock[dq][byte] = wl->edge[dq][byte];

	dio_set_wl_regs(&wl_regs);

	return;
}
