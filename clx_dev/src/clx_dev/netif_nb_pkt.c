/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  hal_nb_pkt_knl.c
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 *
 */

/*****************************************************************************
 * INCLUDE FILE DECLARATIONS
 *****************************************************************************
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/if.h>
#include <uapi/linux/ethtool.h>
#include <linux/ethtool.h>
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

/* netif */
#include <netif_osal.h>
#include <netif_perf.h>
#include <netif_nl.h>

#include <osal/netif_nb_pkt.h>
#include <osal/netif_common.h>

/* clx_sdk */
#include <hal/common/hal_dflt.h>



extern HAL_PKT_DRV_CB_T                         _hal_pkt_drv_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];

#define HAL_NB_PKT_GET_DRV_CB_PTR(unit)         (&_hal_pkt_drv_cb[unit])


static CLX_ERROR_NO_T
_hal_nb_pkt_schedRxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_nb_pkt_strictTxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_initTask(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
hal_nb_pkt_deinitTask(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_rxStop(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_nb_pkt_rxStart(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_deinit_pkt_drv(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_init_irq(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static netdev_tx_t
_hal_nb_pkt_net_dev_tx(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev)
{
    return (0);
}

static ssize_t
_hal_nb_pkt_dev_tx(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    return (0);
}
static CLX_ERROR_NO_T
hal_nb_pkt_lockRxChannelAll(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_unlockRxChannelAll(
    const UI32_T                    unit)
{
    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
hal_nb_pkt_getTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_getRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_clearTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_nb_pkt_clearRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}


CLX_ERROR_NO_T
hal_nb_register_netif_ioctl(void)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* driver */
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_RX_FREE,
        _hal_nb_pkt_schedRxDeQueue);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE,
        _hal_nb_pkt_strictTxDeQueue);

    /* counter */
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
        hal_nb_pkt_getTxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
        hal_nb_pkt_getRxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
        hal_nb_pkt_clearTxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
        hal_nb_pkt_clearRxKnlCnt);

    return rc;
}


void hal_nb_register_drv_cb(
    const UI32_T unit)
{
    HAL_PKT_DRV_CB_T            *ptr_cb = HAL_NB_PKT_GET_DRV_CB_PTR(unit);

    // osal_memset(_hal_nb_pkt_tx_cb, 0x0,
    //             CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_NB_PKT_TX_CB_T));
    // osal_memset(_hal_nb_pkt_rx_cb, 0x0,
    //             CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_NB_PKT_RX_CB_T));

    ptr_cb->init_task = hal_nb_pkt_initTask;
    ptr_cb->deinit_task = hal_nb_pkt_deinitTask;
    ptr_cb->pkt_rx_stop = hal_nb_pkt_rxStop;
    ptr_cb->pkt_rx_start = _hal_nb_pkt_rxStart;
    ptr_cb->pkt_deinit_drv = hal_nb_pkt_deinit_pkt_drv;
    ptr_cb->pkt_init_drv = hal_nb_init_irq;

    ptr_cb->net_dev_tx = _hal_nb_pkt_net_dev_tx;
    ptr_cb->pkt_dev_tx = _hal_nb_pkt_dev_tx;
    ptr_cb->lock_all_rx_channel = hal_nb_pkt_lockRxChannelAll;
    ptr_cb->unlock_all_rx_channel = hal_nb_pkt_unlockRxChannelAll;

    hal_nb_register_netif_ioctl();
}