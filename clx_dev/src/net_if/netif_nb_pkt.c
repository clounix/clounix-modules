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
#include <netif/netif_osal.h>
#include <netif/netif_perf.h>
#include <netif/netif_nl.h>

#include <netif/netif_nb_pkt.h>
#include <netif/netif_common.h>

/* clx_sdk */
#include <hal/common/hal_dflt.h>



extern HAL_PKT_DRV_CB_T                                 _hal_pkt_drv_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_NB_PKT_TX_CB_T                               _hal_nb_pkt_tx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_NB_PKT_RX_CB_T                               _hal_nb_pkt_rx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];

extern HAL_PKT_NETIF_PORT_DB_T                          _hal_pkt_port_db[HAL_PKT_MAX_PORT_NUM];
static CLX_THREAD_ID_T                                  err_task_id;


#define HAL_NB_PKT_GET_DRV_CB_PTR(unit)                (&_hal_pkt_drv_cb[unit])
/*---------------------------------------------------------------------------*/
#define HAL_NB_PKT_GET_TX_CB_PTR(unit)                 (&_hal_nb_pkt_tx_cb[unit])
#define HAL_NB_PKT_GET_TX_PDMA_PTR(unit, channel)      (&_hal_nb_pkt_tx_cb[unit].pdma[channel])
#define HAL_NB_PKT_GET_TX_GPD_PTR(unit, channel, gpd)  (&_hal_nb_pkt_tx_cb[unit].pdma[channel].ring_base_align[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_NB_PKT_GET_RX_CB_PTR(unit)                 (&_hal_nb_pkt_rx_cb[unit])
#define HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel)      (&_hal_nb_pkt_rx_cb[unit].pdma[channel])
#define HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, gpd)  (&_hal_nb_pkt_rx_cb[unit].pdma[channel].ring_base_align[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_NB_PKT_GET_PORT_DB(port)                   (&_hal_pkt_port_db[port])
#define HAL_NB_PKT_GET_PORT_PROFILE_LIST(port)         (_hal_pkt_port_db[port].ptr_profile_list)
#define HAL_NB_PKT_GET_PORT_NETDEV(port)               _hal_pkt_port_db[port].ptr_net_dev


/* Sleep Time Definitions */
#define HAL_NB_PKT_TX_DEQUE_SLEEP()                     osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_RX_DEQUE_SLEEP()                     osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_TX_ENQUE_RETRY_SLEEP()               osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_RX_ENQUE_RETRY_SLEEP()               osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_ALLOC_MEM_RETRY_SLEEP()              osal_sleepThread(1000) /* us */


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
_hal_nb_enable_pdma_rx_channel(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_SET_BIT(enable,(1 << channel));
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_nb_disable_pdma_rx_channel(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_CLR_BIT(enable,(1 << channel));
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_nb_enable_pdma_tx_channel(
    const UI32_T                    unit,
    const HAL_NB_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_SET_BIT(enable,(1 << (channel + 4)));
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_nb_disable_pdma_tx_channel(
    const UI32_T                    unit,
    const HAL_NB_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_CLR_BIT(enable,(1 << (channel + 4)));
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pdma_set_rx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    work_idx)
{
    osal_mdc_writePciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &work_idx, sizeof(UI32_T));
    return CLX_E_OK;
}
static UI32_T
_hal_nb_pdma_get_rx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          work_idx;
    osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &work_idx, sizeof(UI32_T));
    return work_idx;
}
// static UI32_T
// _hal_nb_pdma_get_rx_ch_pop_idx(
//     const UI32_T                    unit,
//     const HAL_NB_PDMA_RX_CHANNEL_T  channel)
// {
//     UI32_T                          pop_idx;
//     osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &pop_idx, sizeof(UI32_T));
//     return pop_idx;
// }

// static CLX_ERROR_NO_T
// _hal_nb_pdma_set_tx_ch_work_idx(
//     const UI32_T                    unit,
//     const HAL_NB_PDMA_TX_CHANNEL_T  channel,
//     const UI32_T                    work_idx)
// {
//     osal_mdc_writePciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &work_idx, sizeof(UI32_T));
//     return CLX_E_OK;
// }
// static UI32_T
// _hal_nb_pdma_get_tx_ch_work_idx(
//     const UI32_T                    unit,
//     const HAL_NB_PDMA_TX_CHANNEL_T  channel)
// {
//     UI32_T                          work_idx;
//     osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &work_idx, sizeof(UI32_T));
//     return work_idx;
// }
// static UI32_T
// _hal_nb_pdma_get_tx_ch_pop_idx(
//     const UI32_T                    unit,
//     const HAL_NB_PDMA_TX_CHANNEL_T  channel)
// {
//     UI32_T                          pop_idx;
//     osal_mdc_readPciReg(unit, HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &pop_idx, sizeof(UI32_T));
//     return pop_idx;
// }


