/* SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause */
/* Copyright(c) 2019-2022  Realtek Corporation
 */

#ifndef __RTW89_8852B_H__
#define __RTW89_8852B_H__

#include "core.h"

#define RF_PATH_NUM_8852B 2
#define BB_PATH_NUM_8852B 2

enum rtw8852b_pmac_mode {
	NONE_TEST,
	PKTS_TX,
	PKTS_RX,
	CONT_TX
};

struct rtw8852b_u_efuse {
	u8 rsvd[0x88];
	u8 mac_addr[ETH_ALEN];
};

struct rtw8852b_e_efuse {
	u8 mac_addr[ETH_ALEN];
};

struct rtw8852b_tssi_offset {
	u8 cck_tssi[TSSI_CCK_CH_GROUP_NUM];
	u8 bw40_tssi[TSSI_MCS_2G_CH_GROUP_NUM];
	u8 rsvd[7];
	u8 bw40_1s_tssi_5g[TSSI_MCS_5G_CH_GROUP_NUM];
} __packed;

struct rtw8852b_efuse {
	u8 rsvd[0x210];
	struct rtw8852b_tssi_offset path_a_tssi;
	u8 rsvd1[10];
	struct rtw8852b_tssi_offset path_b_tssi;
	u8 rsvd2[94];
	u8 channel_plan;
	u8 xtal_k;
	u8 rsvd3;
	u8 iqk_lck;
	u8 rsvd4[5];
	u8 reg_setting:2;
	u8 tx_diversity:1;
	u8 rx_diversity:2;
	u8 ac_mode:1;
	u8 module_type:2;
	u8 rsvd5;
	u8 shared_ant:1;
	u8 coex_type:3;
	u8 ant_iso:1;
	u8 radio_on_off:1;
	u8 rsvd6:2;
	u8 eeprom_version;
	u8 customer_id;
	u8 tx_bb_swing_2g;
	u8 tx_bb_swing_5g;
	u8 tx_cali_pwr_trk_mode;
	u8 trx_path_selection;
	u8 rfe_type;
	u8 country_code[2];
	u8 rsvd7[3];
	u8 path_a_therm;
	u8 path_b_therm;
	u8 rsvd8[2];
	u8 rx_gain_2g_ofdm;
	u8 rsvd9;
	u8 rx_gain_2g_cck;
	u8 rsvd10;
	u8 rx_gain_5g_low;
	u8 rsvd11;
	u8 rx_gain_5g_mid;
	u8 rsvd12;
	u8 rx_gain_5g_high;
	u8 rsvd13[35];
	u8 path_a_cck_pwr_idx[6];
	u8 path_a_bw40_1tx_pwr_idx[5];
	u8 path_a_ofdm_1tx_pwr_idx_diff:4;
	u8 path_a_bw20_1tx_pwr_idx_diff:4;
	u8 path_a_bw20_2tx_pwr_idx_diff:4;
	u8 path_a_bw40_2tx_pwr_idx_diff:4;
	u8 path_a_cck_2tx_pwr_idx_diff:4;
	u8 path_a_ofdm_2tx_pwr_idx_diff:4;
	u8 rsvd14[0xf2];
	union {
		struct rtw8852b_u_efuse u;
		struct rtw8852b_e_efuse e;
	};
} __packed;

static_assert(offsetof(struct rtw8852b_efuse, path_a_tssi) == 0x210);
static_assert(offsetof(struct rtw8852b_efuse, path_b_tssi) == 0x23a);
static_assert(offsetof(struct rtw8852b_efuse, channel_plan) == 0x2b8);
static_assert(offsetof(struct rtw8852b_efuse, rfe_type) == 0x2ca);
static_assert(offsetof(struct rtw8852b_efuse, country_code) == 0x2cb);
static_assert(offsetof(struct rtw8852b_efuse, path_a_therm) == 0x2d0);
static_assert(offsetof(struct rtw8852b_efuse, rx_gain_5g_high) == 0x2dc);
static_assert(offsetof(struct rtw8852b_efuse, e) == 0x400);
static_assert(offsetof(struct rtw8852b_efuse, u.mac_addr) == 0x488);

struct rtw8852b_bb_pmac_info {
	u8 en_pmac_tx:1;
	u8 is_cck:1;
	u8 mode:3;
	u8 rsvd:3;
	u16 tx_cnt;
	u16 period;
	u16 tx_time;
	u8 duty_cycle;
};

struct rtw8852b_bb_tssi_bak {
	u8 tx_path;
	u8 rx_path;
	u32 p0_rfmode;
	u32 p0_rfmode_ftm;
	u32 p1_rfmode;
	u32 p1_rfmode_ftm;
	s16 tx_pwr; /* S9 */
};

extern const struct rtw89_chip_info rtw8852b_chip_info;

void rtw8852b_bb_set_plcp_tx(struct rtw89_dev *rtwdev);
void rtw8852b_bb_set_pmac_tx(struct rtw89_dev *rtwdev,
			     struct rtw8852b_bb_pmac_info *tx_info,
			     enum rtw89_phy_idx idx);
void rtw8852b_bb_set_pmac_pkt_tx(struct rtw89_dev *rtwdev, u8 enable,
				 u16 tx_cnt, u16 period, u16 tx_time,
				 enum rtw89_phy_idx idx);
void rtw8852b_bb_set_power(struct rtw89_dev *rtwdev, s16 pwr_dbm,
			   enum rtw89_phy_idx idx);
void rtw8852b_bb_cfg_tx_path(struct rtw89_dev *rtwdev, u8 tx_path);
void rtw8852b_bb_ctrl_rx_path(struct rtw89_dev *rtwdev,
			      enum rtw89_rf_path_bit rx_path);
void rtw8852b_bb_tx_mode_switch(struct rtw89_dev *rtwdev,
				enum rtw89_phy_idx idx, u8 mode);
void rtw8852b_bb_backup_tssi(struct rtw89_dev *rtwdev, enum rtw89_phy_idx idx,
			     struct rtw8852b_bb_tssi_bak *bak);
void rtw8852b_bb_restore_tssi(struct rtw89_dev *rtwdev, enum rtw89_phy_idx idx,
			      const struct rtw8852b_bb_tssi_bak *bak);

#endif
