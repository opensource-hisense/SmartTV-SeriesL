/*
 * Copyright (C) 2011-2014 Panasonic Corporation
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include "dio.h"

void ddr_init(void)
{
	//Write Leveling
	{
		struct dio_wl wl;

		wl.valid_max[0][0] = 127;
		wl.valid_max[0][1] = 127;
		wl.valid_max[1][0] = 127;
		wl.valid_max[1][1] = 127;
		wl.valid_max[2][0] = 127;
		wl.valid_max[2][1] = 127;
#if CONFIG_DDR_NUM_CH2 > 0
		wl.valid_max[3][0] = 127;
		wl.valid_max[3][1] = 127;
#endif

		dio_calib_wl(&wl);
	}

	//DQSEN Adjust
	{
		struct dio_dqsen   dqsen;

		dqsen.range_min = 6;
		dqsen.iteration = 2;
		dqsen.use_data_compare = 1;

		dio_calib_dqsen(&dqsen);
	}

	//Toffset Calibration
	{
		struct dio_toffset toffset;

		toffset.range_min = 0;
		toffset.range_max = 80;
		toffset.skip_flag = 0;
		toffset.skip_min  = 24;
		toffset.skip_max  = 31;
		toffset.pattern_data = dio_pattern_data;
		toffset.pattern_length = dio_pattern_length;
		toffset.iteration = 5;

		dio_calib_toffset(&toffset);
	}

	//Roffset Calibration
	{
		struct dio_roffset roffset;

		roffset.range_min  =  0;
		roffset.range_max  = 88;
		roffset.data_delay = 0;
		roffset.skip_flag = 0;
		roffset.skip_min  = 28;
		roffset.skip_max  = 43;
		roffset.pattern_data = dio_pattern_data;
		roffset.pattern_length = dio_pattern_length;
		roffset.iteration  = 5;

		dio_calib_roffset(&roffset);
	}

	return;
}
