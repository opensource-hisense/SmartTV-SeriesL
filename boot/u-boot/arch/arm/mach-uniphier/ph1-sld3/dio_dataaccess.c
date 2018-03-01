/*
 * Copyright (C) 2011-2015 Panasonic Corporation
 */

#include <common.h>

#include "dio.h"

/*
 * Enable the following to accelerate the dio_datacpy
 * and dio_datacmp by using LDM and STM instruction.
 */
#define DIO_ARM_INLINE_ASM

void dio_datacpy(u32 addr, const u32 *data ,int length, int bus_mode)
{
	const u32 *data_end;
	data_end  = data + 4 * length;

	if (bus_mode == DIO_BUS32) {
#ifdef DIO_ARM_INLINE_ASM

		__asm__(
			"    mov r0, %0;"
			"    mov r1, %1;"
			"    mov r2, %2;"

			"    stmfd sp!, {r4-r7};"

			"dio_datacpy_bus32_loop:"
			"    cmp r1, r2;"
			"    bge dio_datacpy_bus32_loopend;"
			"    ldm r1!, {r4-r7};"
			"    stm r0!, {r4-r5};"
			"    add r0,  r0, #8;"
			"    stm r0!, {r6-r7};"
			"    add r0,  r0, #8;"
			"    b   dio_datacpy_bus32_loop;"

			"dio_datacpy_bus32_loopend:"
			"    ldmfd sp!, {r4-r7};"
			:
			: "r" (addr), "r" (data), "r" (data_end)
			: "r0", "r1", "r2"
			);
#else
		for (; data < data_end; data += 4) {
			write32(addr, data[0]); addr += 4;
			write32(addr, data[1]); addr += 12;
			write32(addr, data[2]); addr += 4;
			write32(addr, data[3]); addr += 12;
		}
#endif
	} else {
#ifdef DIO_ARM_INLINE_ASM
		__asm__(
			"    mov r0, %0;"
			"    mov r1, %1;"
			"    mov r2, %2;"

			"    stmfd sp!, {r4-r7};"

			"dio_datacpy_bus16_loop:"
			"    cmp r1, r2;"
			"    bge dio_datacpy_bus16_loopend;"
			"    ldm r1!, {r4-r7};"
			"    stm r0!, {r4-r7};"

			"    b   dio_datacpy_bus16_loop;"

			"dio_datacpy_bus16_loopend:"
			"    ldmfd sp!, {r4-r7};"
			:
			: "r" (addr), "r" (data), "r" (data_end)
			: "r0", "r1", "r2"
			);

#else
		for (; data < data_end; data += 4) {
			write32(addr, data[0]); addr += 4;
			write32(addr, data[1]); addr += 4;
			write32(addr, data[2]); addr += 4;
			write32(addr, data[3]); addr += 4;
		}
#endif
	}

	return;
}

u32 dio_datacmp(u32 addr, const u32 *data ,int length, int bus_mode)
{
	u32 diff;
	const u32 *data_end;
	data_end = data + 4 * length;

	if (bus_mode == DIO_BUS32) {
#ifdef DIO_ARM_INLINE_ASM
		__asm__(
			"    mov r0, %1;"
			"    mov r1, %2;"
			"    mov r2, %3;"

			"    stmfd sp!, {r4-r12, r14};"
			"    mov r3, #0;"

			"dio_datacmp_bus32_loop:"
			"    cmp r1, r2;"
			"    bge dio_datacmp_bus32_loopend;"
			"    ldm r1!, {r4-r7};"
			"    ldm r0!, {r8-r12, r14};"
			"    add r0,  r0, #8;"
			"    eor r8,  r8,  r4;"
			"    eor r9,  r9,  r5;"
			"    eor r12, r12, r6;"
			"    eor r14, r14, r7;"
			"    orr r3,  r3,  r8;"
			"    orr r3,  r3,  r9;"
			"    orr r3,  r3,  r12;"
			"    orr r3,  r3,  r14;"
			"    b   dio_datacmp_bus32_loop;"

			"dio_datacmp_bus32_loopend:"
			"    ldmfd sp!, {r4-r12, r14};"
			"    mov %0, r3;"
			: "=r" (diff)
			: "r" (addr), "r" (data), "r" (data_end)
			: "r0", "r1", "r2", "r3"
			);

#else
		diff = 0;

		for (; data < data_end; data += 4) {
			diff |= read32(addr) ^ data[0]; addr += 4;
			diff |= read32(addr) ^ data[1]; addr += 12;
			diff |= read32(addr) ^ data[2]; addr += 4;
			diff |= read32(addr) ^ data[3]; addr += 12;
		}
#endif
	} else {        // BUS 16bits mode
#ifdef DIO_ARM_INLINE_ASM

		__asm__(
			"    mov r0, %1;"
			"    mov r1, %2;"
			"    mov r2, %3;"

			"    stmfd sp!, {r4-r11};"
			"    mov r3, #0;"

			"dio_datacmp_bus16_loop:"
			"    cmp r1, r2;"
			"    bge dio_datacmp_bus16_loopend;"
			"    ldm r1!, {r4-r7};"
			"    ldm r0!, {r8-r11};"
			"    eor r8,  r8,  r4;"
			"    eor r9,  r9,  r5;"
			"    eor r10, r10, r6;"
			"    eor r11, r11, r7;"
			"    orr r3,  r3,  r8;"
			"    orr r3,  r3,  r9;"
			"    orr r3,  r3,  r10;"
			"    orr r3,  r3,  r11;"

			"    b   dio_datacmp_bus16_loop;"

			"dio_datacmp_bus16_loopend:"
			"    ldmfd sp!, {r4-r11};"
			"    mov %0, r3;"
			: "=r" (diff)
			: "r" (addr), "r" (data), "r" (data_end)
			: "r0", "r1", "r2", "r3"
			);
#else
		diff = 0;

		for (; data < data_end; data += 4) {
			diff |= read32(addr) ^ data[0]; addr += 4;
			diff |= read32(addr) ^ data[1]; addr += 4;
			diff |= read32(addr) ^ data[2]; addr += 4;
			diff |= read32(addr) ^ data[3]; addr += 4;
		}
#endif
	}

	return diff;
}
