// SPDX-License-Identifier: ISC
/* Copyright (C) 2020 MediaTek Inc. */

#include "mt7921.h"
#include "../dma.h"
#include "mac.h"

static int mt7921_init_tx_queues(struct mt7921_phy *phy, int idx, int n_desc)
{
	int i, err;

	err = mt76_init_tx_queue(phy->mt76, 0, idx, n_desc, MT_TX_RING_BASE);
	if (err < 0)
		return err;

	for (i = 0; i <= MT_TXQ_PSD; i++)
		phy->mt76->q_tx[i] = phy->mt76->q_tx[0];

	return 0;
}

static int mt7921_poll_tx(struct napi_struct *napi, int budget)
{
	struct mt7921_dev *dev;

	dev = container_of(napi, struct mt7921_dev, mt76.tx_napi);

	if (!mt76_connac_pm_ref(&dev->mphy, &dev->pm)) {
		napi_complete(napi);
		queue_work(dev->mt76.wq, &dev->pm.wake_work);
		return 0;
	}

	mt7921_mcu_tx_cleanup(dev);
	if (napi_complete(napi))
		mt7921_irq_enable(dev, MT_INT_TX_DONE_ALL);
	mt76_connac_pm_unref(&dev->mphy, &dev->pm);

	return 0;
}

static int mt7921_poll_rx(struct napi_struct *napi, int budget)
{
	struct mt7921_dev *dev;
	int done;

	dev = container_of(napi->dev, struct mt7921_dev, mt76.napi_dev);

	if (!mt76_connac_pm_ref(&dev->mphy, &dev->pm)) {
		napi_complete(napi);
		queue_work(dev->mt76.wq, &dev->pm.wake_work);
		return 0;
	}
	done = mt76_dma_rx_poll(napi, budget);
	mt76_connac_pm_unref(&dev->mphy, &dev->pm);

	return done;
}

