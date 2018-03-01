#ifndef ARCH_DIO_REGS_H
#define ARCH_DIO_REGS_H

/*
 *  BASE
 */

#define DIO_BASE       0x5BC00000

#define DIO_CH_OFFSET  0x00200000
#define DIO_DQ_OFFSET  0x00001000

#define DIO_RC_BASE         (DIO_BASE + 0x00002000)

#define DIO_CH_BASE(ch)     (DIO_BASE + DIO_CH_OFFSET * (ch))

#define DIO_MDLL_BASE(ch)   (DIO_CH_BASE(ch) + 0x00001000)
#define DIO_RQ_BASE(ch)     (DIO_CH_BASE(ch) + 0x00003000)
#define DIO_DQ_BASE(ch, dq) (DIO_CH_BASE(ch) + 0x00004000 + DIO_DQ_OFFSET * (dq))



/*
 * ch   = 0..1
 * dq   = 0..1
 * byte = 0..1
 * bit  = 0..7
 */

//dqsen window adjust
#define DIO_DQSEN_ADJ(ch, dq, byte)   (DIO_DQ_BASE(ch, dq) + 0x08 + 0xF8 * (byte))

#define DIO_DQSFBKEN(ch, dq, byte)    (DIO_DQ_BASE(ch, dq) + 0xC4 + 0xF8 * (byte))

#define DIO_DQSFBK(ch, dq, byte, bit) (DIO_DQ_BASE(ch, dq) + 0xD4 + 0xF8 * (byte) + 0x4 * (bit))



//byte timing
#define DIO_BYTETIM_ALL(ch)        (DIO_MDLL_BASE(ch) + 0x400)

#define DIO_BYTETIM(ch, dq, byte)  (DIO_MDLL_BASE(ch) + 0x200 + 0x60 * (dq) + 0x4 * (1 - (byte)))




//ctl timing
#define DIO_CTLTIM(ch, dq, byte)   (DIO_MDLL_BASE(ch) + 0x240 + 0x60 * (dq) + 0x4 * (1 - (byte)))


//write timing
#define DIO_WTIM_ALL(ch)            (DIO_MDLL_BASE(ch) + 0x440)

#define DIO_WTIM_BYTE(ch)           (DIO_MDLL_BASE(ch) + 0x404 + 0x18 * (dq) + 0xC * (byte))

#define DIO_WTIM(ch, dq, byte, bit) (DIO_MDLL_BASE(ch) + 0x00 + 0x100 * (dq) + 0x20 * (1 - (byte)) + 0x4 * (bit))
#define DIO_WTIM_DQM(ch, dq, byte)  (DIO_MDLL_BASE(ch) + 0x230 + 0x60 * (dq) + 0x4 * (1 - (byte)))


//read timing
#define DIO_RTIM_CLOCK_ALL(ch)            (DIO_MDLL_BASE(ch) + 0x444)
#define DIO_RTIM_DATA_ALL(ch)             (DIO_MDLL_BASE(ch) + 0x448)

#define DIO_RTIM_CLOCK_BYTE(ch, dq, byte) (DIO_MDLL_BASE(ch) + 0x408 + 0x18 * (dq) + 0xC * (byte))
#define DIO_RTIM_DATA_BYTE(ch, dq, byte)  (DIO_MDLL_BASE(ch) + 0x40C + 0x18 * (dq) + 0xC * (byte))

#define DIO_RTIM_CLOCK(ch, dq, byte, bit) (DIO_MDLL_BASE(ch) + 0x80 + 0x100 * (dq) + 0x40 * (1 - (byte)) + 0x8 * (bit))
#define DIO_RTIM_DATA(ch, dq, byte, bit)  (DIO_MDLL_BASE(ch) + 0x84 + 0x100 * (dq) + 0x40 * (1 - (byte)) + 0x8 * (bit))


//misc1
#define DIO_MDLL_MISC1(ch)  (DIO_MDLL_BASE(ch) + 0x60C)

#define DIO_UPDATE_FORCE_MASK 0x0400


//ck timing
#define DIO_CKTIM(ch)       (DIO_MDLL_BASE(ch) + 0x370)

#endif /* ARCH_DIO_REGS_H */
