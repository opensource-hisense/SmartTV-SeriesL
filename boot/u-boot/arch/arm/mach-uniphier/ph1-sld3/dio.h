#ifndef ARCH_DIO_H
#define ARCH_DIO_H

#include <config.h>
#include <linux/types.h>

#define SDRAM_BB_SIZE_CH0 CONFIG_SDRAM0_SIZE
#define SDRAM_BB_SIZE_CH1 CONFIG_SDRAM1_SIZE
#define SDRAM_BB_SIZE_CH2 CONFIG_SDRAM2_SIZE

#define SDRAM_BB_BASEADDR_CH0 CONFIG_SDRAM0_BASE
#define SDRAM_BB_BASEADDR_CH1 CONFIG_SDRAM1_BASE
#define SDRAM_BB_BASEADDR_CH2 CONFIG_SDRAM2_BASE

/**
 * DDRインターフェース全体で, DIOのDQ block (16bit単位)が何個あるかを指定するマクロ.
 * (DQ_BLOCK * 16)がDDRインターフェースのbit幅になる.
 */
#if CONFIG_DDR_NUM_CH2 > 0
# define DQ_BLOCKS 4
#else
# define DQ_BLOCKS 3
#endif

#define ROFFSET_POSBASE -2
#define ROFFSET_NEGBASE  2

/**
 * Write Leveling関連のレジスタ設定値を格納する構造体.
 * dio_get_wl_regs(), dio_set_wl_reg()関数を使って読み書きする.
 */
struct dio_wl_regs{
    /** CKに対する, DQS, DQ, DQM等の遅延量のレジスタ設定値. Byte単位. */
    u8 clock[DQ_BLOCKS][2];
};

/**
 * DQSEN Adjust関連のレジスタ設定値を格納する構造体.
 * dio_get_dqsen_regs(), dio_set_dqsen_reg()関数を使って読み書きする.
 */
struct dio_dqsen_regs{
    /** 粗調整(ロジックでの調整)での遅延量のレジスタ設定値. Byte単位. */
    u8 adj[DQ_BLOCKS][2];
    /** 精調整(高性能遅延回路での調整)での遅延量のレジスタ設定値. Byte単位. */
    u8 ctl[DQ_BLOCKS][2];
};

/**
 * Toffset調整関連のレジスタ設定値を格納する構造体.
 * dio_get_toffset_regs(), dio_set_toffset_reg()関数を使って読み書きする.
 */
struct dio_toffset_regs{
    /** Write方向, DQSに対するDQの遅延量. 内部クロックライン. Bit単位. */
    u8 clock[DQ_BLOCKS][16];
    /** Write方向, DQSに対するDQの遅延量. 内部データライン. 通常, clockと同じ値を指定. Bit単位. */
    u8 data[DQ_BLOCKS][16];
    /** Write方向, DQSに対するDQMの遅延量. 内部クロックライン. Byte単位. */
    u8 mclock[DQ_BLOCKS][2];
    /** Write方向, DQSに対するDQMの遅延量. 内部データライン. 通常, mclockと同じ値を指定. Byte単位. */
    u8 mdata[DQ_BLOCKS][2];
};

/**
 * Roffset調整関連のレジスタ設定値を格納する構造体.
 * dio_get_roffset_regs(), dio_set_roffset_reg()関数を使って読み書きする.
 */
struct dio_roffset_regs{
    /** Read方向, DQを叩くDQSのSoC内部信号の遅延量. Rizing Edge, Bit単位. */
    u8 clockp[DQ_BLOCKS][16];
    /** Read方向, DQを叩くDQSのSoC内部信号の遅延量. Falling Edge, Bit単位. */
    u8 clockn[DQ_BLOCKS][16];
    /** Read方向, DQの遅延量. 通常は0指定. Bit単位. */
    u8 data[DQ_BLOCKS][16];
};