static CLX_ERROR_NO_T
_hal_nb_pkt_alloc_rx_payload_buf(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    gpd_idx)
{
    CLX_ERROR_NO_T                  rc = CLX_E_NO_MEMORY;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_NB_PKT_GET_DRV_CB_PTR(unit);
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;

    void                            *ring_buf = NULL;

    ring_buf = osal_dma_alloc(ptr_cb->buf_len + sizeof(HAL_NB_PP_HDR_T));
    phy_addr = osal_dma_convertVirtToPhy(ring_buf);

    if (CLX_E_OK == rc)
    {
        ptr_desc->s_addr_hi = CLX_ADDR_64_HI(phy_addr);
        ptr_desc->s_addr_lo = CLX_ADDR_64_LOW(phy_addr);
        ptr_desc->size      = ptr_cb->buf_len;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_rx_pdma_buf(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_NB_PKT_GET_DRV_CB_PTR(unit);
    HAL_NB_PKT_RX_CB_T              *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = NULL;
    UI32_T                          gpd_idx = 0;

    if (0 == ptr_cb->buf_len)
    {
        return (CLX_E_BAD_PARAMETER);
    }

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_dma_invalidateCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));

        rc = _hal_nb_pkt_alloc_rx_payload_buf(unit, channel, gpd_idx);
        if (CLX_E_OK == rc)
        {
            ptr_desc->interrupt = 0;
            ptr_desc->err = 0;
            ptr_desc->eop = 0;
            osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));
        }
        else
        {
            ptr_rx_cb->cnt.no_memory++;
            break;
        }
    }

    return (rc);
}


