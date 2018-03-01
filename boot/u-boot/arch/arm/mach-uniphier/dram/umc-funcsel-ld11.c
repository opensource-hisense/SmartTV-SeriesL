#include <linux/io.h>

#include "../init.h"

#define UMC_BASE_SBUS 0x5b800000
#define UMC_BASE 0x5bc00000

#define UMC_CA_BASE(ch) (UMC_BASE_SBUS + 0x1000*(ch+1))

#define UMC_COMSIZE_1D_L1_(ch)	(UMC_CA_BASE(ch) + 0xFD0)
#define UMC_COMSIZE_1D_L2_(ch)	(UMC_CA_BASE(ch) + 0xFD4)
#define UMC_COMSIZE_1D_L3_(ch)	(UMC_CA_BASE(ch) + 0xFD8)
#define UMC_COMSIZE_1D_L4_(ch)	(UMC_CA_BASE(ch) + 0xFDC)
#define UMC_COMSIZE_1D_L5_(ch)	(UMC_CA_BASE(ch) + 0xFE0)
#define UMC_COMSIZE_1D_L6_(ch)	(UMC_CA_BASE(ch) + 0xFE4)
#define UMC_COMSIZE_1D_L7_(ch)	(UMC_CA_BASE(ch) + 0xFE8)
#define UMC_COMSIZE_1D_L8_(ch)	(UMC_CA_BASE(ch) + 0xFEC)

#define UMC_RATECONT_ICS(ch) (UMC_CA_BASE(ch) + 0x0)
#define UMC_RATECONT_ICA(ch) (UMC_CA_BASE(ch) + 0x4)
#define UMC_RATECONT_IDS(ch) (UMC_CA_BASE(ch) + 0x8)
#define UMC_RATECONT_ID2(ch) (UMC_CA_BASE(ch) + 0xC)
#define UMC_RATECONT_AR0(ch) (UMC_CA_BASE(ch) + 0x10)
#define UMC_RATECONT_VG2(ch) (UMC_CA_BASE(ch) + 0x14)
#define UMC_RATECONT_SIO(ch) (UMC_CA_BASE(ch) + 0x18)
#define UMC_RATECONT_VO0(ch) (UMC_CA_BASE(ch) + 0x1C)
#define UMC_RATECONT_VPE(ch) (UMC_CA_BASE(ch) + 0x20)
#define UMC_RATECONT_VPD(ch) (UMC_CA_BASE(ch) + 0x24)
#define UMC_RATECONT_RGL(ch) (UMC_CA_BASE(ch) + 0x28)
#define UMC_RATECONT_A2D(ch) (UMC_CA_BASE(ch) + 0x2C)
#define UMC_RATECONT_DMD(ch) (UMC_CA_BASE(ch) + 0x30)
#define UMC_RATECONT_REF(ch) (UMC_CA_BASE(ch) + 0x34)

#define UMC_MASTEROPTION_ICS(ch) (UMC_CA_BASE(ch) + 0x200)
#define UMC_MASTEROPTION_ICA(ch) (UMC_CA_BASE(ch) + 0x204)
#define UMC_MASTEROPTION_IDS(ch) (UMC_CA_BASE(ch) + 0x208)
#define UMC_MASTEROPTION_ID2(ch) (UMC_CA_BASE(ch) + 0x20C)
#define UMC_MASTEROPTION_AR0(ch) (UMC_CA_BASE(ch) + 0x210)
#define UMC_MASTEROPTION_VG2(ch) (UMC_CA_BASE(ch) + 0x214)
#define UMC_MASTEROPTION_SIO(ch) (UMC_CA_BASE(ch) + 0x218)
#define UMC_MASTEROPTION_VO0(ch) (UMC_CA_BASE(ch) + 0x21C)
#define UMC_MASTEROPTION_VPE(ch) (UMC_CA_BASE(ch) + 0x220)
#define UMC_MASTEROPTION_VPD(ch) (UMC_CA_BASE(ch) + 0x224)
#define UMC_MASTEROPTION_RGL(ch) (UMC_CA_BASE(ch) + 0x228)
#define UMC_MASTEROPTION_A2D(ch) (UMC_CA_BASE(ch) + 0x22C)
#define UMC_MASTEROPTION_DMD(ch) (UMC_CA_BASE(ch) + 0x230)
#define UMC_MASTEROPTION_REF(ch) (UMC_CA_BASE(ch) + 0x234)