static void mt7921_dma_prefetch(struct mt7921_dev *dev)
{
#define PREFETCH(base, depth)	((base) << 16 | (depth))

	mt76_wr(dev, MT_WFDMA0_RX_RING0_EXT_CTRL, PREFETCH(0x0, 0x4));
	mt76_wr(dev, MT_WFDMA0_RX_RING2_EXT_CTRL, PREFETCH(0x40, 0x4));
	mt76_wr(dev, MT_WFDMA0_RX_RING3_EXT_CTRL, PREFETCH(0x80, 0x4));
	mt76_wr(dev, MT_WFDMA0_RX_RING4_EXT_CTRL, PREFETCH(0xc0, 0x4));
	mt76_wr(dev, MT_WFDMA0_RX_RING5_EXT_CTRL, PREFETCH(0x100, 0x4));

	mt76_wr(dev, MT_WFDMA0_TX_RING0_EXT_CTRL, PREFETCH(0x140, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING1_EXT_CTRL, PREFETCH(0x180, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING2_EXT_CTRL, PREFETCH(0x1c0, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING3_EXT_CTRL, PREFETCH(0x200, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING4_EXT_CTRL, PREFETCH(0x240, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING5_EXT_CTRL, PREFETCH(0x280, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING6_EXT_CTRL, PREFETCH(0x2c0, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING16_EXT_CTRL, PREFETCH(0x340, 0x4));
	mt76_wr(dev, MT_WFDMA0_TX_RING17_EXT_CTRL, PREFETCH(0x380, 0x4));
}

static int mt7921_dma_disable(struct mt7921_dev *dev, bool force)
{
// https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/drivers/net/wireless/mediatek/mt76/mt7921?h=v6.4-rc5&id=87714bf6ed1589813e473db5471e6e9857755764
// FLAGSP:87714bf6
// wifi: mt76: mt7921e: improve reliability of dma reset
// The hardware team has advised the driver that it is necessary to first put
// WFDMA into an idle state before resetting the WFDMA. Otherwise, the WFDMA
// may enter an unknown state where it cannot be polled with the right state
// successfully. To ensure that the DMA can work properly while a stressful
// cold reboot test was being made, we have reordered the programming sequence
// in the driver based on the hardware team's guidance.

// The patch would modify the WFDMA disabling flow from

// "DMA reset -> disabling DMASHDL -> disabling WFDMA -> polling and waiting
// until DMA idle" to "disabling WFDMA -> polling and waiting for DMA idle ->
// disabling DMASHDL -> DMA reset.

// Where he polling and waiting until WFDMA is idle is coordinated with the
// operation of disabling WFDMA. Even while WFDMA is being disabled, it can
// still handle Tx/Rx requests. The additional polling allows sufficient time
// for WFDMA to process the last T/Rx request. When the idle state of WFDMA is
// reached, it is a reliable indication that DMASHDL is also idle to ensure it
// is safe to disable it and perform the DMA reset.

// Fixes: 0a1059d0f060 ("mt76: mt7921: move mt7921_dma_reset in dma.c")
// Co-developed-by: Sean Wang <sean.wang@mediatek.com>
// Signed-off-by: Sean Wang <sean.wang@mediatek.com>
// Co-developed-by: Deren Wu <deren.wu@mediatek.com>
// Signed-off-by: Deren Wu <deren.wu@mediatek.com>
// Co-developed-by: Wang Zhao <wang.zhao@mediatek.com>
// Signed-off-by: Wang Zhao <wang.zhao@mediatek.com>
// Signed-off-by: Quan Zhou <quan.zhou@mediatek.com>
// Signed-off-by: Felix Fietkau <nbd@nbd.name>
// Signed-off-by: Guan Wentao <guanwentao@uniontech.com>

	// if (force) {// FLAGSP:87714bf6
	// 	/* reset */// FLAGSP:87714bf6
	// 	mt76_clear(dev, MT_WFDMA0_RST,// FLAGSP:87714bf6
	// 		   MT_WFDMA0_RST_DMASHDL_ALL_RST |// FLAGSP:87714bf6
	// 		   MT_WFDMA0_RST_LOGIC_RST);// FLAGSP:87714bf6

	// 	mt76_set(dev, MT_WFDMA0_RST,// FLAGSP:87714bf6
	// 		 MT_WFDMA0_RST_DMASHDL_ALL_RST |// FLAGSP:87714bf6
	// 		 MT_WFDMA0_RST_LOGIC_RST);// FLAGSP:87714bf6
	// }// FLAGSP:87714bf6

	// /* disable dmashdl */// FLAGSP:87714bf6
	// mt76_clear(dev, MT_WFDMA0_GLO_CFG_EXT0,// FLAGSP:87714bf6
	// 	   MT_WFDMA0_CSR_TX_DMASHDL_ENABLE);// FLAGSP:87714bf6
	// mt76_set(dev, MT_DMASHDL_SW_CONTROL, MT_DMASHDL_DMASHDL_BYPASS);// FLAGSP:87714bf6

	/* disable WFDMA0 */
	mt76_clear(dev, MT_WFDMA0_GLO_CFG,
		   MT_WFDMA0_GLO_CFG_TX_DMA_EN | MT_WFDMA0_GLO_CFG_RX_DMA_EN |
		   MT_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN |
		   MT_WFDMA0_GLO_CFG_OMIT_TX_INFO |
		   MT_WFDMA0_GLO_CFG_OMIT_RX_INFO |
		   MT_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2);

// https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/drivers/net/wireless/mediatek?h=v6.4-rc5&id=c397fc1e6365a2a9e5540a85b2c1d4ea412aa0e2
// FLAGSP:c397fc1e
// wifi: mt76: mt7921e: fix probe timeout after reboot
// In system warm reboot scene, due to the polling timeout(now 1000us)
// is too short to wait dma idle in time, it may make driver probe fail
// with error code -ETIMEDOUT. Meanwhile, we also found the dma may take
// around 70ms to enter idle state. Change the polling idle timeout to
// 100ms to avoid the probabilistic probe fail.

// Tested pass with 5000 times warm reboot on x86 platform.

// [4.477496] pci 0000:01:00.0: attach allowed to drvr mt7921e [internal device]
// [4.478306] mt7921e 0000:01:00.0: ASIC revision: 79610010
// [4.480063] mt7921e: probe of 0000:01:00.0 failed with error -110

// Fixes: 0a1059d0f060 ("mt76: mt7921: move mt7921_dma_reset in dma.c")
// Signed-off-by: Quan Zhou <quan.zhou@mediatek.com>
// Signed-off-by: Deren Wu <deren.wu@mediatek.com>
// Signed-off-by: Felix Fietkau <nbd@nbd.name>
// Signed-off-by: Guan Wentao <guanwentao@uniontech.com>
// if (!mt76_poll(dev, MT_WFDMA0_GLO_CFG,
// 	       MT_WFDMA0_GLO_CFG_TX_DMA_BUSY |
// 	       MT_WFDMA0_GLO_CFG_RX_DMA_BUSY, 0, 1000))
	if (!mt76_poll_msec(dev, MT_WFDMA0_GLO_CFG,// FLAGSP:c397fc1e
		       MT_WFDMA0_GLO_CFG_TX_DMA_BUSY |// FLAGSP:c397fc1e
		       MT_WFDMA0_GLO_CFG_RX_DMA_BUSY, 0, 100))// FLAGSP:c397fc1e
		return -ETIMEDOUT;
	/* disable dmashdl */// FLAGSP:87714bf6
	mt76_clear(dev, MT_WFDMA0_GLO_CFG_EXT0,// FLAGSP:87714bf6
		   MT_WFDMA0_CSR_TX_DMASHDL_ENABLE);// FLAGSP:87714bf6
	mt76_set(dev, MT_DMASHDL_SW_CONTROL, MT_DMASHDL_DMASHDL_BYPASS);// FLAGSP:87714bf6

	if (force) {// FLAGSP:87714bf6
		/* reset */// FLAGSP:87714bf6
		mt76_clear(dev, MT_WFDMA0_RST,// FLAGSP:87714bf6
			   MT_WFDMA0_RST_DMASHDL_ALL_RST |// FLAGSP:87714bf6
			   MT_WFDMA0_RST_LOGIC_RST);// FLAGSP:87714bf6

		mt76_set(dev, MT_WFDMA0_RST,// FLAGSP:87714bf6
			 MT_WFDMA0_RST_DMASHDL_ALL_RST |// FLAGSP:87714bf6
			 MT_WFDMA0_RST_LOGIC_RST);// FLAGSP:87714bf6
	}// FLAGSP:87714bf6

	return 0;
}

static int mt7921_dma_enable(struct mt7921_dev *dev)
{
	/* configure perfetch settings */
	mt7921_dma_prefetch(dev);

	/* reset dma idx */
	mt76_wr(dev, MT_WFDMA0_RST_DTX_PTR, ~0);

	/* configure delay interrupt */
	mt76_wr(dev, MT_WFDMA0_PRI_DLY_INT_CFG0, 0);

	mt76_set(dev, MT_WFDMA0_GLO_CFG,
		 MT_WFDMA0_GLO_CFG_TX_WB_DDONE |
		 MT_WFDMA0_GLO_CFG_FIFO_LITTLE_ENDIAN |
		 MT_WFDMA0_GLO_CFG_CLK_GAT_DIS |
		 MT_WFDMA0_GLO_CFG_OMIT_TX_INFO |
		 MT_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN |
		 MT_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2);

	mt76_set(dev, MT_WFDMA0_GLO_CFG,
		 MT_WFDMA0_GLO_CFG_TX_DMA_EN | MT_WFDMA0_GLO_CFG_RX_DMA_EN);

	mt76_set(dev, MT_WFDMA_DUMMY_CR, MT_WFDMA_NEED_REINIT);

	/* enable interrupts for TX/RX rings */
	mt7921_irq_enable(dev,
			  MT_INT_RX_DONE_ALL | MT_INT_TX_DONE_ALL |
			  MT_INT_MCU_CMD);
	mt76_set(dev, MT_MCU2HOST_SW_INT_ENA, MT_MCU_CMD_WAKE_RX_PCIE);

	return 0;
}

static int mt7921_dma_reset(struct mt7921_dev *dev, bool force)
{
	int i, err;

	err = mt7921_dma_disable(dev, force);
	if (err)
		return err;

	/* reset hw queues */
	for (i = 0; i < __MT_TXQ_MAX; i++)
		mt76_queue_reset(dev, dev->mphy.q_tx[i]);

	for (i = 0; i < __MT_MCUQ_MAX; i++)
		mt76_queue_reset(dev, dev->mt76.q_mcu[i]);

	mt76_for_each_q_rx(&dev->mt76, i)
		mt76_queue_reset(dev, &dev->mt76.q_rx[i]);

	mt76_tx_status_check(&dev->mt76, true);

	return mt7921_dma_enable(dev);
}

int mt7921_wfsys_reset(struct mt7921_dev *dev)
{
	mt76_clear(dev, MT_WFSYS_SW_RST_B, WFSYS_SW_RST_B);
	msleep(50);
	mt76_set(dev, MT_WFSYS_SW_RST_B, WFSYS_SW_RST_B);

	if (!__mt76_poll_msec(&dev->mt76, MT_WFSYS_SW_RST_B,
			      WFSYS_SW_INIT_DONE, WFSYS_SW_INIT_DONE, 500))
		return -ETIMEDOUT;

	return 0;
}

int mt7921_wpdma_reset(struct mt7921_dev *dev, bool force)
{
	int i, err;

	/* clean up hw queues */
	for (i = 0; i < ARRAY_SIZE(dev->mt76.phy.q_tx); i++)
		mt76_queue_tx_cleanup(dev, dev->mphy.q_tx[i], true);

	for (i = 0; i < ARRAY_SIZE(dev->mt76.q_mcu); i++)
		mt76_queue_tx_cleanup(dev, dev->mt76.q_mcu[i], true);

	mt76_for_each_q_rx(&dev->mt76, i)
		mt76_queue_rx_cleanup(dev, &dev->mt76.q_rx[i]);

	if (force) {
		err = mt7921_wfsys_reset(dev);
		if (err)
			return err;
	}
	err = mt7921_dma_reset(dev, force);
	if (err)
		return err;

	mt76_for_each_q_rx(&dev->mt76, i)
		mt76_queue_rx_reset(dev, i);

	return 0;
}

int mt7921_wpdma_reinit_cond(struct mt7921_dev *dev)
{
	struct mt76_connac_pm *pm = &dev->pm;
	int err;

	/* check if the wpdma must be reinitialized */
	if (mt7921_dma_need_reinit(dev)) {
		/* disable interrutpts */
		mt76_wr(dev, MT_WFDMA0_HOST_INT_ENA, 0);
		mt76_wr(dev, MT_PCIE_MAC_INT_ENABLE, 0x0);

		err = mt7921_wpdma_reset(dev, false);
		if (err) {
			dev_err(dev->mt76.dev, "wpdma reset failed\n");
			return err;
		}

		/* enable interrutpts */
		mt76_wr(dev, MT_PCIE_MAC_INT_ENABLE, 0xff);
		pm->stats.lp_wake++;
	}

	return 0;
}

int mt7921_dma_init(struct mt7921_dev *dev)
{
	int ret;

	mt76_dma_attach(&dev->mt76);

	ret = mt7921_dma_disable(dev, true);
	if (ret)
		return ret;
// FLAGSP：525c469e5de9
// wifi: mt76: mt7921e: fix init command fail with enabled device
// For some cases as below, we may encounter the unpreditable chip stats
// in driver probe()
// * The system reboot flow do not work properly, such as kernel oops while
//   rebooting, and then the driver do not go back to default status at
//   this moment.
// * Similar to the flow above. If the device was enabled in BIOS or UEFI,
//   the system may switch to Linux without driver fully shutdown.

// To avoid the problem, force push the device back to default in probe()
// * mt7921e_mcu_fw_pmctrl() : return control privilege to chip side.
// * mt7921_wfsys_reset()    : cleanup chip config before resource init.

// Error log
// [59007.600714] mt7921e 0000:02:00.0: ASIC revision: 79220010
// [59010.889773] mt7921e 0000:02:00.0: Message 00000010 (seq 1) timeout
// [59010.889786] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59014.217839] mt7921e 0000:02:00.0: Message 00000010 (seq 2) timeout
// [59014.217852] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59017.545880] mt7921e 0000:02:00.0: Message 00000010 (seq 3) timeout
// [59017.545893] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59020.874086] mt7921e 0000:02:00.0: Message 00000010 (seq 4) timeout
// [59020.874099] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59024.202019] mt7921e 0000:02:00.0: Message 00000010 (seq 5) timeout
// [59024.202033] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59027.530082] mt7921e 0000:02:00.0: Message 00000010 (seq 6) timeout
// [59027.530096] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59030.857888] mt7921e 0000:02:00.0: Message 00000010 (seq 7) timeout
// [59030.857904] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59034.185946] mt7921e 0000:02:00.0: Message 00000010 (seq 8) timeout
// [59034.185961] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59037.514249] mt7921e 0000:02:00.0: Message 00000010 (seq 9) timeout
// [59037.514262] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59040.842362] mt7921e 0000:02:00.0: Message 00000010 (seq 10) timeout
// [59040.842375] mt7921e 0000:02:00.0: Failed to get patch semaphore
// [59040.923845] mt7921e 0000:02:00.0: hardware init failed

// Cc: stable@vger.kernel.org
// Fixes: 5c14a5f944b9 ("mt76: mt7921: introduce mt7921e support")
// Tested-by: Kai-Heng Feng <kai.heng.feng@canonical.com>
// Tested-by: Juan Martinez <juan.martinez@amd.com>
// Co-developed-by: Leon Yen <leon.yen@mediatek.com>
// Signed-off-by: Leon Yen <leon.yen@mediatek.com>
// Signed-off-by: Quan Zhou <quan.zhou@mediatek.com>
// Signed-off-by: Deren Wu <deren.wu@mediatek.com>
// Message-ID: <39fcb7cee08d4ab940d38d82f21897483212483f.1688569385.git.deren.wu@mediatek.com>
// Signed-off-by: Jakub Kicinski <kuba@kernel.org>

	// ret = mt7921_wfsys_reset(dev);// FLAGSP：525c469e5de9
	// if (ret)// FLAGSP：525c469e5de9
	// 	return ret;// FLAGSP：525c469e5de9

	/* init tx queue */
	ret = mt7921_init_tx_queues(&dev->phy, MT7921_TXQ_BAND0,
				    MT7921_TX_RING_SIZE);
	if (ret)
		return ret;

	mt76_wr(dev, MT_WFDMA0_TX_RING0_EXT_CTRL, 0x4);

	/* command to WM */
	ret = mt76_init_mcu_queue(&dev->mt76, MT_MCUQ_WM, MT7921_TXQ_MCU_WM,
				  MT7921_TX_MCU_RING_SIZE, MT_TX_RING_BASE);
	if (ret)
		return ret;

	/* firmware download */
	ret = mt76_init_mcu_queue(&dev->mt76, MT_MCUQ_FWDL, MT7921_TXQ_FWDL,
				  MT7921_TX_FWDL_RING_SIZE, MT_TX_RING_BASE);
	if (ret)
		return ret;

	/* event from WM before firmware download */
	ret = mt76_queue_alloc(dev, &dev->mt76.q_rx[MT_RXQ_MCU],
			       MT7921_RXQ_MCU_WM,
			       MT7921_RX_MCU_RING_SIZE,
			       MT_RX_BUF_SIZE, MT_RX_EVENT_RING_BASE);
	if (ret)
		return ret;

	/* Change mcu queue after firmware download */
	ret = mt76_queue_alloc(dev, &dev->mt76.q_rx[MT_RXQ_MCU_WA],
			       MT7921_RXQ_MCU_WM,
			       MT7921_RX_MCU_RING_SIZE,
			       MT_RX_BUF_SIZE, MT_WFDMA0(0x540));
	if (ret)
		return ret;

	/* rx data */
	ret = mt76_queue_alloc(dev, &dev->mt76.q_rx[MT_RXQ_MAIN],
			       MT7921_RXQ_BAND0, MT7921_RX_RING_SIZE,
			       MT_RX_BUF_SIZE, MT_RX_DATA_RING_BASE);
	if (ret)
		return ret;

	ret = mt76_init_queues(dev, mt7921_poll_rx);
	if (ret < 0)
		return ret;

	netif_tx_napi_add(&dev->mt76.tx_napi_dev, &dev->mt76.tx_napi,
			  mt7921_poll_tx, NAPI_POLL_WEIGHT);
	napi_enable(&dev->mt76.tx_napi);

	return mt7921_dma_enable(dev);
}

void mt7921_dma_cleanup(struct mt7921_dev *dev)
{
	/* disable */
	mt76_clear(dev, MT_WFDMA0_GLO_CFG,
		   MT_WFDMA0_GLO_CFG_TX_DMA_EN |
		   MT_WFDMA0_GLO_CFG_RX_DMA_EN |
		   MT_WFDMA0_GLO_CFG_CSR_DISP_BASE_PTR_CHAIN_EN |
		   MT_WFDMA0_GLO_CFG_OMIT_TX_INFO |
		   MT_WFDMA0_GLO_CFG_OMIT_RX_INFO |
		   MT_WFDMA0_GLO_CFG_OMIT_RX_INFO_PFET2);

	dev_info(dev->mt76.dev,
			 "Debug:Enter %s func wait\n",__func__);// FLAGSP:87714bf6
	mt76_poll_msec(dev, MT_WFDMA0_GLO_CFG,// FLAGSP:87714bf6
		       MT_WFDMA0_GLO_CFG_TX_DMA_BUSY |// FLAGSP:87714bf6
		       MT_WFDMA0_GLO_CFG_RX_DMA_BUSY, 0, 100);// FLAGSP:87714bf6
	dev_info(dev->mt76.dev,
			 "Debug:Out %s func wait\n",__func__);// FLAGSP:87714bf6

	/* reset */
	mt76_clear(dev, MT_WFDMA0_RST,
		   MT_WFDMA0_RST_DMASHDL_ALL_RST |
		   MT_WFDMA0_RST_LOGIC_RST);

	mt76_set(dev, MT_WFDMA0_RST,
		 MT_WFDMA0_RST_DMASHDL_ALL_RST |
		 MT_WFDMA0_RST_LOGIC_RST);

	mt76_dma_cleanup(&dev->mt76);
}
