/*
 * Automatically converted for U-boot.
 * Original file in Diag: umc_es2_real_ch0-2x2_ch1-4x1_ch2-2x1_SCrefES1board_1333mhz.S
 */

int umc_init_sub(void)
{
	/********************************************************
	 * Initialization of UMC				*
	 ********************************************************/

	/* DIOのPowerUpシーケンス(HWシーケンサ制御) */
	writel(0x00000001, 0x5bc00340);	/* UMCANACTLA : ch0 */
	/* DIOのPowerUpシーケンス(HWシーケンサ制御) */
	writel(0x00000001, 0x5be00340);	/* UMCANACTLA : ch1 */
	umc_polling(0x5bc00340, 0x0, 0xfffffffe);	/* UMCANACTLA : ch0 */
	umc_polling(0x5be00340, 0x0, 0xfffffffe);	/* UMCANACTLA : ch1 */
	/* RQ_DRVENアサート */
	writel(0x00000100, 0x5bc00340);	/* UMCANACTLA : ch0 */
	/* RQ_DRVENアサート */
	writel(0x00000100, 0x5be00340);	/* UMCANACTLA : ch1 */
	/* DQ1ブロックのPowerUp */
	writel(0x00000002, 0x5dc00344);	/* ch2 */

	/********************************************************
	 * Initialization of DIO				*
	 ********************************************************/

	/* RQ0/RQ1 driver strength control */
	writel(0x00004747, 0x5bc03144);	/* ch0 */
	/* RQ2/RQ3 driver strength control */
	writel(0x00004747, 0x5bc03148);	/* ch0 */
	/* RQ4/RQ5 driver strength control */
	writel(0x00004747, 0x5bc0314C);	/* ch0 */
	/* RQ6/RQ7 driver strength control */
	writel(0x00004747, 0x5bc03150);	/* ch0 */
	/* RQ8/RQ9 driver strength control */
	writel(0x00004747, 0x5bc03154);	/* ch0 */
	/* RQ10/RQ11 driver strength control */
	writel(0x00004747, 0x5bc03158);	/* ch0 */
	/* RQ12/RQ13 driver strength control */
	writel(0x00004747, 0x5bc0315C);	/* ch0 */
	/* RQ14/RQ15 driver strength control */
	writel(0x00004747, 0x5bc03160);	/* ch0 */
	/* RQ16/RQ17 driver strength control */
	writel(0x00004747, 0x5bc03164);	/* ch0 */
	/* RQ18/RQ19 driver strength control */
	writel(0x00004747, 0x5bc03168);	/* ch0 */
	/* RQ20/RQ21 driver strength control */
	writel(0x00004747, 0x5bc0316C);	/* ch0 */
	/* RQ22〜RQ26 driver strength control */
	writel(0x00004747, 0x5bc03170);	/* ch0 */
	/* REST/CK driver strength control */
	writel(0x00004747, 0x5bc03174);	/* ch0 */
	/* CKN driver strength control */
	writel(0x00000047, 0x5bc03178);	/* ch0 */
	/* [1]コマンド/アドレス用遅延線0度固定制御信号(1：遅延0度固定（低消費電力)、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000002, 0x5bc03184);	/* ch0 */
	/* RQ0/RQ1 driver strength control */
	writel(0x00004747, 0x5be03144);	/* ch1 */
	/* RQ2/RQ3 driver strength control */
	writel(0x00004747, 0x5be03148);	/* ch1 */
	/* RQ4/RQ5 driver strength control */
	writel(0x00004747, 0x5be0314C);	/* ch1 */
	/* RQ6/RQ7 driver strength control */
	writel(0x00004747, 0x5be03150);	/* ch1 */
	/* RQ8/RQ9 driver strength control */
	writel(0x00004747, 0x5be03154);	/* ch1 */
	/* RQ10/RQ11 driver strength control */
	writel(0x00004747, 0x5be03158);	/* ch1 */
	/* RQ12/RQ13 driver strength control */
	writel(0x00004747, 0x5be0315C);	/* ch1 */
	/* RQ14/RQ15 driver strength control */
	writel(0x00004747, 0x5be03160);	/* ch1 */
	/* RQ16/RQ17 driver strength control */
	writel(0x00004747, 0x5be03164);	/* ch1 */
	/* RQ18/RQ19 driver strength control */
	writel(0x00004747, 0x5be03168);	/* ch1 */
	/* RQ20/RQ21 driver strength control */
	writel(0x00004747, 0x5be0316C);	/* ch1 */
	/* RQ22〜RQ26 driver strength control */
	writel(0x00004747, 0x5be03170);	/* ch1 */
	/* REST/CK driver strength control */
	writel(0x00004747, 0x5be03174);	/* ch1 */
	/* CKN driver strength control */
	writel(0x00000047, 0x5be03178);	/* ch1 */
	/* [1]コマンド/アドレス用遅延線0度固定制御信号(1：遅延0度固定（低消費電力)、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000002, 0x5be03184);	/* ch1 */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5bc04004);	/* ch0_A */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5bc040FC);	/* ch0_A */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5bc05004);	/* ch0_B */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5bc050FC);	/* ch0_B */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5bc04008);	/* ch0_A */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5bc04100);	/* ch0_A */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5bc05008);	/* ch0_B */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5bc05100);	/* ch0_B */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5bc0400C);	/* ch0_A */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5bc04104);	/* ch0_A */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5bc0500C);	/* ch0_B */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5bc05104);	/* ch0_B */
	/* DQA0/DQA1 driver strength control */
	writel(0x0000c7c7, 0x5bc04098);	/* ch0_A */
	/* DQA2/DQA3 driver strength control */
	writel(0x0000c7c7, 0x5bc0409C);	/* ch0_A */
	/* DQA4/DQA5 driver strength control */
	writel(0x0000c7c7, 0x5bc040A0);	/* ch0_A */
	/* DQA6/DQA7 driver strength control */
	writel(0x0000c7c7, 0x5bc040A4);	/* ch0_A */
	/* DMA/DQSA  driver strength control */
	writel(0x0000c7c7, 0x5bc040A8);	/* ch0_A */
	/* DQSNA     driver strength control */
	writel(0x000000c7, 0x5bc040AC);	/* ch0_A */
	/* DQB0/DQB1 driver strength control */
	writel(0x0000c7c7, 0x5bc04190);	/* ch0_A */
	/* DQB2/DQB3 driver strength control */
	writel(0x0000c7c7, 0x5bc04194);	/* ch0_A */
	/* DQB4/DQB5 driver strength control */
	writel(0x0000c7c7, 0x5bc04198);	/* ch0_A */
	/* DQB6/DQB7 driver strength control */
	writel(0x0000c7c7, 0x5bc0419C);	/* ch0_A */
	/* DMB/DQSB  driver strength control */
	writel(0x0000c7c7, 0x5bc041A0);	/* ch0_A */
	/* DQSNB     driver strength control */
	writel(0x000000c7, 0x5bc041A4);	/* ch0_A */
	/* DQA0/DQA1 driver strength control */
	writel(0x0000c7c7, 0x5bc05098);	/* ch0_B */
	/* DQA2/DQA3 driver strength control */
	writel(0x0000c7c7, 0x5bc0509C);	/* ch0_B */
	/* DQA4/DQA5 driver strength control */
	writel(0x0000c7c7, 0x5bc050A0);	/* ch0_B */
	/* DQA6/DQA7 driver strength control */
	writel(0x0000c7c7, 0x5bc050A4);	/* ch0_B */
	/* DMA/DQSA  driver strength control */
	writel(0x0000c7c7, 0x5bc050A8);	/* ch0_B */
	/* DQSNA     driver strength control */
	writel(0x000000c7, 0x5bc050AC);	/* ch0_B */
	/* DQB0/DQB1 driver strength control */
	writel(0x0000c7c7, 0x5bc05190);	/* ch0_B */
	/* DQB2/DQB3 driver strength control */
	writel(0x0000c7c7, 0x5bc05194);	/* ch0_B */
	/* DQB4/DQB5 driver strength control */
	writel(0x0000c7c7, 0x5bc05198);	/* ch0_B */
	/* DQB6/DQB7 driver strength control */
	writel(0x0000c7c7, 0x5bc0519C);	/* ch0_B */
	/* DMB/DQSB  driver strength control */
	writel(0x0000c7c7, 0x5bc051A0);	/* ch0_B */
	/* DQSNB     driver strength control */
	writel(0x000000c7, 0x5bc051A4);	/* ch0_B */
	/* DQA0/DQA1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc040b0);	/* ch0_A */
	/* DQA2/DQA3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc040b4);	/* ch0_A */
	/* DQA4/DQA5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc040b8);	/* ch0_A */
	/* DQA6/DQA7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc040bc);	/* ch0_A */
	/* DQSA  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc040c0);	/* ch0_A */
	/* DQB0/DQB1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc041a8);	/* ch0_A */
	/* DQB2/DQB3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc041ac);	/* ch0_A */
	/* DQB4/DQB5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc041b0);	/* ch0_A */
	/* DQB6/DQB7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc041b4);	/* ch0_A */
	/* DQSB  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc041b8);	/* ch0_A */
	/* DQA0/DQA1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc050b0);	/* ch0_B */
	/* DQA2/DQA3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc050b4);	/* ch0_B */
	/* DQA4/DQA5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc050b8);	/* ch0_B */
	/* DQA6/DQA7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc050bc);	/* ch0_B */
	/* DQSA  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc050c0);	/* ch0_B */
	/* DQB0/DQB1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc051a8);	/* ch0_B */
	/* DQB2/DQB3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc051ac);	/* ch0_B */
	/* DQB4/DQB5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc051b0);	/* ch0_B */
	/* DQB6/DQB7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc051b4);	/* ch0_B */
	/* DQSB  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5bc051b8);	/* ch0_B */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5bc040D0);	/* ch0_A */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5bc041C8);	/* ch0_A */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5bc050D0);	/* ch0_B */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5bc051C8);	/* ch0_B */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5be04004);	/* ch1_A */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5be040FC);	/* ch1_A */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5be05004);	/* ch1_B */
	/* DRVEN信号(DIO内部信号)EN区間調整 */
	writel(0x000014f0, 0x5be050FC);	/* ch1_B */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5be04008);	/* ch1_A */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5be04100);	/* ch1_A */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5be05008);	/* ch1_B */
	/* b0=static mode 0 */
	writel(0x00000407, 0x5be05100);	/* ch1_B */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5be0400C);	/* ch1_A */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5be04104);	/* ch1_A */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5be0500C);	/* ch1_B */
	/* ODTEN信号(DIO内部信号)EN区間調整 */
	writel(0x0000ffff, 0x5be05104);	/* ch1_B */
	/* DQA0/DQA1 driver strength control */
	writel(0x0000c7c7, 0x5be04098);	/* ch1_A */
	/* DQA2/DQA3 driver strength control */
	writel(0x0000c7c7, 0x5be0409C);	/* ch1_A */
	/* DQA4/DQA5 driver strength control */
	writel(0x0000c7c7, 0x5be040A0);	/* ch1_A */
	/* DQA6/DQA7 driver strength control */
	writel(0x0000c7c7, 0x5be040A4);	/* ch1_A */
	/* DMA/DQSA  driver strength control */
	writel(0x0000c7c7, 0x5be040A8);	/* ch1_A */
	/* DQSNA     driver strength control */
	writel(0x000000c7, 0x5be040AC);	/* ch1_A */
	/* DQB0/DQB1 driver strength control */
	writel(0x0000c7c7, 0x5be04190);	/* ch1_A */
	/* DQB2/DQB3 driver strength control */
	writel(0x0000c7c7, 0x5be04194);	/* ch1_A */
	/* DQB4/DQB5 driver strength control */
	writel(0x0000c7c7, 0x5be04198);	/* ch1_A */
	/* DQB6/DQB7 driver strength control */
	writel(0x0000c7c7, 0x5be0419C);	/* ch1_A */
	/* DMB/DQSB  driver strength control */
	writel(0x0000c7c7, 0x5be041A0);	/* ch1_A */
	/* DQSNB     driver strength control */
	writel(0x000000c7, 0x5be041A4);	/* ch1_A */
	/* DQA0/DQA1 driver strength control */
	writel(0x0000c7c7, 0x5be05098);	/* ch1_B */
	/* DQA2/DQA3 driver strength control */
	writel(0x0000c7c7, 0x5be0509C);	/* ch1_B */
	/* DQA4/DQA5 driver strength control */
	writel(0x0000c7c7, 0x5be050A0);	/* ch1_B */
	/* DQA6/DQA7 driver strength control */
	writel(0x0000c7c7, 0x5be050A4);	/* ch1_B */
	/* DMA/DQSA  driver strength control */
	writel(0x0000c7c7, 0x5be050A8);	/* ch1_B */
	/* DQSNA     driver strength control */
	writel(0x000000c7, 0x5be050AC);	/* ch1_B */
	/* DQB0/DQB1 driver strength control */
	writel(0x0000c7c7, 0x5be05190);	/* ch1_B */
	/* DQB2/DQB3 driver strength control */
	writel(0x0000c7c7, 0x5be05194);	/* ch1_B */
	/* DQB4/DQB5 driver strength control */
	writel(0x0000c7c7, 0x5be05198);	/* ch1_B */
	/* DQB6/DQB7 driver strength control */
	writel(0x0000c7c7, 0x5be0519C);	/* ch1_B */
	/* DMB/DQSB  driver strength control */
	writel(0x0000c7c7, 0x5be051A0);	/* ch1_B */
	/* DQSNB     driver strength control */
	writel(0x000000c7, 0x5be051A4);	/* ch1_B */
	/* DQA0/DQA1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be040b0);	/* ch1_A */
	/* DQA2/DQA3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be040b4);	/* ch1_A */
	/* DQA4/DQA5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be040b8);	/* ch1_A */
	/* DQA6/DQA7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be040bc);	/* ch1_A */
	/* DQSA  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be040c0);	/* ch1_A */
	/* DQB0/DQB1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be041a8);	/* ch1_A */
	/* DQB2/DQB3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be041ac);	/* ch1_A */
	/* DQB4/DQB5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be041b0);	/* ch1_A */
	/* DQB6/DQB7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be041b4);	/* ch1_A */
	/* DQSB  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be041b8);	/* ch1_A */
	/* DQA0/DQA1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be050b0);	/* ch1_B */
	/* DQA2/DQA3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be050b4);	/* ch1_B */
	/* DQA4/DQA5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be050b8);	/* ch1_B */
	/* DQA6/DQA7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be050bc);	/* ch1_B */
	/* DQSA  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be050c0);	/* ch1_B */
	/* DQB0/DQB1 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be051a8);	/* ch1_B */
	/* DQB2/DQB3 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be051ac);	/* ch1_B */
	/* DQB4/DQB5 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be051b0);	/* ch1_B */
	/* DQB6/DQB7 レシーバ制御 [15]:[7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be051b4);	/* ch1_B */
	/* DQSB  レシーバ制御 [7]レシーバ切り替え信号 (1:低電力 0:default)  */
	writel(0x00008080, 0x5be051b8);	/* ch1_B */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5be040D0);	/* ch1_A */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5be041C8);	/* ch1_A */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5be050D0);	/* ch1_B */
	/* [1]Writeクロックゲーティング制御信号(1：ゲーティング（低消費電力))、[0]遅延線の折り返しゲーティング制御信号(0：ゲーティング（低消費電力)) */
	writel(0x00000000, 0x5be051C8);	/* ch1_B */
	/* D0A_B D1A_B  Toffset設定値 */
	writel(0x00002020, 0x5bc01440);	/* ch0 */
	/* D0A_B D1A_B  Roffset設定値 */
	writel(0x00002020, 0x5bc01444);	/* ch0 */
	/* D0B WriteLeveling設定値 */
	writel(0x00000016, 0x5bc01200);	/* ch0 */
	/* D0A WriteLeveling設定値 */
	writel(0x00000017, 0x5bc01204);	/* ch0 */
	/* D1B WriteLeveling設定値 */
	writel(0x00000013, 0x5bc01260);	/* ch0 */
	/* D1A WriteLeveling設定値 */
	writel(0x00000011, 0x5bc01264);	/* ch0 */
	/* D0A  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5bc01244);	/* ch0 */
	/* D0B  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5bc01240);	/* ch0 */
	/* D1A  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5bc012A4);	/* ch0 */
	/* D1B  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5bc012A0);	/* ch0 */
	/* D0A_B D1A_B  Toffset設定値 */
	writel(0x00002020, 0x5be01440);	/* ch1 */
	/* D0A_B D1A_B  Roffset設定値 */
	writel(0x00002020, 0x5be01444);	/* ch1 */
	/* D0B WriteLeveling設定値 */
	writel(0x00000016, 0x5be01200);	/* ch1 */
	/* D0A WriteLeveling設定値 */
	writel(0x00000017, 0x5be01204);	/* ch1 */
	/* D1B WriteLeveling設定値 */
	writel(0x00000011, 0x5be01260);	/* ch1 */
	/* D1A WriteLeveling設定値 */
	writel(0x00000013, 0x5be01264);	/* ch1 */
	/* D0A  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5be01244);	/* ch1 */
	/* D0B  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5be01240);	/* ch1 */
	/* D1A  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5be012A4);	/* ch1 */
	/* D1B  DQSEN用遅延線角度情報 */
	writel(0x00000020, 0x5be012A0);	/* ch1 */
	/* DLL更新遅延設定 */
	writel(0x00000e0e, 0x5bc01608);	/* ch0 */
	/* CK　　角度情報  */
	writel(0x00000010, 0x5bc01370);	/* ch0 */
	/* DLL更新遅延設定 */
	writel(0x00000e0e, 0x5be01608);	/* ch1 */
	/* CK　　角度情報  */
	writel(0x00000010, 0x5be01370);	/* ch1 */
	/* Update Force */
	writel(0x00000000, 0x5bc0160C);	/* Update : ch0 */
	/* Update Force */
	writel(0x00000400, 0x5bc0160C);	/*  Force : ch0 */
	/* Update Force */
	writel(0x00000000, 0x5bc0160C);	/* ch0 */
	/* CK Update Force */
	writel(0x00008000, 0x5bc0160C);	/* CK用 : ch0 */
	/* CK Update Force */
	writel(0x00008400, 0x5bc0160C);	/* Update : ch0 */
	/* CK Update Force */
	writel(0x00000000, 0x5bc0160C);	/*  Force : ch0 */
	/* Update Force */
	writel(0x00000000, 0x5be0160C);	/* Update : ch1 */
	/* Update Force */
	writel(0x00000400, 0x5be0160C);	/*  Force : ch1 */
	/* Update Force */
	writel(0x00000000, 0x5be0160C);	/* ch1 */
	/* CK Update Force */
	writel(0x00008000, 0x5be0160C);	/* CK用 : ch1 */
	/* CK Update Force */
	writel(0x00008400, 0x5be0160C);	/* Update : ch1 */
	/* CK Update Force */
	writel(0x00000000, 0x5be0160C);	/*  Force : ch1 */

	adjust_duty();

	/********************************************************
	 * Post initialization of UMC
	 ********************************************************/

	/* DRAMコマンド間隔設定レジスタA */
	writel(0x66BB0F16, 0x5bc00000);	/* UMCCmdCtlA : ch0 */
	writel(0x66BB0F16, 0x5be00000);	/* ch1 */
	/* DRAMコマンド間隔設定レジスタA */
	writel(0x66BB0F16, 0x5dc00000);	/* ch2 */
	/* DRAMコマンド間隔設定レジスタB */
	writel(0x18C6AA44, 0x5bc00004);	/* UMCCmdCtlB : ch0 */
	writel(0x18C6AA44, 0x5be00004);	/* ch1 */
	writel(0x18C6AA44, 0x5dc00004);	/* ch2 */
	/* DRAM初期化コマンド間隔設定レジスタA */
	writel(0x5101387F, 0x5bc00008);	/* UMCInitCtlA : ch0 */
	writel(0x5101387F, 0x5be00008);	/* ch1 */
	writel(0x5101387F, 0x5dc00008);	/* ch2 */
	/* DRAM初期化コマンド間隔設定レジスタB */
	writel(0x43030D3F, 0x5bc0000c);	/* UMCInitCtlB : ch0 */
	writel(0x43030D3F, 0x5be0000c);	/* ch1 */
	writel(0x43030D3F, 0x5dc0000c);	/* ch2 */
	/* DRAM初期化コマンド間隔設定レジスタC */
	writel(0x00FF00FF, 0x5bc00010);	/* UMCInitCtlC : ch0 */
	writel(0x00FF00FF, 0x5be00010);	/* ch1 */
	writel(0x00FF00FF, 0x5dc00010);	/* ch2 */
	/* DRAM初期化MR0設定レジスタ */
	writel(0x00000B51, 0x5bc0001c);	/* UMCDrmMR0 : ch0 */
	writel(0x00000B51, 0x5be0001c);	/* ch1 */
	writel(0x00000B51, 0x5dc0001c);	/* ch2 */
	/* DRAM初期化MR1設定レジスタ */
	writel(0x00000006, 0x5bc00020);	/* UMCDrmMR1 : ch0 */
	writel(0x00000006, 0x5be00020);	/* ch1 */
	writel(0x00000006, 0x5dc00020);	/* ch2 */
	/* DRAM初期化MR2設定レジスタ */
	writel(0x00000210, 0x5bc00024);	/* UMCDrmMR2 : ch0 */
	writel(0x00000210, 0x5be00024);	/* ch1 */
	writel(0x00000210, 0x5dc00024);	/* ch2 */
	/* DRAM初期化MR3設定レジスタ */
	writel(0x00000000, 0x5bc00028);	/* UMCDrmMR3 : ch0 */
	writel(0x00000000, 0x5be00028);	/* ch1 */
	writel(0x00000000, 0x5dc00028);	/* ch2 */
	/* DRAM特殊コマンド間隔設定レジスタA、tREFIをJEDEC規格の5%マージンから算出 */
	writel(0x003F0a26, 0x5bc00030);	/* UMCSpcCtlA : ch0 */
	writel(0x003F0a26, 0x5be00030);	/* ch1 */
	writel(0x003F0a26, 0x5dc00030);	/* ch2 */
	/* DRAM特殊コマンド間隔設定レジスタB */
	writel(0x00ff0008, 0x5bc00034);	/* UMCSpcCtlB : ch0 */
	writel(0x00ff0008, 0x5be00034);	/* ch1 */
	writel(0x00ff0008, 0x5dc00034);	/* ch2 */
	writel(0x00000001, 0x5bc00014);	/* UMCInitSet : ch0 */
	writel(0x00000001, 0x5be00014);	/* UMCInitSet : ch1 */
	writel(0x00000001, 0x5dc00014);	/* UMCInitSet : ch2 */
	if (readl(0x5bc00058) == 0x00000000) {	/* VEEBOT : ch0 */
		umc_polling(0x5bc00018, 0x0, 0xfffffffe);	 /* UMCInitStat : ch0 */
	}
	if (readl(0x5be00058) == 0x00000000) {	/* VEEBOT : ch1 */
		umc_polling(0x5be00018, 0x0, 0xfffffffe);	 /* UMCInitStat : ch1 */
	}
	if (readl(0x5dc00058) == 0x00000000) {	/* VEEBOT : ch2 */
		umc_polling(0x5dc00018, 0x0, 0xfffffffe);	 /* UMCInitStat : ch2 */
	}
	writel(0x00190019, 0x5bc00600);	/* UMCRDATACTL_D0 : ch0 */
	writel(0x00190019, 0x5bc00608);	/* UMCRDATACTL_D1 : ch0 */
	writel(0x00190019, 0x5be00600);	/* UMCRDATACTL_D0 : ch1 */
	writel(0x00190019, 0x5dc00600);	/* UMCRDATACTL_D0 : ch2 */
	writel(0x02062806, 0x5bc00604);	/* UMCWDATACTL_D0 : ch0 */
	writel(0x02062806, 0x5bc0060c);	/* UMCWDATACTL_D1 : ch0 */
	writel(0x02062806, 0x5be00604);	/* UMCWDATACTL_D0 : ch1 */
	writel(0x02062806, 0x5dc00604);	/* UMCWDATACTL_D0 : ch2 */
	writel(0x04802000, 0x5bc00610);	/* UMCDATASET : ch0 */
	/* ch0 Read Dataラッチタイミング調整 */
	writel(0x04802000, 0x5be00610);	/* UMCDATASET : ch1 */
	writel(0x04802000, 0x5dc00610);	/* UMCDATASET : ch2 */
	/* ch1 Read Dataラッチタイミング調整 */
	writel(0x00000000, 0x5b803300);	/* WDIFLATSEL : ch0 */
	writel(0x00000000, 0x5b804300);	/* WDIFLATSEL : ch1 */
	/* ch0 Write Dataアサートタイミング調整 */
	writel(0x00000000, 0x5b807300);	/* WDIFLATSEL : ch2 */
	/* ch1 Read Dataラッチタイミング調整 */
	writel(0x0003C000, 0x5bc00348);	/* UMCANACTLC : ch0 */
	writel(0x0003C000, 0x5be00348);	/* UMCANACTLC : ch1 */
	/* ch1 Write Dataアサートタイミング調整 */
	writel(0x00400020, 0x5bc00720);	/* UMCDCCGCTL : ch0 */
	writel(0x00400020, 0x5be00720);	/* UMCDCCGCTL : ch1 */
	/* RCV_FIFWP_RST_DELAY(MDLLのリセット)の設定 */
	writel(0x00400020, 0x5dc00720);	/* UMCDCCGCTL : ch2 */
	/* RCV_FIFWP_RST_DELAY(MDLLのリセット)の設定 */
	writel(0x00000003, 0x5bc07000);	/* CLKEN_RWBUF_REG : ch0 */
	writel(0x00000003, 0x5be07000);	/* CLKEN_RWBUF_REG : ch1 */
	/* RCV_FIFWP_RST_DELAY(MDLLのリセット)の設定 */
	writel(0x00000003, 0x5dc07000);	/* CLKEN_RWBUF_REG : ch2 */
	/* RCV_FIFWP_RST_DELAY(MDLLのリセット)の設定 */
	writel(0x0000000f, 0x5bc08000);	/* CLKEN_DC_REG : ch0 */
	writel(0x0000000f, 0x5be08000);	/* CLKEN_DC_REG : ch1 */
	writel(0x0000000f, 0x5dc08000);	/* CLKEN_DC_REG : ch2 */
	/* DCのクロックゲーティングのタイミングを設定（REF用、コマンド処理用） */
	writel(0x0000017f, 0x5bc08004);	/* CLKEN_DIOREG_REG : ch0 */
	/* DCのクロックゲーティングのタイミングを設定（REF用、コマンド処理用） */
	writel(0x0000017f, 0x5be08004);	/* CLKEN_DIOREG_REG : ch1 */
	writel(0x0000017f, 0x5dc08004);	/* CLKEN_DIOREG_REG : ch2 */
	/* RBUF:WBUFクロックゲーティング */
	writel(0x00000061, 0x5bc08008);	/* CLKEN_OTHER_REG : ch0 */
	/* RBUF:WBUFクロックゲーティング */
	writel(0x00000061, 0x5be08008);	/* CLKEN_OTHER_REG : ch1 */
	writel(0x00000061, 0x5dc08008);	/* CLKEN_OTHER_REG : ch2 */
	/* DCのクロックゲーティングのON/OFF制御 */
	writel(0x0000003b, 0x5bc00724);	/* UMCDICGCTLA : ch0 */
	/* DCのクロックゲーティングのON/OFF制御 */
	writel(0x0000003b, 0x5be00724);	/* UMCDICGCTLA : ch1 */
	writel(0x0000003b, 0x5dc00724);	/* UMCDICGCTLA : ch2 */
	/* DIOREGのクロックゲーティングのON/OFF制御 */
	writel(0x020A0808, 0x5bc00728);	/* UMCDICGCTLB : ch0 */
	/* DIOREGのクロックゲーティングのON/OFF制御 */
	writel(0x020A0808, 0x5be00728);	/* UMCDICGCTLB : ch1 */
	writel(0x020A0808, 0x5dc00728);	/* UMCDICGCTLB : ch2 */
	/* HO3などその他クロックゲーティングのON/OFF制御 */
	writel(0x00000004, 0x5be00508);	/* UMCFLOWCTLG : ch1 */
	/* DIOのクロックゲーティングのON/OFF制御 */
	writel(0x00000004, 0x5dc00508);	/* UMCFLOWCTLG : ch2 */
	/* DIOのクロックゲーティングのON/OFF制御 */
	writel(0x80000201, 0x5b801c20);	/* async_config : ch0 */
	/* DIOのクロックゲーティングのタイミング設定（開始、終了） */
	writel(0x80000201, 0x5b802c20);	/* ch1 */
	/* DIOのクロックゲーティングのタイミング設定（開始、終了） */
	writel(0x80000201, 0x5b804c20);	/* ch2 */
	writel(0x0801e01e, 0x5bc00400);	/* UMCFlowCtlA : ch0 */
	writel(0x0801e01e, 0x5be00400);	/* ch1 */
	writel(0x0801e01e, 0x5dc00400);	/* ch2 */
	writel(0x00300000, 0x5bc00404);	/* UMCFlowCtlB : ch0 */
	writel(0x00200000, 0x5be00404);	/* ch1 */
	writel(0x00200000, 0x5dc00404);	/* ch2 */
	/* UMC1:UMC3間のcomstr信号設定 */
	writel(0x00004444, 0x5bc00408);	/* UMCFlowCtlC : ch0 */
	/* UMC1:UMC3間のcomstr信号設定 */
	writel(0x00004444, 0x5be00408);	/* ch1 */
	writel(0x00004444, 0x5dc00408);	/* ch2 */
	/* ch0 AutoRefreshコマンドフェッチタイミング */
	/* ch1 AutoRefreshコマンドフェッチタイミング */
	writel(0x200a0a00, 0x5bc0003c);	/* UMCSpcSetB : ch0 */
	writel(0x200a0a00, 0x5be0003c);	/* ch1 */
	/* ch0 前半部コマンド発行制御設定レジスタB(矩形分割ルール設定) */
	writel(0x200a0a00, 0x5dc0003c);	/* ch2 */
	/* ch1 前半部コマンド発行制御設定レジスタB(矩形分割ルール設定) */
	writel(0x00000000, 0x5b80b004);	/* CLKEN_OTHERS_REG : - */
	/* DRAMコマンド発行制御設定レジスタC(非同期パルス幅調整) */
	writel(0xffffffff, 0x5b80c004);	/* CLKEN_SSIF_REG200 : - */
	/* DRAMコマンド発行制御設定レジスタC(非同期パルス幅調整) */
	writel(0x07ffffff, 0x5b80c008);	/* CLKEN_OTHERS_REG : - */
	writel(0x00000000, 0x5b80b000);	/* INDIVRST_CTRL_REG : - */
	writel(0x00000000, 0x5b80c000);	/* INDIVRST_CTRL_REG : - */

	umc_ssif_start();

	return 0;
}
