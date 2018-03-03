/*** CONFIDENTIAL ***/
/* Copyright (C) 2015, Socionext Inc. */

#ifndef _MEMTRANS_H_
#define _MEMTRANS_H_

#if defined(__KERNEL__)
#include <linux/kernel.h>  
#else
#include <stdio.h>   
#endif 

#define DDR_CH0_OFFSET_P				(unsigned int)0x00000000
#define DDR_CH1_OFFSET_P				(unsigned int)0x20000000

#define DDR_CH0_SIZE_P					(unsigned int)0x20000000
#define DDR_CH1_SIZE_P					(unsigned int)0x20000000

#define DDR_CH0_START_P					(unsigned int)0x80000000
#define DDR_CH1_START_P					(unsigned int)(DDR_CH0_START_P+DDR_CH1_OFFSET_P)

#define DDR_CH0_MASK					(unsigned int)(DDR_CH0_SIZE_P-1)
#define DDR_CH1_MASK					(unsigned int)(DDR_CH1_SIZE_P-1)

#define DDR_CH0_KERNEL_START_P			0xffffffffffffffff
#define DDR_CH0_KERNEL_END_P			0xffffffffffffffff

#define DDR_CH1_KERNEL_START_P			0xffffffffffffffff
#define DDR_CH1_KERNEL_END_P			0xffffffffffffffff

#if defined(_BOOT_MINI) || defined(PIE_STYLE_LINUX)
#define	MEM_PRINTF			CSC_printf
#elif defined(__KERNEL__)
#define	MEM_PRINTF			printk
#else
#define	MEM_PRINTF			printf
#endif

#define MEM_CHECK_ARG
#ifdef MEM_CHECK_ARG
#define mem_bug(addr, file, line)	\
	    MEM_PRINTF("memtrans.h: ERROR: illegal access to 0x%08x in %s:%d\n",	\
	    	(unsigned int)addr, file, line)
	     
#else 
#define mem_bug(...)
#endif

#define MEM_BUG(x)						mem_bug(x, __FILE__, __LINE__)
#define MEM_bug(x)						mem_bug(x, file, line)

#if defined(__KERNEL__)
extern unsigned long DdrSize;
#elif defined(PIE_STYLE_LINUX)
#else
#include <stdlib.h>  
#endif 

#define DDR_USE_CH1_UNCACHE
#undef DDR_USE_CH1_UNCACHE

#define DDR_CHECK_ARG
#define EXBUS_CHECK_ARG

#define	DDR_PHYADDR_USER(x)						\
		DDR_phyaddr((unsigned int) (x), __FILE__, __LINE__)

#define	DDR_PHYADDR_KERN(x)						\
		DDR_phyaddr((unsigned int) (x), __FILE__, __LINE__)

#define DDR_PHYADDR_KERN_CACHE(x)		DDR_PHYADDR_KERN(x)

#if defined(__KERNEL__)
#define DDR_PHYADDR_KERN_UNCACHE(x)		(MEM_BUG(x))

#elif defined(PIE_STYLE_LINUX)
#else
#endif 

#define	DDR_PHYMEMADDR(x)						\
		DDR_phyaddr((unsigned int) (x), __FILE__, __LINE__)

static inline unsigned int
DDR_phyaddr(unsigned int offset, char *file, int line)
{
	unsigned int addr = 0;

	if (offset < DDR_CH0_SIZE_P) {	 
		addr = offset + DDR_CH0_START_P;
	} else if ( (DDR_CH1_OFFSET_P <= offset) && (offset <= (DDR_CH1_OFFSET_P+DDR_CH1_SIZE_P-1))) {	
		addr = offset + DDR_CH0_START_P;
	} else {
		MEM_bug(offset);
	}
	return addr;
}


#define	DDR_CH(x)							\
		DDR_ch((unsigned int) (x), __FILE__, __LINE__)

static inline unsigned int
DDR_ch(unsigned int offset, char *file, int line)
{
	unsigned int ch = 0;

	if (offset < DDR_CH0_SIZE_P) {	
		ch = 0;
	} else if ((DDR_CH1_OFFSET_P <= offset) && (offset <= (DDR_CH1_OFFSET_P + DDR_CH1_SIZE_P-1))) {	
		ch = 1;
	} else {
		MEM_bug(offset);
	}
	return ch;
}