static CLX_ERROR_NO_T
_hal_nb_pkt_free_rx_payload_buf(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    gpd_idx)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OTHERS;
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;
    void                            *ptr_virt_addr;

    phy_addr = CLX_ADDR_32_TO_64(ptr_desc->d_addr_hi, ptr_desc->d_addr_lo);
    if (0x0 != phy_addr)
    {
        ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
        osal_dma_free(ptr_virt_addr);
        rc = CLX_E_OK;
    }

    if (CLX_E_OK == rc)
    {
        ptr_desc->d_addr_hi = 0x0;
        ptr_desc->d_addr_lo = 0x0;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_deinit_rx_pdma_ring_buf(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; ((gpd_idx < ptr_rx_pdma->ring_size) && (CLX_E_OK == rc)); gpd_idx++)
    {
        /* we shuold process all descriptors in Rx-done task before free */
        // TODO
        rc = _hal_nb_pkt_free_rx_payload_buf(unit, channel, gpd_idx);
    }
    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_free_rx_payload_buf_desc(
    const UI32_T                    unit,
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_desc)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OTHERS;
    CLX_ADDR_T                      phy_addr = 0;
    void                            *ptr_virt_addr;

    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_desc->desc.d_addr_hi, ptr_sw_desc->desc.d_addr_lo);
    if (0x0 != phy_addr)
    {
        ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
        osal_dma_free(ptr_virt_addr);
        rc = CLX_E_OK;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_freeRxGpdList(
    UI32_T                          unit,
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_desc,
    BOOL_T                          free_payload)
{
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_desc_cur = NULL;

    while (NULL != ptr_sw_desc)
    {
        ptr_sw_desc_cur = ptr_sw_desc;
        ptr_sw_desc = ptr_sw_desc->ptr_next;
        if (TRUE == free_payload)
        {
            _hal_nb_pkt_free_rx_payload_buf_desc(unit, ptr_sw_desc_cur);
        }
        osal_free(ptr_sw_desc_cur);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_enqueue(
    HAL_PKT_SW_QUEUE_T  *ptr_que,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_enque(&ptr_que->que_id, ptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_dequeue(
    HAL_PKT_SW_QUEUE_T  *ptr_que,
    void                    **pptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_deque(&ptr_que->que_id, pptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_flushRxQueue(
    const UI32_T                unit,
    HAL_PKT_SW_QUEUE_T      *ptr_que)
{
    HAL_NB_PKT_RX_SW_DESC_T     *ptr_sw_desc_knl = NULL;
    CLX_ERROR_NO_T              rc;

    while (1)
    {
        rc = _hal_nb_pkt_dequeue(ptr_que, (void **)&ptr_sw_desc_knl);
        if (CLX_E_OK == rc)
        {
            _hal_nb_pkt_freeRxGpdList(unit, ptr_sw_desc_knl, TRUE);
        }
        else
        {
            break;
        }
    }

    return (CLX_E_OK);
}

static void
_hal_nb_pkt_handleErrorTask(
    void                    *ptr_argv)
{
}
static void
_hal_nb_pkt_handleTxDoneTask(
    void                    *ptr_argv)
{
}


typedef enum
{
    HAL_NB_PKT_DEST_NETDEV = 0,
    HAL_NB_PKT_DEST_SDK,
    HAL_NB_PKT_DEST_NETLINK,
    HAL_NB_PKT_DEST_DROP,
    HAL_NB_PKT_DEST_LAST
} HAL_NB_PKT_DEST_T;

static void
_hal_nb_pkt_rxEnQueue(
    const UI32_T                    unit,
    const UI32_T                    channel,
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_gpd)
{
    HAL_NB_PKT_RX_CB_T              *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_first_gpd = ptr_sw_gpd;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    HAL_NB_PKT_DEST_T               dest_type;
    HAL_NB_PP_HDR_T                 *ptr_pph;

#if defined(PERF_EN_TEST)
    /* To verify kernel Rx performance */
    if (CLX_E_OK == perf_rxTest())
    {
        while (NULL != ptr_sw_gpd)
        {
            len += ptr_sw_gpd->desc.size
            total_len += len;

            /* next */
            ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        }
        perf_rxCallback(total_len - sizeof(HAL_NB_PP_HDR_T));
        _hal_nb_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
        return ;
    }
#endif

        phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->desc.d_addr_hi, ptr_sw_gpd->desc.d_addr_lo);
        ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
        ptr_pph = (HAL_NB_PP_HDR_T*)ptr_virt_addr;

    // TODO
    // _hal_nb_pkt_getPacketDest(ptr_pph, &dest_type, &ptr_dest);
    dest_type = HAL_NB_PKT_DEST_SDK;

#if defined(NETIF_EN_NETLINK)
    if ((HAL_NB_PKT_DEST_NETDEV  == dest_type) ||
        (HAL_NB_PKT_DEST_NETLINK == dest_type))
#else
    if (HAL_NB_PKT_DEST_NETDEV == dest_type)
#endif
    {
        /*
        * TODO
        */
    }
    else if (HAL_NB_PKT_DEST_SDK == dest_type)
    {
        while (0 != _hal_nb_pkt_enqueue(&ptr_rx_cb->sw_queue[channel], ptr_sw_gpd))
        {
            ptr_rx_cb->cnt.channel[channel].enque_retry++;
            HAL_NB_PKT_RX_ENQUE_RETRY_SLEEP();
        }
        ptr_rx_cb->cnt.channel[channel].enque_ok++;

        osal_triggerEvent(&ptr_rx_cb->sync_sema);
        ptr_rx_cb->cnt.channel[channel].trig_event++;
    }
    else if (HAL_NB_PKT_DEST_DROP == dest_type)
    {
        _hal_nb_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
    }
    else
    {
        DIAG_PRINT((HAL_DBG_ERR | HAL_DBG_RX),
                        "u=%u, rxch=%u, invalid pkt dest=%d\n",
                        unit, channel, dest_type);
    }
}

static void
_hal_nb_pkt_handleRxDoneTask(
    void                            *ptr_argv)
{
    /* cookie or index */
    UI32_T                          unit    = ((HAL_NB_PKT_ISR_COOKIE_T *)ptr_argv)->unit;
    HAL_NB_PDMA_RX_CHANNEL_T        channel = (HAL_NB_PDMA_RX_CHANNEL_T)
                                              ((HAL_NB_PKT_ISR_COOKIE_T *)ptr_argv)->channel;
    /* control block */
    HAL_NB_PKT_RX_CB_T              *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = NULL;

    BOOL_T                          first = TRUE;
    BOOL_T                          last = FALSE;
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_desc = NULL;
    HAL_NB_PKT_RX_SW_DESC_T         *ptr_sw_first_desc = NULL;
    UI32_T                          work_idx = 0;
    UI32_T                          loop_cnt = 0;
    unsigned long                   timeout  = 0;

    osal_initRunThread();
    do
    {
        if (CLX_E_OK != osal_isRunThread())
        {
            DIAG_PRINT(HAL_DBG_RX,
                            "u=%u, rxch=%u, rx done task destroyed\n", unit, channel);
            break; /* deinit-thread */
        }
        /* protect Rx PDMA */
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);

        loop_cnt = ptr_rx_pdma->ring_size;
        work_idx = _hal_nb_pdma_get_rx_ch_work_idx(unit,channel);
        while (loop_cnt > 0)
        {
            ptr_desc = HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, work_idx);
            osal_dma_invalidateCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));

            if(0 == ptr_desc->interrupt)
            {
                break;
            }

            // TODO: Error Handler

            /* Move HW-GPD to SW-GPD and append to a link-list */
            if (TRUE == first)
            {
                ptr_sw_first_desc = (HAL_NB_PKT_RX_SW_DESC_T *)osal_alloc(sizeof(HAL_NB_PKT_RX_SW_DESC_T));
                ptr_sw_desc = ptr_sw_first_desc;
                if (NULL != ptr_sw_desc)
                {
                    memcpy(&ptr_sw_desc->desc, (void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));
                    first = FALSE;
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    DIAG_PRINT((HAL_DBG_RX | HAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc 1st sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_NB_PKT_RX_SW_DESC_T));
                    break;
                }
            }
            else
            {
                ptr_sw_desc->ptr_next = (HAL_NB_PKT_RX_SW_DESC_T *)osal_alloc(sizeof(HAL_NB_PKT_RX_SW_DESC_T));
                ptr_sw_desc = ptr_sw_desc->ptr_next;
                if (NULL != ptr_sw_desc)
                {
                    memcpy(&ptr_sw_desc->desc, (void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    DIAG_PRINT((HAL_DBG_RX | HAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc mid sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_NB_PKT_RX_SW_DESC_T));
                    break;
                }
            }

            /* If hwo=SW and ch=0, enque SW-GPD and signal rxTask */
            if (1 == ptr_desc->eop)
            {
                last = TRUE;
            }

            /* If hwo=SW and ch=*, re-alloc-buf and resume */
            while (CLX_E_OK != _hal_nb_pkt_alloc_rx_payload_buf(unit, channel, work_idx))
            {
                ptr_rx_cb->cnt.no_memory++;
                HAL_NB_PKT_ALLOC_MEM_RETRY_SLEEP();
            }
            ptr_desc->interrupt = 0;
            ptr_desc->eop = 0;
            osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));

            /* Enque the SW-GPD to rxTask */
            if (TRUE == last)
            {
                ptr_sw_desc->ptr_next = NULL;
                ptr_sw_first_desc->rx_complete = TRUE;
                _hal_nb_pkt_rxEnQueue(unit, channel, ptr_sw_first_desc);
                ptr_sw_first_desc = NULL;

                /* To rebuild the SW GPD link list */
                first = TRUE;
                last = FALSE;
            }

            work_idx ++;
            work_idx %= ptr_rx_pdma->ring_size;
            loop_cnt--;
        }
        _hal_nb_pdma_set_rx_ch_work_idx(unit,channel,work_idx);

        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously rx for 1 tick */
        }

    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}
static CLX_ERROR_NO_T
hal_nb_pkt_initTask(
    const UI32_T                    unit)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_TX_CB_T              *ptr_tx_cb = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    HAL_NB_PKT_RX_CB_T              *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    UI32_T                          channel = 0;

    /* Init handleErrorTask */
    rc = osal_createThread("ERROR", HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_STACK,
                           HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_PRI, _hal_nb_pkt_handleErrorTask,
                           (void *)((CLX_HUGE_T)unit), &err_task_id);

    /* Init handleTxDoneTask */
    for (channel = 0; ((channel < HAL_NB_PDMA_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_tx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_tx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("TX_ISR", HAL_DFLT_CFG_PKT_TX_ISR_THREAD_STACK,
                               HAL_DFLT_CFG_PKT_TX_ISR_THREAD_PRI, _hal_nb_pkt_handleTxDoneTask,
                               (void *)&ptr_tx_cb->isr_task_cookie[channel],
                               &ptr_tx_cb->isr_task_id[channel]);
    }

    /* Init handleRxDoneTask */
    for (channel = 0; ((channel < HAL_NB_PDMA_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_rx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_rx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("RX_ISR", HAL_DFLT_CFG_PKT_RX_ISR_THREAD_STACK,
                               HAL_DFLT_CFG_PKT_RX_ISR_THREAD_PRI, _hal_nb_pkt_handleRxDoneTask,
                               (void *)&ptr_rx_cb->isr_task_cookie[channel],
                               &ptr_rx_cb->isr_task_id[channel]);
    }

    /* Init txTask */
    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ptr_tx_cb->running = TRUE;
    }

    ptr_tx_cb->net_tx_allowed = TRUE;

    return (rc);
}

static CLX_ERROR_NO_T
hal_nb_pkt_deinit_task(
    const UI32_T            unit)
{
    HAL_NB_PKT_TX_CB_T     *ptr_tx_cb = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    HAL_NB_PKT_RX_CB_T     *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    UI32_T                  channel = 0;
    /* Make the Rx IOCTL from userspace return back*/
    osal_triggerEvent(&ptr_rx_cb->sync_sema);

    /* Destroy txTask */
    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ptr_tx_cb->running = FALSE;
        osal_triggerEvent(&ptr_tx_cb->sync_sema);
    }

    /* Destroy handleRxDoneTask */
    for (channel = 0; channel < HAL_NB_PDMA_RX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_rx_cb->isr_task_id[channel]);
        // osal_triggerEvent(HAL_NB_PKT_RCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_rx_cb->isr_task_id[channel]);
    }

    /* Destroy handleTxDoneTask */
    for (channel = 0; channel < HAL_NB_PDMA_TX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_tx_cb->isr_task_id[channel]);
        // osal_triggerEvent(HAL_NB_PKT_TCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_tx_cb->isr_task_id[channel]);
    }

    /* Destroy handleErrorTask */
    osal_stopThread(&err_task_id);
    // osal_triggerEvent(HAL_NB_PKT_ERR_EVENT(unit));
    osal_destroyThread(&err_task_id);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
hal_nb_pkt_rx_stop(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      idx;
    HAL_NB_PKT_RX_CB_T          *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_PDMA_T        *ptr_rx_pdma = NULL;

    /* Deinit Rx PDMA and free buf for Rx GPD */
    for (channel = 0; channel < HAL_NB_PDMA_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        _hal_nb_disable_pdma_rx_channel(unit, channel);
        rc = _hal_nb_pkt_deinit_rx_pdma_ring_buf(unit, channel);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }


    /* flush packets in all queues since Rx task may be blocked in user space
     * in this case it won't do ioctl to kernel to handle remaining packets
     */
    for (idx = 0; idx < HAL_NB_PKT_RX_QUEUE_NUM; idx++)
    {
        _hal_nb_pkt_flushRxQueue(unit, &ptr_rx_cb->sw_queue[idx]);
    }

    /* Return user thread */
    ptr_rx_cb->running = FALSE;

    osal_triggerEvent(&ptr_rx_cb->sync_sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_deinit_rx_pdma(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    HAL_NB_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Free DMA */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    osal_dma_free(ptr_rx_pdma->ring_base);
    osal_giveSemaphore(&ptr_rx_pdma->sema);
    osal_destroySemaphore(&ptr_rx_pdma->sema);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_deinit_pkt_rx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PKT_RX_CB_T          *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    /* Deinitialize RX PDMA sub-system */
    for (channel = 0; channel < HAL_NB_PDMA_RX_CHANNEL_LAST; channel++)
    {
        _hal_nb_pkt_deinit_rx_pdma(unit, channel);
    }

    /* Destroy the sync semaphore of rxTask */
    osal_destroyEvent(&ptr_rx_cb->sync_sema);

    /* Deinitialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; queue < HAL_NB_PKT_RX_QUEUE_NUM; queue++)
    {
        osal_destroySemaphore(&ptr_rx_cb->sw_queue[queue].sema);
        osal_que_destroy(&ptr_rx_cb->sw_queue[queue].que_id);
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_deinit_tx_pdma(
    const UI32_T                    unit,
    const HAL_NB_PDMA_TX_CHANNEL_T  channel)
{
    HAL_NB_PKT_TX_CB_T             *ptr_tx_cb   = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    HAL_NB_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_NB_PKT_GET_TX_PDMA_PTR(unit, channel);

    _hal_nb_disable_pdma_tx_channel(unit, channel);

    /* Free DMA and flush queue */
    osal_dma_free(ptr_tx_pdma->ring_base);

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        osal_free(ptr_tx_pdma->pptr_sw_gpd_ring);
        osal_free(ptr_tx_pdma->pptr_sw_gpd_bulk);
    }
    else if (HAL_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        osal_destroySemaphore(&ptr_tx_pdma->sync_intr_sema);
    }

    osal_destroyIsrLock(&ptr_tx_pdma->ring_lock);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_deinit_pkt_tx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PKT_TX_CB_T          *ptr_tx_cb = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    HAL_NB_PDMA_TX_CHANNEL_T    channel = 0;

    /* Deinitialize TX PDMA sub-system.*/
    for (channel = 0; channel < HAL_NB_PDMA_TX_CHANNEL_LAST; channel++)
    {
        _hal_nb_pkt_deinit_tx_pdma(unit, channel);
    }

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Destroy the sync semaphore of txTask */
        osal_destroyEvent(&ptr_tx_cb->sync_sema);

        /* Deinitialize Tx GPD-queue (of first SW-GPD) from handleTxDoneTask to txTask */
        osal_destroySemaphore(&ptr_tx_cb->sw_queue.sema);
        osal_que_destroy(&ptr_tx_cb->sw_queue.que_id);
    }

    return (rc);
}

static CLX_ERROR_NO_T
hal_nb_pkt_deinit_pkt_drv(
    const UI32_T            unit)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    rc = _hal_nb_pkt_deinit_pkt_rx_drv(unit);
    if (CLX_E_OK == rc)
    {
        rc = _hal_nb_pkt_deinit_pkt_tx_drv(unit);
    }

    return rc;
}

static CLX_ERROR_NO_T
_hal_nb_pkt_set_rx_ring_base(
    const UI32_T                        unit,
    const HAL_NB_PDMA_RX_CHANNEL_T      channel,
    const CLX_ADDR_T                    ring_base_phy,
    const UI32_T                        ring_size)
{
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    rc = osal_mdc_writePciReg(unit,
            HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_BASE_REG(channel)),
            (void *)&ring_base_phy, sizeof(CLX_ADDR_T));

    /* Configure the GPD ring size. */
    if (CLX_E_OK == rc)
    {
        rc = osal_mdc_writePciReg(unit,
                HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_SIZE_REG(channel)),
                &ring_size, sizeof(UI32_T));
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_rx_pdma_ring(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_NB_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_desc, 0x0, sizeof(HAL_NB_PDMA_DESC_T));
        ptr_desc->dinc = 1;
        osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_rx_pdma->ring_base);
    rc = _hal_nb_pkt_set_rx_ring_base(unit, channel, phy_addr, ptr_rx_pdma->ring_size);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_rx_pdma(
    const UI32_T                    unit,
    const HAL_NB_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_RX_CB_T              *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Binary semaphore to protect Rx PDMA */
    osal_createSemaphore("RCH_LCK", CLX_SEMAPHORE_BINARY, &ptr_rx_pdma->sema);

    /* Reset Rx PDMA */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    ptr_rx_pdma->cur_idx = 0;
    ptr_rx_pdma->ring_size = HAL_NB_DFLT_RX_RING_SIZE;

    /* Prepare the HW-GPD ring */
    ptr_rx_pdma->ring_base = (HAL_NB_PDMA_DESC_T *)osal_dma_alloc(
        (ptr_rx_pdma->ring_size + 1) * sizeof(HAL_NB_PDMA_DESC_T));

    if (NULL != ptr_rx_pdma->ring_base)
    {
        osal_memset(ptr_rx_pdma->ring_base, 0,
            (ptr_rx_pdma->ring_size + 1) * sizeof(HAL_NB_PDMA_DESC_T));

        ptr_rx_pdma->ring_base_align = (HAL_NB_PDMA_DESC_T *)HAL_PKT_PDMA_ALIGN_ADDR(
            (CLX_HUGE_T)ptr_rx_pdma->ring_base, sizeof(HAL_NB_PDMA_DESC_T));

        /* will initRxPdmaRingBuf and start RCH after setRxConfig */
        rc = _hal_nb_pkt_init_rx_pdma_ring(unit, channel);
    }
    else
    {
        ptr_rx_cb->cnt.no_memory++;
        rc = CLX_E_NO_MEMORY;
    }

    if (CLX_E_OK == rc)
    {
        /* Prepare the SKB ring */
        ptr_rx_pdma->pptr_skb_ring = (struct sk_buff **)osal_alloc(
            ptr_rx_pdma->ring_size * sizeof(struct sk_buff *));

        if (NULL != ptr_rx_pdma->pptr_skb_ring)
        {
            osal_memset(ptr_rx_pdma->pptr_skb_ring, 0x0,
                ptr_rx_pdma->ring_size * sizeof(struct sk_buff *));
        }
        else
        {
            ptr_rx_cb->cnt.no_memory++;
            rc = CLX_E_NO_MEMORY;
        }
    }

    osal_giveSemaphore(&ptr_rx_pdma->sema);

    return (rc);
}