/**
 * Write Leveling関連のレジスタ設定値を取得する関数.
 * @param wl_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_get_wl_regs(struct dio_wl_regs *wl_regs);

/**
 * Write Leveling関連のレジスタ設定値を設定する関数.
 * @param wl_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_set_wl_regs(const struct dio_wl_regs *wl_regs);

/**
 * DQSEN Adjust関連のレジスタ設定値を取得する関数.
 * @param dqsen_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_get_dqsen_regs(struct dio_dqsen_regs *dqsen_regs);

/**
 * DQSEN Adjust関連のレジスタ設定値を設定する関数.
 * @param dqsen_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_set_dqsen_regs(const struct dio_dqsen_regs *dqsen_regs);

/**
 * Toffset調整関連のレジスタ設定値を取得する関数.
 * @param toffset_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_get_toffset_regs(struct dio_toffset_regs *toffset_regs);

/**
 * Toffset調整関連のレジスタ設定値を設定する関数.
 * @param toffset_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_set_toffset_regs(const struct dio_toffset_regs *toffset_regs);

/**
 * Roffset調整関連のレジスタ設定値を取得する関数.
 * @param roffset_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_get_roffset_regs(struct dio_roffset_regs *roffset_regs);

/**
 * Roffset調整関連のレジスタ設定値を設定する関数.
 * @param roffset_regs レジスタ設定を格納する構造体へのポインタ.
 * @return None
 */
void dio_set_roffset_regs(const struct dio_roffset_regs *roffset_regs);


/**
 * Write Levelingの設定値, 調整結果, 詳細ログを格納する構造体.
 * この構造体に必要な設定値を入れて, dio_calib_wl()関数を呼び出すことでWrite Levelingを実行する.
 */
struct dio_wl{
    /** Write Levelingの調整結果がこの値より大きくなった場合は, 強制的に0に置き換えられます.
     これは, CKとDQSの比較は位相比較であり, Write Levelingの調整値が1周期分回ってしまうことがあることへの対策です. */
    int  valid_max[DQ_BLOCKS][2];
    /** Write Levelingのログが格納される配列. 後で詳細プロットを表示されるためのOK/NGの生データ. */
    u8 log[DQ_BLOCKS][128];
    /** Write Levelingの調整結果. レジスタにセットされます. */
    int  edge[DQ_BLOCKS][2];
};

/**
 * DQSEN adjustの設定値, 調整結果, 詳細ログを格納する構造体.
 * この構造体に必要な設定値を入れて, dio_calib_dqsen()関数を呼び出すことでDQSEN Adjustを実行する.
 */
struct dio_dqsen{
    /** 調整開始点. 粗調整(adj)での遅延量. */
    int range_min;
    /** 1箇所の検査に対し, 比較を何回繰り返すか. 回数が多いと精度は上がるが、時間がかかる. */
    int iteration;
    /** データ比較を使うか. trueならDQSのrising edgeの数とデータwrite/read/compareで判定し, DQSのrising edgeの数だけで判定します. */
    int use_data_compare;
    /** DQSEN adjustの調整結果のセンター値 (64 * adj + ctl). */
    int width[DQ_BLOCKS][2];
    /** DQSEN adjustの調整結果の幅 (64 * adj + ctl). */
    int center[DQ_BLOCKS][2];
};

/**
 * Toffset調整の設定値, 調整結果, 詳細ログを格納する構造体.
 * この構造体に必要な設定値を入れて, dio_calib_toffset()関数を呼び出すことでToffset調整を実行する.
 */
struct dio_toffset{
    /** 調整開始点. DQ, DQMの値. */
    int   range_min;
    /** 調整終了点. DQ, DQMの値. */
    int   range_max;
    /** 時短のため, 中央部分の検査をスキップするかどうかのフラグ. */
    int  skip_flag;
    /** この値からスキップする. skip_flag が falseなら無視される. */
    int   skip_min;
    /** この値までスキップする. skip_flag が falseなら無視される. */
    int   skip_max;
    /** Toffset調整に使用するデータ比較パターン. */
    const u32 *pattern_data;
    /** Toffset調整に使用するデータ比較パターン数. */
    int   pattern_length;
    /** 1箇所の検査に対し, 比較を何回繰り返すか. 回数が多いと精度は上がるが、時間がかかる. */
    int   iteration;
    /** Toffsetのログが格納される配列. 後で詳細プロットを表示されるためのDQのOK/NGの生データ. */
    u16 log[DQ_BLOCKS][128];
    /** Toffset調整結果が格納される配列. DQの幅. DQ_BLOCKSあたり16bit分. */
    int   width[DQ_BLOCKS][16];
    /** Toffset調整結果が格納される配列. DQのセンター値. DQ_BLOCKSあたり16bit分. */
    int   center[DQ_BLOCKS][16];
    /** Toffsetのログが格納される配列. 後で詳細プロットを表示されるためのDQMのOK/NGの生データ. */
    u16 mlog[DQ_BLOCKS][128];
    /** Toffset調整結果が格納される配列. DQMの幅. DQ_BLOCKSあたり2bit分. */
    int   mwidth[DQ_BLOCKS][2];
    /** Toffset調整結果が格納される配列. DQMのセンター値. DQ_BLOCKSあたり2bit分. */
    int   mcenter[DQ_BLOCKS][2];
};

