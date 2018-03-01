/*
 * Copyright (C) 2016 Socionext Inc.
 *   Author: Masahiro Yamada <yamada.masahiro@socionext.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef _ROM_API_H_
#define _ROM_API_H_

int uniphier_rom_nand_load_image(void *load_addr, u32 block, u32 page_cnt);

int uniphier_rom_emmc_load_image(u32 block_or_byte_addr, void *load_addr,
				 u32 block_cnt);
int uniphier_rom_emmc_send_cmd(u32 cmd, u32 arg);
int uniphier_rom_emmc_card_blockaddr(u32 rca);
int uniphier_rom_emmc_switch_part(int hwpart);
int uniphier_rom_emmc_load_image_part(int hwpart, u32 block_addr,
				      void *load_addr, u32 block_cnt);

int uniphier_rom_usb_load_image(void *hci_op, unsigned int sector,
				unsigned int size, void *buf);

int uniphier_rom_secure_verify_nand_emmc(unsigned int addr, unsigned int size,
					 unsigned int key);
int uniphier_rom_secure_verify_usb(unsigned int addr, unsigned int size,
				   unsigned int key);
int uniphier_rom_secure_aes_decrypt(void *in, void *out, unsigned int size,
				    void *key, void *iv);

void uniphier_rom_api_prepare(void);
void uniphier_rom_api_unprepare(void);

#endif /* _ROM_API_H_ */