static CLX_ERROR_NO_T
_hal_nb_pkt_init_rx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PKT_RX_CB_T          *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    osal_memset(ptr_rx_cb, 0x0, sizeof(HAL_NB_PKT_RX_CB_T));

    ptr_rx_cb->sched_mode = HAL_DFLT_CFG_PKT_RX_SCHED_MODE;

    /* Sync semaphore to signal rxTask */
    osal_createEvent("RX_SYNC", &ptr_rx_cb->sync_sema);

    /* Initialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; ((queue < HAL_NB_PKT_RX_QUEUE_NUM) && (CLX_E_OK == rc)); queue++)
    {
        ptr_rx_cb->sw_queue[queue].len    = HAL_DFLT_CFG_PKT_RX_QUEUE_LEN;
        ptr_rx_cb->sw_queue[queue].weight = HAL_DFLT_CFG_PKT_RX_QUEUE_WEIGHT;

        osal_createSemaphore("RX_QUE", CLX_SEMAPHORE_BINARY, &ptr_rx_cb->sw_queue[queue].sema);
        osal_que_create(&ptr_rx_cb->sw_queue[queue].que_id, ptr_rx_cb->sw_queue[queue].len);
    }

    /* Init Rx PDMA */
    for (channel = 0; ((channel < HAL_NB_PDMA_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        rc = _hal_nb_pkt_init_rx_pdma(unit, channel);
    }

    return (rc);
}


static CLX_ERROR_NO_T
_hal_nb_pkt_set_tx_ring_base(
    const UI32_T                        unit,
    const HAL_NB_PDMA_TX_CHANNEL_T      channel,
    const CLX_ADDR_T                    ring_base_phy,
    const UI32_T                        ring_size)
{
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    rc = osal_mdc_writePciReg(unit,
            HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_BASE_REG(channel+4)),
            (UI32_T*)&ring_base_phy, sizeof(CLX_ADDR_T));

    /* Configure the ring size. */
    if (CLX_E_OK == rc)
    {
        rc = osal_mdc_writePciReg(unit,
                HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_SIZE_REG(channel+4)),
                &ring_size, sizeof(UI32_T));
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_tx_pdma_ring(
    const UI32_T                    unit,
    const HAL_NB_PDMA_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_TX_PDMA_T            *ptr_tx_pdma = HAL_NB_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_NB_PDMA_DESC_T     *ptr_desc = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_tx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_NB_PKT_GET_TX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_desc, 0x0, sizeof(HAL_NB_PDMA_DESC_T));
        ptr_desc->sinc = 1;
        osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_NB_PDMA_DESC_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_tx_pdma->ring_base);
    rc = _hal_nb_pkt_set_tx_ring_base(unit, channel, phy_addr, ptr_tx_pdma->ring_size);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_tx_pdma(
    const UI32_T                    unit,
    const HAL_NB_PDMA_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_NB_PKT_TX_CB_T              *ptr_tx_cb = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    HAL_NB_PKT_TX_PDMA_T            *ptr_tx_pdma = HAL_NB_PKT_GET_TX_PDMA_PTR(unit, channel);
    CLX_IRQ_FLAGS_T                 irg_flags;

    /* Isr lock to protect Tx PDMA */
    osal_createIsrLock("TCH_LCK", &ptr_tx_pdma->ring_lock);

    if (HAL_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        /* Sync semaphore to signal sendTxPacket */
        osal_createSemaphore("TCH_SYN", CLX_SEMAPHORE_SYNC, &ptr_tx_pdma->sync_intr_sema);
    }

    /* Reset Tx PDMA */
    osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    ptr_tx_pdma->used_idx     = 0;
    ptr_tx_pdma->free_idx     = 0;
    ptr_tx_pdma->used_desc_num = 0;
    ptr_tx_pdma->free_desc_num = HAL_NB_DFLT_TX_RING_SIZE;
    ptr_tx_pdma->ring_size      = HAL_NB_DFLT_TX_RING_SIZE;

    /* Prepare the HW-GPD ring */
    ptr_tx_pdma->ring_base = (HAL_NB_PDMA_DESC_T *)osal_dma_alloc(
        (ptr_tx_pdma->ring_size + 1) * sizeof(HAL_NB_PDMA_DESC_T));

    if (NULL != ptr_tx_pdma->ring_base)
    {
        osal_memset(ptr_tx_pdma->ring_base, 0x0,
            (ptr_tx_pdma->ring_size + 1) * sizeof(HAL_NB_PDMA_DESC_T));
        
        ptr_tx_pdma->ring_base_align = (HAL_NB_PDMA_DESC_T *)HAL_PKT_PDMA_ALIGN_ADDR(
            (CLX_HUGE_T)ptr_tx_pdma->ring_base, sizeof(HAL_NB_PDMA_DESC_T));

        rc = _hal_nb_pkt_init_tx_pdma_ring(unit, channel);
        if (CLX_E_OK == rc)
        {
            _hal_nb_enable_pdma_tx_channel(unit, channel);
        }
    }
    else
    {
        ptr_tx_cb->cnt.no_memory++;
        rc = CLX_E_NO_MEMORY;
    }

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        if (CLX_E_OK == rc)
        {
            /* Prepare the SW-GPD ring */
            ptr_tx_pdma->pptr_sw_gpd_ring = (HAL_NB_PDMA_DESC_T **)osal_alloc(
                ptr_tx_pdma->ring_size * sizeof(HAL_NB_PDMA_DESC_T *));

            if (NULL != ptr_tx_pdma->pptr_sw_gpd_ring)
            {
                osal_memset(ptr_tx_pdma->pptr_sw_gpd_ring, 0x0,
                    ptr_tx_pdma->ring_size * sizeof(HAL_NB_PDMA_DESC_T *));
            }
            else
            {
                ptr_tx_cb->cnt.no_memory++;
                rc = CLX_E_NO_MEMORY;
            }

            /* a temp buffer to store the 1st sw gpd for each packet to be enque
             * we cannot enque packet before release a spinlock
             */
            ptr_tx_pdma->pptr_sw_gpd_bulk = (HAL_NB_PDMA_DESC_T **)osal_alloc(
                ptr_tx_pdma->ring_size * sizeof(HAL_NB_PDMA_DESC_T *));

            if (NULL != ptr_tx_pdma->pptr_sw_gpd_bulk)
            {
                osal_memset(ptr_tx_pdma->pptr_sw_gpd_bulk, 0x0,
                    ptr_tx_pdma->ring_size * sizeof(HAL_NB_PDMA_DESC_T *));
            }
            else
            {
                ptr_tx_cb->cnt.no_memory++;
                rc = CLX_E_NO_MEMORY;
            }
        }
    }

    osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_init_tx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PKT_TX_CB_T          *ptr_tx_cb = HAL_NB_PKT_GET_TX_CB_PTR(unit);
    UI32_T                      channel = 0;

    osal_memset(ptr_tx_cb, 0x0, sizeof(HAL_NB_PKT_TX_CB_T));

    ptr_tx_cb->wait_mode = HAL_NB_PKT_TX_WAIT_MODE;

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Sync semaphore to signal txTask */
        osal_createEvent("TX_SYNC", &ptr_tx_cb->sync_sema);

        /* Initialize Tx GPD-queue (of first SW-GPD) from handleTxDoneTask to txTask */
        ptr_tx_cb->sw_queue.len    = HAL_DFLT_CFG_PKT_TX_QUEUE_LEN;
        ptr_tx_cb->sw_queue.weight = 0;

        osal_createSemaphore("TX_QUE", CLX_SEMAPHORE_BINARY, &ptr_tx_cb->sw_queue.sema);
        osal_que_create(&ptr_tx_cb->sw_queue.que_id, ptr_tx_cb->sw_queue.len);
    }
    else if (HAL_PKT_TX_WAIT_SYNC_POLL == ptr_tx_cb->wait_mode)
    {
        /* Disable TX done ISR. */
        for (channel = 0; channel < HAL_NB_PDMA_TX_CHANNEL_LAST; channel++)
        {
            // _hal_nb_pkt_disableIntr(unit, HAL_NB_PKT_TCH_REG(unit, channel));
        }
    }

    /* Init Tx PDMA */
    for (channel = 0; ((channel < HAL_NB_PDMA_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        rc = _hal_nb_pkt_init_tx_pdma(unit, channel);
    }

    return (rc);
}

static CLX_ERROR_NO_T hal_nb_init_drv(
    const UI32_T            unit)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    rc = _hal_nb_pkt_init_tx_drv(unit);
    if (CLX_E_OK == rc)
    {
        rc = _hal_nb_pkt_init_rx_drv(unit);
    }

    return rc;
}