/**
 * Roffset調整の設定値, 調整結果, 詳細ログを格納する構造体.
 * この構造体に必要な設定値を入れて, dio_calib_roffset()関数を呼び出すことでRoffset調整を実行する.
 */
struct dio_roffset{
    /** 調整開始点. DQを叩くDQSのSoC内部信号の遅延量. */
    int   range_min;
    /** 調整終了点. DQを叩くDQSのSoC内部信号の遅延量. */
    int   range_max;
    /** Roffset中にDQ信号をどれだけ遅らせるか. range_min, range_maxを小さい方向にシフトさせるのと同等の効果がある.
        調整開始点をマイナスから始めたい場合に主に使用する. */
    int   data_delay;
    /** 時短のため, 中央部分の検査をスキップするかどうかのフラグ. */
    int  skip_flag;
    /** この値からスキップする. skip_flag が falseなら無視される. */
    int   skip_min;
    /** この値までスキップする. skip_flag が falseなら無視される. */
    int   skip_max;
    /** Roffset調整に使用するデータ比較パターン. */
    const u32 *pattern_data;
    /** Roffset調整に使用するデータ比較パターン数. */
    int   pattern_length;
    /** 1箇所の検査に対し, 比較を何回繰り返すか. 回数が多いと精度は上がるが、時間がかかる. */
    int   iteration;
    /** Roffsetのログが格納される配列. 後で詳細プロットを表示されるためのOK/NGの生データ. */
    u32 log[DQ_BLOCKS][128];
    /** Roffset調整結果が格納される配列. 幅. DQ_BLOCKSあたりPostive Edge16bit + Neggative Edge16bitで32bit分. */
    int   width[DQ_BLOCKS][32];
    /** Roffset調整結果が格納される配列. センター値. DQ_BLOCKSあたりPostive Edge16bit + Neggative Edge16bitで32bit分. */
    int   center[DQ_BLOCKS][32];
    /** Roffset調整結果が格納される配列. 最小値. DQ_BLOCKSあたりPostive Edge16bit + Neggative Edge16bitで32bit分. */
    int   min[DQ_BLOCKS][32];
    /** Roffset調整結果が格納される配列. 最大値. DQ_BLOCKSあたりPostive Edge16bit + Neggative Edge16bitで32bit分. */
    int   max[DQ_BLOCKS][32];
};

/**
 * Write Levelingを実行する関数.
 * Write LevelingとはCKとDQSの位相差を解消する調整工程である.
 * @param wl Write Levelingの設定値, 調整結果, 詳細ログを格納する構造体へのポインタ
 * @return None
 */
void dio_calib_wl(struct dio_wl *wl);

/**
 * DQSEN Adjustを実行する関数.
 * DQSEN AdjustとはDDRメモリが返してくるDQS信号にかけるマスクの有効範囲を決める工程である.
 * @param dqsen DQSEN adjustの設定値, 調整結果, 詳細ログを格納する構造体へのポインタ
 * @return None
 */
void dio_calib_dqsen(struct dio_dqsen *dqsen);

/**
 * Toffset調整を実行する関数.
 * Toffsetとは, DDRメモリへWriteするときのDQSとDQラインの位相差のことである.
 * @param toffset Toffset調整の設定値, 調整結果, 詳細ログを格納する構造体へのポインタ
 * @return None
 */
void dio_calib_toffset(struct dio_toffset *toffset);

/**
 * Roffset調整を実行する関数.
 * Roffsetとは, DDRメモリからReadするときのDQSとDQラインの位相差のことである.
 * @param roffset Roffset調整の設定値, 調整結果, 詳細ログを格納する構造体へのポインタ
 * @return None
 */
void dio_calib_roffset(struct dio_roffset *roffset);

//#define DIO_BUS32H 3
//#define DIO_BUS32L 2
#define DIO_BUS32  2
#define DIO_BUS16  0

void  dio_datacpy(u32 addr, const u32 *data ,int length, int bus_mode);
u32 dio_datacmp(u32 addr, const u32 *data ,int length, int bus_mode);

/**
 * Toffset/Roffset調整に使用するデータ比較パターン.
 */
extern const u32 dio_pattern_data[];

/**
 * Toffset/Roffset調整に使用するデータ比較パターン数.
 * 1パターン = 16byte なので, バイト数でいうと (dio_pattern_length * 16)byteになる.
 */
extern const int   dio_pattern_length;

#endif /* ARCH_DIO_H */