#define UMC_PRIORITY_ICS(ch) (UMC_CA_BASE(ch) + 0x400)
#define UMC_PRIORITY_ICA(ch) (UMC_CA_BASE(ch) + 0x404)
#define UMC_PRIORITY_IDS(ch) (UMC_CA_BASE(ch) + 0x408)
#define UMC_PRIORITY_ID2(ch) (UMC_CA_BASE(ch) + 0x40C)
#define UMC_PRIORITY_AR0(ch) (UMC_CA_BASE(ch) + 0x410)
#define UMC_PRIORITY_VG2(ch) (UMC_CA_BASE(ch) + 0x414)
#define UMC_PRIORITY_SIO(ch) (UMC_CA_BASE(ch) + 0x418)
#define UMC_PRIORITY_VO0(ch) (UMC_CA_BASE(ch) + 0x41C)
#define UMC_PRIORITY_VPE(ch) (UMC_CA_BASE(ch) + 0x420)
#define UMC_PRIORITY_VPD(ch) (UMC_CA_BASE(ch) + 0x424)
#define UMC_PRIORITY_RGL(ch) (UMC_CA_BASE(ch) + 0x428)
#define UMC_PRIORITY_A2D(ch) (UMC_CA_BASE(ch) + 0x42C)
#define UMC_PRIORITY_DMD(ch) (UMC_CA_BASE(ch) + 0x430)
#define UMC_PRIORITY_REF(ch) (UMC_CA_BASE(ch) + 0x434)

#define UMC_RATE_CHANGE(ch) (UMC_CA_BASE(ch) + 0xA00)
#define UMC_DIR_ARB(ch)      (UMC_CA_BASE(ch) + 0xA90)
#define UMC_BNK_ARB_MODE(ch) (UMC_CA_BASE(ch) + 0xA94)

#define UMC_CHSEL_DMD (UMC_BASE + 0x08F0)

#define UMC_FUNC_DATA_END (0xffff)

#define TOTAL_CH 2

typedef struct _umc_addrdata_t{
  u32 addr;
  u32 data;
} umc_addrdata_t;