static CLX_ERROR_NO_T
_hal_nb_pkt_rx_start(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_NB_PDMA_RX_CHANNEL_T    channel = 0;
    HAL_NB_PKT_RX_CB_T          *ptr_rx_cb = HAL_NB_PKT_GET_RX_CB_PTR(unit);
    HAL_NB_PKT_RX_PDMA_T        *ptr_rx_pdma = NULL;

    /* init Rx PDMA and alloc buf for Rx GPD */
    for (channel = 0; channel < HAL_NB_PDMA_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        rc = _hal_nb_pkt_init_rx_pdma_buf(unit, channel);
        if (CLX_E_OK == rc)
        {
            ptr_rx_pdma->cur_idx = 0;
            _hal_nb_enable_pdma_rx_channel(unit, channel);
        }

        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }

    /* enable to dequeue rx packets */
    ptr_rx_cb->running = TRUE;

    return (rc);
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
    UI32_T                          rch;
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma;

    for (rch = 0; rch < HAL_NB_PDMA_TX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    }
    return CLX_E_OK;
}

static CLX_ERROR_NO_T
hal_nb_pkt_unlockRxChannelAll(
    const UI32_T                    unit)
{
    UI32_T                          rch;
    HAL_NB_PKT_RX_PDMA_T            *ptr_rx_pdma;

    for (rch = 0; rch < HAL_NB_PDMA_TX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_NB_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }
    return CLX_E_OK;
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

    osal_memset(_hal_nb_pkt_tx_cb, 0x0,
                CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_NB_PKT_TX_CB_T));
    osal_memset(_hal_nb_pkt_rx_cb, 0x0,
                CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_NB_PKT_RX_CB_T));

    ptr_cb->pkt_init_task = hal_nb_pkt_initTask;
    ptr_cb->pkt_deinit_task = hal_nb_pkt_deinit_task;
    ptr_cb->pkt_rx_stop = hal_nb_pkt_rx_stop;
    ptr_cb->pkt_rx_start = _hal_nb_pkt_rx_start;
    ptr_cb->pkt_deinit_drv = hal_nb_pkt_deinit_pkt_drv;
    ptr_cb->pkt_init_drv = hal_nb_init_drv;
    ptr_cb->pkt_init_irq = NULL;
    ptr_cb->pkt_deinit_irq = NULL;

    ptr_cb->net_dev_tx = _hal_nb_pkt_net_dev_tx;
    ptr_cb->pkt_dev_tx = _hal_nb_pkt_dev_tx;
    ptr_cb->lock_all_rx_channel = hal_nb_pkt_lockRxChannelAll;
    ptr_cb->unlock_all_rx_channel = hal_nb_pkt_unlockRxChannelAll;

    hal_nb_register_netif_ioctl();
}