#define	DDR_CHOFFSET(x)							\
		DDR_choffset((unsigned int) (x), __FILE__, __LINE__)

static inline unsigned int
DDR_choffset(unsigned int offset, char *file, int line)
{
	unsigned int ch_offset = 0;

	if (offset < DDR_CH0_SIZE_P) {
		ch_offset = offset & DDR_CH0_MASK;
	} else if ((DDR_CH1_OFFSET_P <= offset) && (offset <= (DDR_CH1_OFFSET_P+DDR_CH1_SIZE_P-1))) {
		ch_offset = offset & DDR_CH1_MASK;
	} else {
		MEM_bug(offset);
	}
	return ch_offset;
}


#if defined(__KERNEL__)
#define	DDR_VADDR_UNCACHE(x)		(MEM_BUG(x))

#elif defined(PIE_STYLE_LINUX)
#else
#define	DDR_VADDR_UNCACHE(x)		(MEM_BUG(x))

#endif 


#define EXBUS_PHYADDR_USER(x)			(MEM_BUG(x))

#define EXBUS_PHYADDR_KERN_CACHE(x)		(MEM_BUG(x))

#define EXBUS_PHYADDR_KERN_UNCACHE(x)	(MEM_BUG(x))

#define EXBUS_VADDR_UNCACHE(x)			(MEM_BUG(x))


#define	DDR_PHYADDR_USER_TO_CHOFFSET(x)		(MEM_BUG(x))


#define DDR_PHYADDR_TO_CHOFFSET(x)			(MEM_BUG(x))


#define	DDR_PHYADDR_USER_TO_CH(x)					\
		DDR_phyaddr_to_ch((unsigned int) (x), __FILE__, __LINE__)


#define DDR_PHYADDR_TO_CH(x)						\
		DDR_phyaddr_to_ch((unsigned int) (x), __FILE__, __LINE__)

static inline unsigned int
DDR_phyaddr_to_ch(unsigned int addr, char *file, int line)
{
	unsigned int ch = 0;

	if ((DDR_CH0_START_P <= addr) && (addr < (DDR_CH0_START_P+DDR_CH0_SIZE_P))) {	
		ch = 0;
	} else if ((DDR_CH1_START_P <= addr) && (addr <= (DDR_CH1_START_P+DDR_CH1_SIZE_P-1))) {	
		ch = 1;
	} else {
		MEM_bug(addr);
	}
	return ch;
}


#define	DDR_CH_AND_OFFSET_TO_PHYMEMADDR(x, y)				\
		DDR_ch_and_offset_to_phymemaddr((unsigned int) (x),		\
										(unsigned int) (y),		\
										__FILE__, __LINE__)

static inline unsigned int
DDR_ch_and_offset_to_phymemaddr(unsigned int ch, unsigned int offset,
								char *file, int line)
{
	unsigned int addr = 0;

	if ((ch == 0) && (offset < DDR_CH0_SIZE_P)) {	
		addr = offset + DDR_CH0_START_P;
	} else if ((ch == 1) && (offset < DDR_CH1_SIZE_P)) {	
		addr = offset + DDR_CH1_START_P;
	} else {
		MEM_bug(offset);
	}
	return addr;
}
	

#define DDR_CH_AND_OFFSET_TO_KERN_PHYADDR(x, y)			DDR_CH_AND_OFFSET_TO_PHYMEMADDR(x, y)

#define DDR_CH_AND_OFFSET_TO_KERN_CACHE_PHYADDR(x, y)	(MEM_BUG(x))


#if defined(__KERNEL__)
#define DDR_CH_AND_OFFSET_TO_KERN_UNCACHE_PHYADDR(x, y)	(MEM_BUG(x))

#elif defined(PIE_STYLE_LINUX)
#else
#endif 


#if defined(__KERNEL__)
#define DDR_CACHE_TO_UNCACHE(x)			(MEM_BUG(x))

#elif defined(PIE_STYLE_LINUX)
#define	DDR_CACHE_TO_UNCACHE(x)			(MEM_BUG(x))

#endif

#define DDR_UNCACHE_TO_CACHE(y)			(MEM_BUG(y))

#define DDR_UNCACHE_TO_PCI(a)			(MEM_BUG(a))
#define DDR_PCI_TO_UNCACHE(a)			(MEM_BUG(a))

