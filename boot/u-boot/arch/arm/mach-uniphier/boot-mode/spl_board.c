/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <spl.h>
#include <linux/err.h>

#include "../arm64/rom-api.h"
#include "boot-device.h"

void spl_board_announce_boot_device(void)
{
	switch (spl_boot_device_raw()) {
	case BOOT_DEVICE_MMC1:
		printf("eMMC");
		break;
	case BOOT_DEVICE_USB:
		printf("USB");
		break;
	default:
		printf("Not Supported");
		break;
	}
}

static int uniphier_spl_emmc_load_image(void)
{
	u32 block_addr = CONFIG_SYS_MMCSD_RAW_MODE_U_BOOT_SECTOR;
	int ret;

	ret = uniphier_rom_emmc_load_image_part(1, block_addr,
						(void *)CONFIG_SYS_TEXT_BASE,
						1);
	if (ret) {
		printf("failed to load image (error %d)\n", ret);
		return ret;
	}

	ret = spl_parse_image_header((void *)CONFIG_SYS_TEXT_BASE);
	if (ret)
		return ret;

	ret = uniphier_rom_emmc_load_image_part(1, block_addr,
				(void *)(unsigned long)spl_image.load_addr,
						spl_image.size / 512);
	if (ret) {
		printf("failed to load image (error %d)\n", ret);
		return ret;
	}

	return 0;
}

static int uniphier_spl_usb_load_image(void)
{
	/* to be implemented */
	return 0;
}

int spl_board_load_image(void)
{
	int ret;

	uniphier_rom_api_prepare();

	switch (spl_boot_device_raw()) {
	case BOOT_DEVICE_MMC1:
		ret = uniphier_spl_emmc_load_image();
		break;
	case BOOT_DEVICE_USB:
		ret = uniphier_spl_usb_load_image();
		break;
	default:
		ret = -ENOTSUPP;
		break;
	}

	uniphier_rom_api_unprepare();

	return ret;
}