static const umc_addrdata_t UMC_FUNC_CORETEST[] = {
    { UMC_MASTEROPTION_ICS(0),    0x00000004},
    { UMC_MASTEROPTION_ICA(0),    0x00000004},
    { UMC_MASTEROPTION_IDS(0),    0x00000010},
    { UMC_MASTEROPTION_ID2(0),    0x00000010},
    { UMC_MASTEROPTION_AR0(0),    0x00000004},
    { UMC_MASTEROPTION_VG2(0),    0x00000010},
    { UMC_MASTEROPTION_SIO(0),    0x00000010},
    { UMC_MASTEROPTION_VO0(0),    0x00000010},
    { UMC_MASTEROPTION_VPE(0),    0x00000010},
    { UMC_MASTEROPTION_VPD(0),    0x00000010},
    { UMC_MASTEROPTION_RGL(0),    0x00000010},
    { UMC_MASTEROPTION_A2D(0),    0x00000010},
    { UMC_MASTEROPTION_DMD(0),    0x00000010},
    { UMC_MASTEROPTION_REF(0),    0x00000010},
    { UMC_MASTEROPTION_ICS(1),    0x00000004},
    { UMC_MASTEROPTION_ICA(1),    0x00000004},
    { UMC_MASTEROPTION_IDS(1),    0x00000010},
    { UMC_MASTEROPTION_ID2(1),    0x00000010},
    { UMC_MASTEROPTION_AR0(1),    0x00000004},
    { UMC_MASTEROPTION_VG2(1),    0x00000010},
    { UMC_MASTEROPTION_SIO(1),    0x00000010},
    { UMC_MASTEROPTION_VO0(1),    0x00000010},
    { UMC_MASTEROPTION_VPE(1),    0x00000010},
    { UMC_MASTEROPTION_VPD(1),    0x00000010},
    { UMC_MASTEROPTION_RGL(1),    0x00000010},
    { UMC_MASTEROPTION_A2D(1),    0x00000010},
    { UMC_MASTEROPTION_DMD(1),    0x00000010},
    { UMC_MASTEROPTION_REF(1),    0x00000010},
    { UMC_PRIORITY_ICS(0),    0x00010000},
    { UMC_PRIORITY_ICA(0),    0x01020000},
    { UMC_PRIORITY_IDS(0),    0x02000009},
    { UMC_PRIORITY_ID2(0),    0x0300000A},
    { UMC_PRIORITY_AR0(0),    0x04000000},
    { UMC_PRIORITY_VG2(0),    0x05000001},
    { UMC_PRIORITY_SIO(0),    0x06000002},
    { UMC_PRIORITY_VO0(0),    0x07000003},
    { UMC_PRIORITY_VPE(0),    0x08000004},
    { UMC_PRIORITY_VPD(0),    0x09000005},
    { UMC_PRIORITY_RGL(0),    0x0A000006},
    { UMC_PRIORITY_A2D(0),    0x0B000007},
    { UMC_PRIORITY_DMD(0),    0x0C000008},
    { UMC_PRIORITY_REF(0),    0x0D000000},
    { UMC_PRIORITY_ICS(1),    0x00010000},
    { UMC_PRIORITY_ICA(1),    0x01020000},
    { UMC_PRIORITY_IDS(1),    0x02000009},
    { UMC_PRIORITY_ID2(1),    0x0300000A},
    { UMC_PRIORITY_AR0(1),    0x04000000},
    { UMC_PRIORITY_VG2(1),    0x05000001},
    { UMC_PRIORITY_SIO(1),    0x06000002},
    { UMC_PRIORITY_VO0(1),    0x07000003},
    { UMC_PRIORITY_VPE(1),    0x08000004},
    { UMC_PRIORITY_VPD(1),    0x09000005},
    { UMC_PRIORITY_RGL(1),    0x0A000006},
    { UMC_PRIORITY_A2D(1),    0x0B000007},
    { UMC_PRIORITY_DMD(1),    0x0C000008},
    { UMC_PRIORITY_REF(1),    0x0D000000},
    { UMC_CHSEL_DMD,    0x03000100 },
    { UMC_FUNC_DATA_END, 0 }
};