#if defined(__KERNEL__)
#elif defined(PIE_STYLE_LINUX)
#define	DDR_PHYADDR_TO_VADDR(x)					\
		DDR_phyaddr_to_vaddr((unsigned int) (x), __FILE__, __LINE__)

static inline unsigned int
DDR_phyaddr_to_vaddr(unsigned int p_addr, char *file, int line)
{
	unsigned int v_addr = 0;

	if ( (DDR_CH0_START_P <= p_addr) && (p_addr <= (DDR_CH0_START_P+DDR_CH0_SIZE_P)) ) {	 
		v_addr = p_addr;
	} else if ( (DDR_CH1_START_P <= p_addr) && (p_addr <= (DDR_CH1_START_P+DDR_CH1_SIZE_P-1)) ) {	
		v_addr = p_addr;
	} else {
		MEM_bug(p_addr);
	}
	return v_addr;
}

#define	DDR_PHYADDR_TO_VADDR_UNCACHE(x)		(MEM_BUG(x))

#define	DDR_VADDR_TO_PHYADDR(x)		(MEM_BUG(x))

#else 
#define	DDR_PHYADDR_TO_VADDR_UNCACHE(x)		(MEM_BUG(x))

#endif 


#if defined(__KERNEL__)
#elif defined(PIE_STYLE_LINUX)
#else
#define	DDR_VADDR_UNCACHE_TO_MEMOFFSET(x)			(MEM_BUG(x))

#endif 


#if defined(__KERNEL__)
#elif defined(PIE_STYLE_LINUX)
#else
#define DDR_VADDR_CONTINUOUS(x)			(MEM_BUG(x))
#endif


#define REG_RADDR_TO_VADDR(x)			(MEM_BUG(x))

#define REG_RADDR_TO_KERNADDR(x)		(MEM_BUG(x))

#define VADDR_TO_REG_RADDR(x)			(MEM_BUG(x))

#define IO_FAKEADDR_TO_PHYADDR(x)		(MEM_BUG(x))

#define KERNADDR_TO_REG_RADDR(x)		(MEM_BUG(x))

#define DDR_PHYADDR_USER_TO_MEMOFFSET(x)	\
		DDR_phyaddr_user_to_memoffset((unsigned int) (x), __FILE__, __LINE__)


static inline unsigned int
DDR_phyaddr_user_to_memoffset(unsigned int addr, char *file, int line)
{
	unsigned int offset = 0;

	if ((DDR_CH0_START_P <= addr) && (addr < (DDR_CH0_START_P+DDR_CH0_SIZE_P))) { 
		offset = addr - DDR_CH0_START_P;
	} else if ((DDR_CH1_START_P <= addr) && (addr <= (DDR_CH1_START_P+DDR_CH1_SIZE_P-1))) { 
		offset = addr - DDR_CH0_START_P;
	} else {
		MEM_bug(addr);
	}
	return offset;
}

#define DDR_PHYADDR_KERN_TO_MEMOFFSET(x)		DDR_PHYADDR_USER_TO_MEMOFFSET(x)

#define DDR_PHYADDR_KERN_CACHE_TO_MEMOFFSET(x)	DDR_PHYADDR_USER_TO_MEMOFFSET(x)


#if defined(__KERNEL__)
#define DDR_PHYADDR_KERN_UNCACHE_TO_MEMOFFSET(x)	(MEM_BUG(x))

#elif defined(PIE_STYLE_LINUX)
#else
#endif


#define DDR_PHYMEMADDR_TO_MEMOFFSET(x)	DDR_PHYADDR_USER_TO_MEMOFFSET(x)


#define DDR_CH_AND_OFFSET_TO_MEMOFFSET(x, y)	\
		DDR_ch_and_offset_to_memoffset((unsigned int) (x),		\
										(unsigned int) (y),		\
										__FILE__, __LINE__)

static inline unsigned int
DDR_ch_and_offset_to_memoffset(unsigned int ch, unsigned int offset,
								char *file, int line)
{
	unsigned int addr = 0;

	if ((ch == 0) && (offset < DDR_CH0_SIZE_P)) {	
		addr = offset;
	} else if ((ch == 1) && (offset < DDR_CH1_SIZE_P)) {	
		addr = offset + DDR_CH1_OFFSET_P;
	} else {
		MEM_bug(offset);
	}
	return addr;
}

#endif 
