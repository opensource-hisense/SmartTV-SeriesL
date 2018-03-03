/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */
#ifndef __MEMMAP_H__
#define __MEMMAP_H__

#ifndef __ASSEMBLY__
#include <linux/config.h>
#include <memmap/memtrans.h>
#endif

#include <memmap/memmap_data.h>

#define DDR_CH0_PHYADDR_OFFSET          0x80000000

#ifndef __ASSEMBLY__
#define DEV_EXMEM                       "/dev/mem"

#define DDR_CH0_OFFSET                  0x00000000
#define DDR_CH1_OFFSET                  0x20000000

#define DDR_CH0_SIZE                    0x20000000
#define DDR_CH1_SIZE                    0x20000000

#define HSC_START_OFFSET                DDR_HSC_OFFSET
#define HSC_SIZE                        DDR_HSC_SIZE
#endif 

#endif 