const umc_addrdata_t UMC_FUNC_MAXLOAD_COMMON[] = {
    { UMC_RATECONT_ICA(0),    0x03B640B6},
    { UMC_RATECONT_AR0(0),    0x05004200},
    { UMC_RATECONT_SIO(0),    0x04CB41CB},
    { UMC_RATECONT_VO0(0),    0x03624062},
    { UMC_RATECONT_VPE(0),    0x03204020},
    { UMC_RATECONT_A2D(0),    0x07D244D2},
    { UMC_RATECONT_DMD(0),    0x04D341D3},
    { UMC_RATECONT_REF(0),    0x060C430C},
    { UMC_RATECONT_AR0(1),    0x03AA40AA},
    { UMC_RATECONT_VG2(1),    0x03164016},
    { UMC_RATECONT_VO0(1),    0x03364036},
    { UMC_RATECONT_VPE(1),    0x03604060},
    { UMC_RATECONT_VPD(1),    0x038A408A},
    { UMC_RATECONT_A2D(1),    0x07D244D2},
    { UMC_RATECONT_REF(1),    0x060C430C},
    { UMC_MASTEROPTION_ICS(0),    0x00000004},
    { UMC_MASTEROPTION_ICA(0),    0x00000005},
    { UMC_MASTEROPTION_IDS(0),    0x00000010},
    { UMC_MASTEROPTION_ID2(0),    0x00000010},
    { UMC_MASTEROPTION_AR0(0),    0x00000805},
    { UMC_MASTEROPTION_VG2(0),    0x00000010},
    { UMC_MASTEROPTION_SIO(0),    0x00000011},
    { UMC_MASTEROPTION_VO0(0),    0x00000801},
    { UMC_MASTEROPTION_VPE(0),    0x00000811},
    { UMC_MASTEROPTION_VPD(0),    0x00000010},
    { UMC_MASTEROPTION_RGL(0),    0x00000010},
    { UMC_MASTEROPTION_A2D(0),    0x00000011},
    { UMC_MASTEROPTION_DMD(0),    0x00001011},
    { UMC_MASTEROPTION_REF(0),    0x00000011},
    { UMC_MASTEROPTION_ICS(1),    0x00000004},
    { UMC_MASTEROPTION_ICA(1),    0x00000004},
    { UMC_MASTEROPTION_IDS(1),    0x00000010},
    { UMC_MASTEROPTION_ID2(1),    0x00000010},
    { UMC_MASTEROPTION_AR0(1),    0x00000805},
    { UMC_MASTEROPTION_VG2(1),    0x00000811},
    { UMC_MASTEROPTION_SIO(1),    0x00000010},
    { UMC_MASTEROPTION_VO0(1),    0x00000801},
    { UMC_MASTEROPTION_VPE(1),    0x00000811},
    { UMC_MASTEROPTION_VPD(1),    0x00000811},
    { UMC_MASTEROPTION_RGL(1),    0x00000010},
    { UMC_MASTEROPTION_A2D(1),    0x00000011},
    { UMC_MASTEROPTION_DMD(1),    0x00001010},
    { UMC_MASTEROPTION_REF(1),    0x00000011},
    { UMC_PRIORITY_ICS(0),    0x00010000},
    { UMC_PRIORITY_ICA(0),    0x01020000},
    { UMC_PRIORITY_IDS(0),    0x02000009},
    { UMC_PRIORITY_ID2(0),    0x0300000A},
    { UMC_PRIORITY_AR0(0),    0x04000000},
    { UMC_PRIORITY_VG2(0),    0x05000001},
    { UMC_PRIORITY_SIO(0),    0x06000002},
    { UMC_PRIORITY_VO0(0),    0x07000003},
    { UMC_PRIORITY_VPE(0),    0x08000004},
    { UMC_PRIORITY_VPD(0),    0x09000005},
    { UMC_PRIORITY_RGL(0),    0x0A000006},
    { UMC_PRIORITY_A2D(0),    0x0B000007},
    { UMC_PRIORITY_DMD(0),    0x0C000008},
    { UMC_PRIORITY_REF(0),    0x0D000000},
    { UMC_PRIORITY_ICS(1),    0x00010000},
    { UMC_PRIORITY_ICA(1),    0x01020000},
    { UMC_PRIORITY_IDS(1),    0x02000009},
    { UMC_PRIORITY_ID2(1),    0x0300000A},
    { UMC_PRIORITY_AR0(1),    0x04000000},
    { UMC_PRIORITY_VG2(1),    0x05000001},
    { UMC_PRIORITY_SIO(1),    0x06000002},
    { UMC_PRIORITY_VO0(1),    0x07000003},
    { UMC_PRIORITY_VPE(1),    0x08000004},
    { UMC_PRIORITY_VPD(1),    0x09000005},
    { UMC_PRIORITY_RGL(1),    0x0A000006},
    { UMC_PRIORITY_A2D(1),    0x0B000007},
    { UMC_PRIORITY_DMD(1),    0x0C000008},
    { UMC_PRIORITY_REF(1),    0x0D000000},
    { UMC_CHSEL_DMD,    0x03000100 },
    { UMC_FUNC_DATA_END, 0 }
};

