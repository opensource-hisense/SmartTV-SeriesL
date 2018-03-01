/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/types.h>

#include "arm-smccc.h"

#define ROM_FN_NAND_LOAD_IMAGE			0x10000000
#define ROM_FN_EMMC_LOAD_IMAGE			0x10010000
#define ROM_FN_EMMC_SEND_CMD			0x10010001
#define ROM_FN_EMMC_CARD_BLOCKADDR		0x10010002
#define ROM_FN_EMMC_SWITCH_PART			0x10010003
#define ROM_FN_EMMC_LOAD_IMAGE_PART		0x10010004
#define ROM_FN_USB_LOAD_IMAGE			0x10020000
#define ROM_FN_SECURE_VERIFY_NAND_EMMC		0x10030000
#define ROM_FN_SECURE_VERIFY_USB		0x10030001
#define ROM_FN_SECURE_AES_DECRYPT		0x10030002

static unsigned long invoke_rom_fn(unsigned long function_id,
				   unsigned long arg0,
				   unsigned long arg1,
				   unsigned long arg2,
				   unsigned long arg3,
				   unsigned long arg4,
				   unsigned long arg5,
				   unsigned long arg6)
{
	struct arm_smccc_res res;

	arm_smccc_smc(function_id, arg0, arg1, arg2, arg3, arg4, arg5, arg6,
		      &res);
	return res.a0;
}

int uniphier_rom_nand_load_image(void *load_addr, u32 block, u32 page_cnt)
{
	return invoke_rom_fn(ROM_FN_NAND_LOAD_IMAGE, (unsigned long)load_addr,
			     block, page_cnt, 0, 0, 0, 0);
}

int uniphier_rom_emmc_load_image(u32 block_or_byte_addr, void *load_addr,
				 u32 block_cnt)
{
	return invoke_rom_fn(ROM_FN_EMMC_LOAD_IMAGE, block_or_byte_addr,
			     (unsigned long)load_addr, block_cnt, 0, 0, 0, 0);
}

int uniphier_rom_emmc_send_cmd(u32 cmd, u32 arg)
{
	return invoke_rom_fn(ROM_FN_EMMC_SEND_CMD, cmd, arg, 0, 0, 0, 0, 0);
}

int uniphier_rom_emmc_card_blockaddr(u32 rca)
{
	return invoke_rom_fn(ROM_FN_EMMC_CARD_BLOCKADDR, rca, 0, 0, 0, 0, 0, 0);
}

int uniphier_rom_emmc_switch_part(int hwpart)
{
	return invoke_rom_fn(ROM_FN_EMMC_SWITCH_PART, hwpart, 0, 0, 0, 0, 0, 0);
}

int uniphier_rom_emmc_load_image_part(int hwpart, u32 block_addr,
				      void *load_addr, u32 block_cnt)
{
	return invoke_rom_fn(ROM_FN_EMMC_LOAD_IMAGE_PART, hwpart, block_addr,
			     (unsigned long)load_addr, block_cnt, 0, 0, 0);
}

int uniphier_rom_usb_load_image(void *hci_op, unsigned int sector,
				unsigned int size, void *buf)
{
	return invoke_rom_fn(ROM_FN_USB_LOAD_IMAGE, (unsigned long)hci_op,
			     sector, size, (unsigned long)buf, 0, 0, 0);
}

int uniphier_rom_secure_verify_nand_emmc(unsigned int addr, unsigned int size,
					 unsigned int key)
{
	return invoke_rom_fn(ROM_FN_SECURE_VERIFY_NAND_EMMC, addr, size, key,
			     0, 0, 0, 0);
}

int uniphier_rom_secure_verify_usb(unsigned int addr, unsigned int size,
				   unsigned int key)
{
	return invoke_rom_fn(ROM_FN_SECURE_VERIFY_USB, addr, size, key,
			     0, 0, 0, 0);
}

int uniphier_rom_secure_aes_decrypt(void *in, void *out, unsigned int size,
				    void *key, void *iv)
{
	return invoke_rom_fn(ROM_FN_SECURE_AES_DECRYPT, (unsigned long)in,
			     (unsigned long)out, size, (unsigned long)key,
			     (unsigned long)iv, 0, 0);
}

static u64 get_vbar(void)
{
	u64 vbar;

	asm volatile("mrs %0, vbar_el3" : "=r" (vbar) : : "cc");

	return vbar;
}

static void set_vbar(u64 vbar)
{
	asm volatile("msr vbar_el3, %0" : : "r" (vbar) : "cc");
}

static u64 orig_vector;

void uniphier_rom_api_prepare(void)
{
	orig_vector = get_vbar();
#if 1
	set_vbar(0x3000f000); /* ES1 */
#else
	set_vbar(0x00000800); /* ES2~*/
#endif
}

void uniphier_rom_api_unprepare(void)
{
	set_vbar(orig_vector);
}