static const umc_addrdata_t comsize_table_sld11[] = {
	/*** For DDR3-CH0 ***/
	{UMC_COMSIZE_1D_L1_(0),     0x01001002}, /*lsz=01, cls=00, pin=0, ett=0, dir=w, CH=1, oh=32 (=> setval=2)*/
	{UMC_COMSIZE_1D_L2_(0),     0x02001004}, /*lsz=02, cls=00, pin=0, ett=0, dir=w, CH=1, oh=64 (=> setval=4)*/
	{UMC_COMSIZE_1D_L3_(0),     0x03001006}, /*lsz=03, cls=00, pin=0, ett=0, dir=w, CH=1, oh=96 (=> setval=6)*/
	{UMC_COMSIZE_1D_L4_(0),     0x04001008}, /*lsz=04, cls=00, pin=0, ett=0, dir=w, CH=1, oh=128 (=> setval=8)*/
	{UMC_COMSIZE_1D_L5_(0),     0x0600100C}, /*lsz=06, cls=00, pin=0, ett=0, dir=w, CH=1, oh=192 (=> setval=12)*/
	{UMC_COMSIZE_1D_L6_(0),     0x08001010}, /*lsz=08, cls=00, pin=0, ett=0, dir=w, CH=1, oh=256 (=> setval=16)*/
	{UMC_COMSIZE_1D_L7_(0),     0x0c001018}, /*lsz=12, cls=00, pin=0, ett=0, dir=w, CH=1, oh=384 (=> setval=24)*/
	{UMC_COMSIZE_1D_L8_(0),     0x10001020}, /*lsz=16, cls=00, pin=0, ett=0, dir=w, CH=1, oh=512 (=> setval=32)*/
	
	/*** For DDR3-CH1 ***/
	{UMC_COMSIZE_1D_L1_(1),     0x01001002}, /*lsz=01, cls=00, pin=0, ett=0, dir=w, CH=1, oh=32 (=> setval=2)*/
	{UMC_COMSIZE_1D_L2_(1),     0x02001004}, /*lsz=02, cls=00, pin=0, ett=0, dir=w, CH=1, oh=64 (=> setval=4)*/
	{UMC_COMSIZE_1D_L3_(1),     0x03001006}, /*lsz=03, cls=00, pin=0, ett=0, dir=w, CH=1, oh=96 (=> setval=6)*/
	{UMC_COMSIZE_1D_L4_(1),     0x04001008}, /*lsz=04, cls=00, pin=0, ett=0, dir=w, CH=1, oh=128 (=> setval=8)*/
	{UMC_COMSIZE_1D_L5_(1),     0x0600100C}, /*lsz=06, cls=00, pin=0, ett=0, dir=w, CH=1, oh=192 (=> setval=12)*/
	{UMC_COMSIZE_1D_L6_(1),     0x08001010}, /*lsz=08, cls=00, pin=0, ett=0, dir=w, CH=1, oh=256 (=> setval=16)*/
	{UMC_COMSIZE_1D_L7_(1),     0x0c001018}, /*lsz=12, cls=00, pin=0, ett=0, dir=w, CH=1, oh=384 (=> setval=24)*/
	{UMC_COMSIZE_1D_L8_(1),     0x10001020}, /*lsz=16, cls=00, pin=0, ett=0, dir=w, CH=1, oh=512 (=> setval=32)*/
	
	{UMC_FUNC_DATA_END, 0x0 }
};

static inline void write32(u32 addr, u32 data)
{
	writel(data, (void __iomem *)(u64)addr);
}

static inline u32 read32(u32 addr)
{
	return readl((void __iomem *)(u64)addr);
}

void uniphier_ld11_umc_funcsel(void)
{

	const umc_addrdata_t *p;
	int ch;

	/* set table for command size rate control */
	for(p = comsize_table_sld11; p->addr != UMC_FUNC_DATA_END; p++) {
		write32(p->addr, p->data);
	}

	/* set table for bandwidth setting */
	for(p = UMC_FUNC_MAXLOAD_COMMON; p->addr != UMC_FUNC_DATA_END; p++) {
		write32(p->addr, p->data);
	}
	//	/* Write Funcsel Index */
	//write32(UMCDEBUGD(TOTAL_CH - 1), p->data);

	/* set rate change register */
	for(ch = 0; ch < TOTAL_CH; ch++){
		write32(UMC_RATE_CHANGE(ch), 0x00000001);
	}

	/* register polling ( wait until UMC_RATE_CHANGE is 0x00000000 ) */
	for(ch = 0; ch < TOTAL_CH; ch++){
		while(read32(UMC_RATE_CHANGE(ch)))
			;
	}

	/* direction history arbitration mode off (temporary) */
	for(ch = 0; ch < TOTAL_CH; ch++){
		write32(UMC_DIR_ARB(ch), 0x00000000);
	}

	/* bank history arbitration mode off (temporary) */
	for(ch = 0; ch < TOTAL_CH; ch++){
		write32(UMC_BNK_ARB_MODE(ch), 0x00000000);
	}

}
