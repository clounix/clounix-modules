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

/* FILE NAME:  hal_mt_namchabarwa_pkt_knl.c
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

/* netif */
#include <netif/common/netif_osal.h>
#include <netif/common/netif_perf.h>
#include <netif/common/netif_nl.h>

#include <netif/netif_knl.h>
#include <netif/mountain/namchabarwa/netif_mt_namchabarwa_pkt.h>

#include <hal/hal_netif.h>



extern HAL_PKT_DRV_CB_T                                             _hal_pkt_drv_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_MT_NAMCHABARWA_PKT_TX_CB_T                               _hal_mt_namchabarwa_pkt_tx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_MT_NAMCHABARWA_PKT_RX_CB_T                               _hal_mt_namchabarwa_pkt_rx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];

extern HAL_PKT_NETIF_PORT_DB_T                                      _hal_pkt_port_db[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM][HAL_PKT_MAX_PORT_NUM];
static CLX_THREAD_ID_T                                              err_task_id;


#define HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit)                (&_hal_pkt_drv_cb[unit])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit)                 (&_hal_mt_namchabarwa_pkt_tx_cb[unit])
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel)      (&_hal_mt_namchabarwa_pkt_tx_cb[unit].pdma[channel])
#define HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, gpd)  (&_hal_mt_namchabarwa_pkt_tx_cb[unit].pdma[channel].ring_base_align[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit)                 (&_hal_mt_namchabarwa_pkt_rx_cb[unit])
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel)      (&_hal_mt_namchabarwa_pkt_rx_cb[unit].pdma[channel])
#define HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd)  (&_hal_mt_namchabarwa_pkt_rx_cb[unit].pdma[channel].ring_base_align[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_DB(unit,port)               (&_hal_pkt_port_db[unit][port])
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_PROFILE_LIST(unit,port)     (_hal_pkt_port_db[unit][port].ptr_profile_list)
#define HAL_MT_NAMCHABARWA_PKT_GET_PORT_NETDEV(unit,port)           _hal_pkt_port_db[unit][port].ptr_net_dev


/* Sleep Time Definitions */
#define HAL_MT_NAMCHABARWA_PKT_TX_DEQUE_SLEEP()                     osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_RX_DEQUE_SLEEP()                     osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_TX_ENQUE_RETRY_SLEEP()               osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_RX_ENQUE_RETRY_SLEEP()               osal_sleepThread(1000) /* us */
#define HAL_MT_NAMCHABARWA_PKT_ALLOC_MEM_RETRY_SLEEP()              osal_sleepThread(1000) /* us */

typedef enum
{
    HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV = 0,
    HAL_MT_NAMCHABARWA_PKT_DEST_SDK,
    HAL_MT_NAMCHABARWA_PKT_DEST_NETLINK,
    HAL_MT_NAMCHABARWA_PKT_DEST_DROP,
    HAL_MT_NAMCHABARWA_PKT_DEST_LAST
} HAL_MT_NAMCHABARWA_PKT_DEST_T;


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_enable_pdma_rx_channel(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_SET_BIT(enable,(1 << channel));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_disable_pdma_rx_channel(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_CLR_BIT(enable,(1 << channel));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_enable_pdma_tx_channel(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_SET_BIT(enable,(1 << (channel + 4)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
_hal_mt_namchabarwa_disable_pdma_tx_channel(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          enable;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE), &enable, sizeof(UI32_T));
    HAL_NETIF_CLR_BIT(enable,(1 << (channel + 4)));
    osal_mdc_writePciReg(unit,HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_PDMA_CFG_CH_ENABLE),&enable, sizeof(UI32_T));
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pdma_set_rx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    work_idx)
{
    osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &work_idx, sizeof(UI32_T));
    return CLX_E_OK;
}
static UI32_T
_hal_mt_namchabarwa_pdma_get_rx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    UI32_T                          work_idx;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &work_idx, sizeof(UI32_T));
    return work_idx;
}
// static UI32_T
// _hal_mt_namchabarwa_pdma_get_rx_ch_pop_idx(
//     const UI32_T                    unit,
//     const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
// {
//     UI32_T                          pop_idx;
//     osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)), &pop_idx, sizeof(UI32_T));
//     return pop_idx;
// }

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pdma_set_tx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel,
    const UI32_T                    work_idx)
{
    osal_mdc_writePciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &work_idx, sizeof(UI32_T));
    return CLX_E_OK;
}
static UI32_T
_hal_mt_namchabarwa_pdma_get_tx_ch_work_idx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          work_idx;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &work_idx, sizeof(UI32_T));
    return work_idx;
}
static UI32_T
_hal_mt_namchabarwa_pdma_get_tx_ch_pop_idx(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    UI32_T                          pop_idx;
    osal_mdc_readPciReg(unit, HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_DESC_WORK_IDX_REG(channel+4)), &pop_idx, sizeof(UI32_T));
    return pop_idx;
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_alloc_rx_payload_buf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    gpd_idx)
{
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;

    void                            *ring_buf = NULL;

    ring_buf = osal_dma_alloc(ptr_cb->buf_len + sizeof(HAL_MT_NAMCHABARWA_PP_HDR_T));
    if(ring_buf == NULL)
    {
        OSAL_PRINT(OSAL_DBG_ERR,"No memory!");
        return CLX_E_NO_MEMORY;
    }
    phy_addr = osal_dma_convertVirtToPhy(ring_buf);

        ptr_desc->s_addr_hi = CLX_ADDR_64_HI(phy_addr);
        ptr_desc->s_addr_lo = CLX_ADDR_64_LOW(phy_addr);
        ptr_desc->size      = ptr_cb->buf_len;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_rx_pdma_buf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = NULL;
    UI32_T                          gpd_idx = 0;

    if (0 == ptr_cb->buf_len)
    {
        return (CLX_E_BAD_PARAMETER);
    }

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_dma_invalidateCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

        rc = _hal_mt_namchabarwa_pkt_alloc_rx_payload_buf(unit, channel, gpd_idx);
        if (CLX_E_OK != rc)
        {
            OSAL_PRINT(OSAL_DBG_ERR,"No memory!");
            ptr_rx_cb->cnt.no_memory++;
            break;
        }
        ptr_desc->interrupt = 0;
        ptr_desc->err = 0;
        ptr_desc->eop = 0;
        osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

    }

    return (rc);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_free_rx_payload_buf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    gpd_idx)
{
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
    CLX_ADDR_T                      phy_addr = 0;
    void                            *ptr_virt_addr;

    phy_addr = CLX_ADDR_32_TO_64(ptr_desc->d_addr_hi, ptr_desc->d_addr_lo);
    if (0x0 == phy_addr)
    {
        OSAL_PRINT(OSAL_DBG_ERR,"descriptor d_addr is NULL");
        return CLX_E_OTHERS;
    }
    ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
    osal_dma_free(ptr_virt_addr);

    ptr_desc->d_addr_hi = 0x0;
    ptr_desc->d_addr_lo = 0x0;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinit_rx_pdma_ring_buf(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; ((gpd_idx < ptr_rx_pdma->ring_size) && (CLX_E_OK == rc)); gpd_idx++)
    {
        /* we shuold process all descriptors in Rx-done task before free */
        // TODO
        rc = _hal_mt_namchabarwa_pkt_free_rx_payload_buf(unit, channel, gpd_idx);
    }
    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_free_rx_payload_buf_desc(
    const UI32_T                    unit,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_desc)
{
    CLX_ADDR_T                      phy_addr = 0;
    void                            *ptr_virt_addr;

    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_desc->desc.d_addr_hi, ptr_sw_desc->desc.d_addr_lo);
    if (0x0 == phy_addr)
    {
        OSAL_PRINT(OSAL_DBG_ERR,"descriptor d_addr is NULL");
        return CLX_E_OTHERS;
    }
    ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
    osal_dma_free(ptr_virt_addr);

    return (CLX_E_OK);
}

static void
_hal_mt_namchabarwa_pkt_freeTxGpdList(
    UI32_T                          unit,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd)
{
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd_cur = NULL;

    while (NULL != ptr_sw_gpd)
    {
        ptr_sw_gpd_cur = ptr_sw_gpd;
        ptr_sw_gpd = ptr_sw_gpd->ptr_next;
        osal_free(ptr_sw_gpd_cur);
    }
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_freeRxGpdList(
    UI32_T                          unit,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_desc,
    BOOL_T                          free_payload)
{
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_desc_cur = NULL;

    while (NULL != ptr_sw_desc)
    {
        ptr_sw_desc_cur = ptr_sw_desc;
        ptr_sw_desc = ptr_sw_desc->ptr_next;
        if (TRUE == free_payload)
        {
            _hal_mt_namchabarwa_pkt_free_rx_payload_buf_desc(unit, ptr_sw_desc_cur);
        }
        osal_free(ptr_sw_desc_cur);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_enqueue(
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
_hal_mt_namchabarwa_pkt_dequeue(
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
_hal_mt_namchabarwa_pkt_get_queue_count(
    HAL_PKT_SW_QUEUE_T  *ptr_que,
    UI32_T                  *ptr_count)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    osal_que_getCount(&ptr_que->que_id, ptr_count);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_flushRxQueue(
    const UI32_T                unit,
    HAL_PKT_SW_QUEUE_T      *ptr_que)
{
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T     *ptr_sw_desc_knl = NULL;
    CLX_ERROR_NO_T              rc;

    while (1)
    {
        rc = _hal_mt_namchabarwa_pkt_dequeue(ptr_que, (void **)&ptr_sw_desc_knl);
        if (CLX_E_OK != rc)
        {
            break;
        }
        _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_desc_knl, TRUE);
    }

    return (CLX_E_OK);
}

static void
_hal_mt_namchabarwa_pkt_handleErrorTask(
    void                    *ptr_argv)
{
}

static void
_hal_mt_namchabarwa_pkt_txEnQueueBulk(
    const UI32_T                    unit,
    const UI32_T                    channel,
    const UI32_T                    number)
{
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd = NULL;
    UI32_T                          idx;

    for (idx = 0; idx < number; idx++)
    {
        ptr_sw_gpd = ptr_tx_pdma->pptr_sw_gpd_bulk[idx];
        ptr_tx_pdma->pptr_sw_gpd_bulk[idx] = NULL;
        if (NULL != ptr_sw_gpd->callback)
        {
            ptr_sw_gpd->callback(unit, ptr_sw_gpd, ptr_sw_gpd->ptr_cookie);
        }
    }
}

static void
_hal_mt_namchabarwa_pkt_handleTxDoneTask(
    void                    *ptr_argv)
{
    /* cookie or index */
    UI32_T                          unit    = ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->unit;
    HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T        channel = (HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T)
                                              ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->channel;
    /* control block */
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T   *ptr_tx_gpd = NULL;
    UI32_T                          first_gpd_idx = 0; /* To record the first GPD */
    UI32_T                          loop_cnt = 0;
    CLX_IRQ_FLAGS_T                 irg_flags;
    unsigned long                   timeout  = 0;
    UI32_T                          bulk_pkt_cnt = 0;

    osal_initRunThread();
    do
    {
        if (CLX_E_OK != osal_isRunThread())
        {
            OSAL_PRINT(OSAL_DBG_TX,
                            "u=%u, txch=%u, tx done task destroyed\n", unit, channel);
            break; /* deinit-thread */
        }

        /* protect Tx PDMA
         * for sync-intr, the sema is locked by sendGpd
         */
        if (HAL_PKT_TX_WAIT_SYNC_INTR != ptr_tx_cb->wait_mode)
        {
            osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);
        }

        loop_cnt = ptr_tx_pdma->used_desc_num;
        while (loop_cnt > 0)
        {
            ptr_tx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, ptr_tx_pdma->free_idx);
            osal_dma_invalidateCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

            // TODO: Error Handler

            if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
            {
                /* If sop=1, record the head of sw gpd in bulk buf */
                if (1 == ptr_tx_gpd->sop)
                {
                    ptr_tx_pdma->pptr_sw_gpd_bulk[bulk_pkt_cnt]
                        = ptr_tx_pdma->pptr_sw_gpd_ring[first_gpd_idx];

                    bulk_pkt_cnt++;
                    ptr_tx_pdma->pptr_sw_gpd_ring[first_gpd_idx] = NULL;

                    /* next SW-GPD must be the head of another PKT->SW-GPD */
                    first_gpd_idx = ptr_tx_pdma->free_idx + 1;
                    first_gpd_idx %= ptr_tx_pdma->ring_size;
                }
            }

            if (1 == ptr_tx_gpd->err)
            {
                ptr_tx_cb->cnt.channel[channel].error++;
            }

            /* update Tx PDMA */
            ptr_tx_pdma->free_idx++;
            ptr_tx_pdma->free_idx %= ptr_tx_pdma->ring_size;
            ptr_tx_pdma->used_desc_num--;
            ptr_tx_pdma->free_desc_num++;
            loop_cnt--;
        }

        /* let the netdev resume Tx */
        hal_pkt_resumeAllIntf(unit);

        /* update ISR and counter */
        ptr_tx_cb->cnt.channel[channel].tx_done++;


        if (HAL_PKT_TX_WAIT_SYNC_INTR != ptr_tx_cb->wait_mode)
        {
            osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);
        }
        else
        {
            osal_giveSemaphore(&ptr_tx_pdma->sync_intr_sema);
        }

        /* enque packet after releasing the spinlock */
        _hal_mt_namchabarwa_pkt_txEnQueueBulk(unit, channel, bulk_pkt_cnt);
        bulk_pkt_cnt = 0;

        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously free tx descriptor for 1 tick */
        }

    } while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}



static void
_hal_mt_namchabarwa_pkt_rxEnQueue(
    const UI32_T                    unit,
    const UI32_T                    channel,
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_gpd)
{
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_first_gpd = ptr_sw_gpd;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    HAL_MT_NAMCHABARWA_PKT_DEST_T               dest_type;
    HAL_MT_NAMCHABARWA_PP_HDR_T                 *ptr_pph;

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
        perf_rxCallback(total_len - sizeof(HAL_MT_NAMCHABARWA_PP_HDR_T));
        _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
        return ;
    }
#endif

        phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->desc.d_addr_hi, ptr_sw_gpd->desc.d_addr_lo);
        ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);
        ptr_pph = (HAL_MT_NAMCHABARWA_PP_HDR_T*)ptr_virt_addr;

    // TODO
    // _hal_mt_namchabarwa_pkt_getPacketDest(ptr_pph, &dest_type, &ptr_dest);
    dest_type = HAL_MT_NAMCHABARWA_PKT_DEST_SDK;

#if defined(NETIF_EN_NETLINK)
    if ((HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV  == dest_type) ||
        (HAL_MT_NAMCHABARWA_PKT_DEST_NETLINK == dest_type))
#else
    if (HAL_MT_NAMCHABARWA_PKT_DEST_NETDEV == dest_type)
#endif
    {
        /*
        * TODO
        */
    }
    else if (HAL_MT_NAMCHABARWA_PKT_DEST_SDK == dest_type)
    {
        while (0 != _hal_mt_namchabarwa_pkt_enqueue(&ptr_rx_cb->sw_queue[channel], ptr_sw_gpd))
        {
            ptr_rx_cb->cnt.channel[channel].enque_retry++;
            HAL_MT_NAMCHABARWA_PKT_RX_ENQUE_RETRY_SLEEP();
        }
        ptr_rx_cb->cnt.channel[channel].enque_ok++;

        osal_triggerEvent(&ptr_rx_cb->sync_sema);
        ptr_rx_cb->cnt.channel[channel].trig_event++;
    }
    else if (HAL_MT_NAMCHABARWA_PKT_DEST_DROP == dest_type)
    {
        _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd, TRUE);
    }
    else
    {
        OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_RX),
                        "u=%u, rxch=%u, invalid pkt dest=%d\n",
                        unit, channel, dest_type);
    }
}

static void
_hal_mt_namchabarwa_pkt_handleRxDoneTask(
    void                            *ptr_argv)
{
    /* cookie or index */
    UI32_T                          unit    = ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->unit;
    HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T        channel = (HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T)
                                              ((HAL_MT_NAMCHABARWA_PKT_ISR_COOKIE_T *)ptr_argv)->channel;
    /* control block */
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = NULL;

    BOOL_T                          first = TRUE;
    BOOL_T                          last = FALSE;
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_desc = NULL;
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_first_desc = NULL;
    UI32_T                          work_idx = 0;
    UI32_T                          loop_cnt = 0;
    unsigned long                   timeout  = 0;

    osal_initRunThread();
    do
    {
        if (CLX_E_OK != osal_isRunThread())
        {
            OSAL_PRINT(OSAL_DBG_RX,
                            "u=%u, rxch=%u, rx done task destroyed\n", unit, channel);
            break; /* deinit-thread */
        }
        /* protect Rx PDMA */
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);

        loop_cnt = ptr_rx_pdma->ring_size;
        work_idx = _hal_mt_namchabarwa_pdma_get_rx_ch_work_idx(unit,channel);
        while (loop_cnt > 0)
        {
            ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, work_idx);
            osal_dma_invalidateCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

            if(0 == ptr_desc->interrupt)
            {
                break;
            }

            // TODO: Error Handler

            /* Move HW-GPD to SW-GPD and append to a link-list */
            if (TRUE == first)
            {
                ptr_sw_first_desc = (HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T *)osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T));
                ptr_sw_desc = ptr_sw_first_desc;
                if (NULL != ptr_sw_desc)
                {
                    memcpy(&ptr_sw_desc->desc, (void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
                    first = FALSE;
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc 1st sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T));
                    break;
                }
            }
            else
            {
                ptr_sw_desc->ptr_next = (HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T *)osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T));
                ptr_sw_desc = ptr_sw_desc->ptr_next;
                if (NULL != ptr_sw_desc)
                {
                    memcpy(&ptr_sw_desc->desc, (void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
                }
                else
                {
                    ptr_rx_cb->cnt.no_memory++;
                    OSAL_PRINT((OSAL_DBG_RX | OSAL_DBG_ERR),
                                    "u=%u, rxch=%u, alloc mid sw gpd failed, size=%zu\n",
                                    unit, channel, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T));
                    break;
                }
            }

            /* If hwo=SW and ch=0, enque SW-GPD and signal rxTask */
            if (1 == ptr_desc->eop)
            {
                last = TRUE;
            }

            /* If hwo=SW and ch=*, re-alloc-buf and resume */
            while (CLX_E_OK != _hal_mt_namchabarwa_pkt_alloc_rx_payload_buf(unit, channel, work_idx))
            {
                ptr_rx_cb->cnt.no_memory++;
                HAL_MT_NAMCHABARWA_PKT_ALLOC_MEM_RETRY_SLEEP();
            }
            ptr_desc->interrupt = 0;
            ptr_desc->eop = 0;
            osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

            /* Enque the SW-GPD to rxTask */
            if (TRUE == last)
            {
                ptr_sw_desc->ptr_next = NULL;
                ptr_sw_first_desc->rx_complete = TRUE;
                _hal_mt_namchabarwa_pkt_rxEnQueue(unit, channel, ptr_sw_first_desc);
                ptr_sw_first_desc = NULL;

                /* To rebuild the SW GPD link list */
                first = TRUE;
                last = FALSE;
            }

            work_idx ++;
            work_idx %= ptr_rx_pdma->ring_size;
            loop_cnt--;
        }
        _hal_mt_namchabarwa_pdma_set_rx_ch_work_idx(unit,channel,work_idx);

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
hal_mt_namchabarwa_pkt_initTask(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_PKT_IOCTL_TASK_COOKIE_T     *ptr_cookie = (HAL_PKT_IOCTL_TASK_COOKIE_T*)ptr_data;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T              *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    UI32_T                          channel = 0;

    /* Init handleErrorTask */
    rc = osal_createThread("ERROR", ptr_cookie->error_isr_stack_size,
                           ptr_cookie->error_thread_pri, _hal_mt_namchabarwa_pkt_handleErrorTask,
                           (void *)((CLX_HUGE_T)unit), &err_task_id);

    /* Init handleTxDoneTask */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_tx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_tx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("TX_ISR", ptr_cookie->tx_isr_stack_size,
                               ptr_cookie->tx_thread_pri, _hal_mt_namchabarwa_pkt_handleTxDoneTask,
                               (void *)&ptr_tx_cb->isr_task_cookie[channel],
                               &ptr_tx_cb->isr_task_id[channel]);
    }

    /* Init handleRxDoneTask */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        ptr_rx_cb->isr_task_cookie[channel].unit    = unit;
        ptr_rx_cb->isr_task_cookie[channel].channel = channel;

        rc = osal_createThread("RX_ISR", ptr_cookie->rx_isr_stack_size,
                               ptr_cookie->rx_thread_pri, _hal_mt_namchabarwa_pkt_handleRxDoneTask,
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
hal_mt_namchabarwa_pkt_deinit_task(
    const UI32_T            unit,
    void                        *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T     *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T     *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
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
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_rx_cb->isr_task_id[channel]);
        // osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_RCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_rx_cb->isr_task_id[channel]);
    }

    /* Destroy handleTxDoneTask */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST; channel++)
    {
        osal_stopThread(&ptr_tx_cb->isr_task_id[channel]);
        // osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_TCH_EVENT(unit, channel));
        osal_destroyThread(&ptr_tx_cb->isr_task_id[channel]);
    }

    /* Destroy handleErrorTask */
    osal_stopThread(&err_task_id);
    // osal_triggerEvent(HAL_MT_NAMCHABARWA_PKT_ERR_EVENT(unit));
    osal_destroyThread(&err_task_id);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_rx_stop(
    const UI32_T                unit,
    void                        *ptr_data)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      idx;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T          *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T        *ptr_rx_pdma = NULL;

    /* Deinit Rx PDMA and free buf for Rx GPD */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        _hal_mt_namchabarwa_disable_pdma_rx_channel(unit, channel);
        rc = _hal_mt_namchabarwa_pkt_deinit_rx_pdma_ring_buf(unit, channel);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }


    /* flush packets in all queues since Rx task may be blocked in user space
     * in this case it won't do ioctl to kernel to handle remaining packets
     */
    for (idx = 0; idx < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; idx++)
    {
        _hal_mt_namchabarwa_pkt_flushRxQueue(unit, &ptr_rx_cb->sw_queue[idx]);
    }

    /* Return user thread */
    ptr_rx_cb->running = FALSE;

    osal_triggerEvent(&ptr_rx_cb->sync_sema);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinit_rx_pdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T           *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Free DMA */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    osal_dma_free(ptr_rx_pdma->ring_base);
    osal_giveSemaphore(&ptr_rx_pdma->sema);
    osal_destroySemaphore(&ptr_rx_pdma->sema);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinit_pkt_rx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T          *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    /* Deinitialize RX PDMA sub-system */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_deinit_rx_pdma(unit, channel);
    }

    /* Destroy the sync semaphore of rxTask */
    osal_destroyEvent(&ptr_rx_cb->sync_sema);

    /* Deinitialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; queue++)
    {
        osal_destroySemaphore(&ptr_rx_cb->sw_queue[queue].sema);
        osal_que_destroy(&ptr_rx_cb->sw_queue[queue].que_id);
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_deinit_tx_pdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb   = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);

    _hal_mt_namchabarwa_disable_pdma_tx_channel(unit, channel);

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
_hal_mt_namchabarwa_pkt_deinit_pkt_tx_drv(
    const UI32_T                unit)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T          *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T    channel = 0;

    /* Deinitialize TX PDMA sub-system.*/
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST; channel++)
    {
        _hal_mt_namchabarwa_pkt_deinit_tx_pdma(unit, channel);
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
hal_mt_namchabarwa_pkt_deinit_pkt_drv(
    const UI32_T            unit,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    rc = _hal_mt_namchabarwa_pkt_deinit_pkt_rx_drv(unit);
    if (CLX_E_OK == rc)
    {
        rc = _hal_mt_namchabarwa_pkt_deinit_pkt_tx_drv(unit);
    }

    return rc;
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_set_rx_ring_base(
    const UI32_T                        unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T      channel,
    const CLX_ADDR_T                    ring_base_phy,
    const UI32_T                        ring_size)
{
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    rc = osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_RING_BASE_REG(channel)),
            (void *)&ring_base_phy, sizeof(CLX_ADDR_T));

    /* Configure the GPD ring size. */
    if (CLX_E_OK == rc)
    {
        rc = osal_mdc_writePciReg(unit,
                HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_RING_SIZE_REG(channel)),
                &ring_size, sizeof(UI32_T));
    }

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_rx_pdma_ring(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_rx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_RX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_desc, 0x0, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
        ptr_desc->dinc = 1;
        osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_rx_pdma->ring_base);
    rc = _hal_mt_namchabarwa_pkt_set_rx_ring_base(unit, channel, phy_addr, ptr_rx_pdma->ring_size);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_rx_pdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T  channel,
    const UI32_T                    ring_size)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T              *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

    /* Binary semaphore to protect Rx PDMA */
    osal_createSemaphore("RCH_LCK", CLX_SEMAPHORE_BINARY, &ptr_rx_pdma->sema);

    /* Reset Rx PDMA */
    osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    ptr_rx_pdma->cur_idx = 0;
    ptr_rx_pdma->ring_size = ring_size;

    /* Prepare the HW-GPD ring */
    ptr_rx_pdma->ring_base = (HAL_MT_NAMCHABARWA_PDMA_DESC_T *)osal_dma_alloc(
        (ptr_rx_pdma->ring_size + 1) * sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

    if (NULL == ptr_rx_pdma->ring_base)
    {
        ptr_rx_cb->cnt.no_memory++;
        OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"No memory!");
        rc = CLX_E_NO_MEMORY;
        goto exit;
    }
    osal_memset(ptr_rx_pdma->ring_base, 0,
        (ptr_rx_pdma->ring_size + 1) * sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

    ptr_rx_pdma->ring_base_align = (HAL_MT_NAMCHABARWA_PDMA_DESC_T *)HAL_PKT_PDMA_ALIGN_ADDR(
        (CLX_HUGE_T)ptr_rx_pdma->ring_base, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

    /* will initRxPdmaRingBuf and start RCH after setRxConfig */
    rc = _hal_mt_namchabarwa_pkt_init_rx_pdma_ring(unit, channel);
    if (CLX_E_OK != rc)
    {
        OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"Init rx pdma ring failed");
        goto free_ring_base;
    }
    /* Prepare the SKB ring */
    ptr_rx_pdma->pptr_skb_ring = (struct sk_buff **)osal_alloc(
        ptr_rx_pdma->ring_size * sizeof(struct sk_buff *));

    if (NULL == ptr_rx_pdma->pptr_skb_ring)
    {
        ptr_rx_cb->cnt.no_memory++;
        rc = CLX_E_NO_MEMORY;
        goto free_ring_base;
    }

    osal_memset(ptr_rx_pdma->pptr_skb_ring, 0x0,
        ptr_rx_pdma->ring_size * sizeof(struct sk_buff *));
    
    osal_giveSemaphore(&ptr_rx_pdma->sema);
    return (rc);

free_ring_base:
    osal_dma_free(ptr_rx_pdma->ring_base);
exit:
    osal_giveSemaphore(&ptr_rx_pdma->sema);
    return (rc);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_rx_drv(
    const UI32_T                unit,
    HAL_PKT_IOCTL_DRV_COOKIE_T        *ptr_data)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T          *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T    channel = 0;
    UI32_T                      queue = 0;

    osal_memset(ptr_rx_cb, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CB_T));

    ptr_rx_cb->sched_mode = ptr_data->rx_sched_mode;

    /* Sync semaphore to signal rxTask */
    osal_createEvent("RX_SYNC", &ptr_rx_cb->sync_sema);

    /* Initialize Rx GPD-queue (of first SW-GPD) from handleRxDoneTask to rxTask */
    for (queue = 0; ((queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM) && (CLX_E_OK == rc)); queue++)
    {
        ptr_rx_cb->sw_queue[queue].len    = ptr_data->rx_que.len;
        ptr_rx_cb->sw_queue[queue].weight = ptr_data->rx_que.weight;

        osal_createSemaphore("RX_QUE", CLX_SEMAPHORE_BINARY, &ptr_rx_cb->sw_queue[queue].sema);
        osal_que_create(&ptr_rx_cb->sw_queue[queue].que_id, ptr_rx_cb->sw_queue[queue].len);
    }

    /* Init Rx PDMA */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        rc = _hal_mt_namchabarwa_pkt_init_rx_pdma(unit, channel,ptr_data->ring_size);
    }

    return (rc);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_set_tx_ring_base(
    const UI32_T                        unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T      channel,
    const CLX_ADDR_T                    ring_base_phy,
    const UI32_T                        ring_size)
{
    osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_RING_BASE_REG(channel+4)),
            (UI32_T*)&ring_base_phy, sizeof(CLX_ADDR_T));

    /* Configure the ring size. */
    osal_mdc_writePciReg(unit,
            HAL_MT_NAMCHABARWA_PDMA_GET_MMIO(HAL_MT_NAMCHABARWA_GET_PDMA_CH_RING_SIZE_REG(channel+4)),
            &ring_size, sizeof(UI32_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_tx_pdma_ring(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T            *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T     *ptr_desc = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          gpd_idx = 0;

    for (gpd_idx = 0; gpd_idx < ptr_tx_pdma->ring_size; gpd_idx++)
    {
        ptr_desc = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, gpd_idx);
        osal_memset((void *)ptr_desc, 0x0, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
        ptr_desc->sinc = 1;
        osal_dma_flushCache((void *)ptr_desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
    }

    phy_addr = osal_dma_convertVirtToPhy(ptr_tx_pdma->ring_base);
    rc = _hal_mt_namchabarwa_pkt_set_tx_ring_base(unit, channel, phy_addr, ptr_tx_pdma->ring_size);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_tx_pdma(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel,
    const UI32_T                    ring_size)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T              *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T            *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
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

    ptr_tx_pdma->free_idx      = 0;
    ptr_tx_pdma->used_desc_num = 0;
    ptr_tx_pdma->free_desc_num = ring_size;
    ptr_tx_pdma->ring_size     = ring_size;

    /* Prepare the HW-GPD ring */
    ptr_tx_pdma->ring_base = (HAL_MT_NAMCHABARWA_PDMA_DESC_T *)osal_dma_alloc(
        (ptr_tx_pdma->ring_size + 1) * sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
    if (NULL == ptr_tx_pdma->ring_base)
    {
        ptr_tx_cb->cnt.no_memory++;
        OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"No memory!");
        rc = CLX_E_NO_MEMORY;
        goto exit;
    }
    osal_memset(ptr_tx_pdma->ring_base, 0x0,
        (ptr_tx_pdma->ring_size + 1) * sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
    
    ptr_tx_pdma->ring_base_align = (HAL_MT_NAMCHABARWA_PDMA_DESC_T *)HAL_PKT_PDMA_ALIGN_ADDR(
        (CLX_HUGE_T)ptr_tx_pdma->ring_base, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

    rc = _hal_mt_namchabarwa_pkt_init_tx_pdma_ring(unit, channel);
    if (CLX_E_OK != rc)
    {
        OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"init_tx_pdma_ring failed");
        goto free_ring_base;
    }
    _hal_mt_namchabarwa_enable_pdma_tx_channel(unit, channel);

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Prepare the SW-GPD ring */
        ptr_tx_pdma->pptr_sw_gpd_ring = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T **)osal_alloc(
            ptr_tx_pdma->ring_size * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));

        if (NULL == ptr_tx_pdma->pptr_sw_gpd_ring)
        {
            ptr_tx_cb->cnt.no_memory++;
            OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"No memory!");
            rc = CLX_E_NO_MEMORY;
            goto free_ring_base;
        }
        osal_memset(ptr_tx_pdma->pptr_sw_gpd_ring, 0x0,
            ptr_tx_pdma->ring_size * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));

        /* a temp buffer to store the 1st sw gpd for each packet to be enque
            * we cannot enque packet before release a spinlock
            */
        ptr_tx_pdma->pptr_sw_gpd_bulk = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T **)osal_alloc(
            ptr_tx_pdma->ring_size * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));
        if (NULL == ptr_tx_pdma->pptr_sw_gpd_bulk)
        {
            ptr_tx_cb->cnt.no_memory++;
            OSAL_PRINT(OSAL_DBG_RX|OSAL_DBG_ERR,"No memory!");
            rc = CLX_E_NO_MEMORY;
            goto free_sw_gpd_ring;
        }
        osal_memset(ptr_tx_pdma->pptr_sw_gpd_bulk, 0x0,
            ptr_tx_pdma->ring_size * sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *));
    }
    osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    return (rc);
free_sw_gpd_ring:
    osal_free(ptr_tx_pdma->pptr_sw_gpd_ring);
free_ring_base:
    osal_dma_free(ptr_tx_pdma->ring_base);
exit:
    osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irg_flags);

    return (rc);
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_init_tx_drv(
    const UI32_T                unit,
    HAL_PKT_IOCTL_DRV_COOKIE_T        *ptr_data)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T          *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    UI32_T                      channel = 0;

    osal_memset(ptr_tx_cb, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CB_T));

    // ptr_tx_cb->wait_mode = ptr_data->tx_wait_mode;
    ptr_tx_cb->wait_mode = HAL_MT_NAMCHABARWA_PKT_TX_WAIT_MODE;

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Sync semaphore to signal txTask */
        osal_createEvent("TX_SYNC", &ptr_tx_cb->sync_sema);

        /* Initialize Tx GPD-queue (of first SW-GPD) from handleTxDoneTask to txTask */
        ptr_tx_cb->sw_queue.len    = ptr_data->tx_que.len;
        ptr_tx_cb->sw_queue.weight = ptr_data->tx_que.weight;

        osal_createSemaphore("TX_QUE", CLX_SEMAPHORE_BINARY, &ptr_tx_cb->sw_queue.sema);
        osal_que_create(&ptr_tx_cb->sw_queue.que_id, ptr_tx_cb->sw_queue.len);
    }
    else if (HAL_PKT_TX_WAIT_SYNC_POLL == ptr_tx_cb->wait_mode)
    {
        /* Disable TX done ISR. */
        for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST; channel++)
        {
            // _hal_mt_namchabarwa_pkt_disableIntr(unit, HAL_MT_NAMCHABARWA_PKT_TCH_REG(unit, channel));
        }
    }

    /* Init Tx PDMA */
    for (channel = 0; ((channel < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST) && (CLX_E_OK == rc)); channel++)
    {
        rc = _hal_mt_namchabarwa_pkt_init_tx_pdma(unit, channel,ptr_data->ring_size);
    }

    return (rc);
}

static CLX_ERROR_NO_T hal_mt_namchabarwa_init_drv(
    const UI32_T                unit,
    void                        *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    HAL_PKT_IOCTL_DRV_COOKIE_T    *ptr_cookie=(HAL_PKT_IOCTL_DRV_COOKIE_T*)ptr_data;

    rc = _hal_mt_namchabarwa_pkt_init_tx_drv(unit,ptr_cookie);
    if (CLX_E_OK != rc)
    {
        OSAL_PRINT(OSAL_DBG_ERR,"init tx drv failed");
        return rc;
    }
    rc = _hal_mt_namchabarwa_pkt_init_rx_drv(unit,ptr_cookie);

    return rc;
}

static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_rx_start(
    const UI32_T                unit,
    void                        *ptr_data)
{
    CLX_ERROR_NO_T              rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_T    channel = 0;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T          *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T        *ptr_rx_pdma = NULL;

    /* init Rx PDMA and alloc buf for Rx GPD */
    for (channel = 0; channel < HAL_MT_NAMCHABARWA_PDMA_RX_CHANNEL_LAST; channel++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, channel);

        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
        rc = _hal_mt_namchabarwa_pkt_init_rx_pdma_buf(unit, channel);
        if (CLX_E_OK != rc)
        {
            OSAL_PRINT(OSAL_DBG_ERR,"init rx pdma buf failed");
            osal_giveSemaphore(&ptr_rx_pdma->sema);
            return rc;
        }
        ptr_rx_pdma->cur_idx = 0;
        _hal_mt_namchabarwa_enable_pdma_rx_channel(unit, channel);

        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }

    /* enable to dequeue rx packets */
    ptr_rx_cb->running = TRUE;

    return (rc);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_waitTxDone(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T   *ptr_sw_gpd)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    UI32_T                          loop_cnt = 0;
    UI32_T                          work_idx;
    UI32_T                          pop_idx;

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        ;
    }
    else if (HAL_PKT_TX_WAIT_SYNC_INTR == ptr_tx_cb->wait_mode)
    {
        osal_takeSemaphore(&ptr_tx_pdma->sync_intr_sema, HAL_MT_NAMCHABARWA_PKT_PDMA_TX_INTR_TIMEOUT);
        /* rc = _hal_mt_namchabarwa_pkt_invokeTxGpdCallback(unit, ptr_sw_gpd); */
    }
    else if (HAL_PKT_TX_WAIT_SYNC_POLL == ptr_tx_cb->wait_mode)
    {
        work_idx = _hal_mt_namchabarwa_pdma_get_tx_ch_work_idx(unit,channel);
        pop_idx = _hal_mt_namchabarwa_pdma_get_tx_ch_pop_idx(unit,channel);
        do
        {
            loop_cnt++;
            if (0 == loop_cnt % HAL_MT_NAMCHABARWA_PKT_PDMA_TX_POLL_MAX_LOOP)
            {
                ptr_tx_cb->cnt.channel[channel].poll_timeout++;
                rc = CLX_E_OTHERS;
                break;
            }
        } while (work_idx != pop_idx);
        
        if (CLX_E_OK == rc)
        {
            ptr_tx_pdma->free_desc_num += ptr_tx_pdma->used_desc_num;
            ptr_tx_pdma->used_desc_num  = 0;
        }
    }

    return (rc);
}

CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_sendGpd(
    const UI32_T                    unit,
    const HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_T  channel,
          HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T    *ptr_sw_gpd)
{
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_PDMA_T           *ptr_tx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_TX_PDMA_PTR(unit, channel);
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T   *ptr_tx_gpd = NULL;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_first_gpd = ptr_sw_gpd;
    UI32_T                          work_idx = 0;
    UI32_T                          used_gpd_num = ptr_sw_gpd->desc_num;
    CLX_IRQ_FLAGS_T                 irq_flags;
    HAL_PKT_DRV_CB_T            *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);

    if(ptr_cb->init_stage != HAL_PKT_INIT_DONE)
    {
        OSAL_PRINT(OSAL_DBG_ERR,
                        "u=%u, send Gpd failed. init_stage=%d\n", unit, ptr_cb->init_stage);
        return CLX_E_OTHERS;
    }

    osal_takeIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);

    /* If not PDMA error */
    if (TRUE == ptr_tx_pdma->err_flag)
    {
        OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_TX),
                "u=%u, txch=%u, pdma hw err\n",
                unit, channel);
        osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);
        return CLX_E_OTHERS;
    }
    /* Make Sure descriptor is enough */
    if (ptr_tx_pdma->free_desc_num < used_gpd_num)
    {
        OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_TX),
                        "u=%u, txch=%u, pdma hw err\n",
                        unit, channel);
        osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);
        return CLX_E_OTHERS;
    }

    work_idx = _hal_mt_namchabarwa_pdma_get_tx_ch_work_idx(unit,channel);
    while (NULL != ptr_sw_gpd)
    {
        ptr_tx_gpd = HAL_MT_NAMCHABARWA_PKT_GET_TX_GPD_PTR(unit, channel, work_idx);
        osal_dma_invalidateCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

        if (1 == ptr_tx_gpd->interrupt)
        {
            OSAL_PRINT((OSAL_DBG_ERR | OSAL_DBG_TX),
                            "u=%u, txch=%u, free gpd idx out-of-sync\n",
                            unit, channel);
            rc = CLX_E_TABLE_FULL;
            break;
        }

        /* Fill in HW-GPD Ring */
        osal_memcpy((void *)ptr_tx_gpd, &ptr_sw_gpd->desc, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
        osal_dma_flushCache((void *)ptr_tx_gpd, sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

        /* next */
        work_idx++;
        work_idx %= ptr_tx_pdma->ring_size;
        ptr_sw_gpd = ptr_sw_gpd->ptr_next;
    }
    _hal_mt_namchabarwa_pdma_set_tx_ch_work_idx(unit,channel,work_idx);

    if (HAL_PKT_TX_WAIT_ASYNC == ptr_tx_cb->wait_mode)
    {
        /* Fill 1st GPD in SW-GPD Ring */
        ptr_tx_pdma->pptr_sw_gpd_ring[work_idx] = ptr_sw_first_gpd;
    }

    /* update Tx PDMA */
    ptr_tx_pdma->used_desc_num += used_gpd_num;
    ptr_tx_pdma->free_desc_num -= used_gpd_num;

    _hal_mt_namchabarwa_enable_pdma_tx_channel(unit, channel);
    ptr_tx_cb->cnt.channel[channel].send_ok++;

    _hal_mt_namchabarwa_pkt_waitTxDone(unit, channel, ptr_sw_first_gpd);

    /* reserve 1 packet buffer for each port in case that the suspension is too late */
#define HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW      (HAL_MT_NAMCHABARWA_PORT_NUM)
    if (ptr_tx_pdma->free_desc_num < HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW)
    {
        OSAL_PRINT(OSAL_DBG_TX,
                        "u=%u, txch=%u, tx avbl gpd < %d, suspend all netdev\n",
                        unit, channel, HAL_MT_NAMCHABARWA_PKT_KNL_TX_RING_AVBL_GPD_LOW);
        hal_pkt_suspendAllIntf(unit);
    }

    osal_giveIsrLock(&ptr_tx_pdma->ring_lock, &irq_flags);

    return (rc);
}


static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_prepare_descriptor(
    const UI32_T                unit,
    const CLX_ADDR_T            phy_addr,
    const struct sk_buff        *ptr_skb,
    const UI32_T                port,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T     *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PP_HDR_T             *ptr_pph)
{
    /* fill up tx_gpd */
    ptr_sw_gpd->desc.s_addr_hi              = CLX_ADDR_64_HI(phy_addr);
    ptr_sw_gpd->desc.s_addr_lo              = CLX_ADDR_64_LOW(phy_addr);
    ptr_sw_gpd->desc.size                   = ptr_skb->len;
    ptr_sw_gpd->desc.interrupt                        = 0;
    ptr_sw_gpd->desc.sop                           = 1; 
    ptr_sw_gpd->desc.eop                           = 1;
    ptr_sw_gpd->desc.sinc                           = 1;

    /* fill up pp header */
    ptr_pph->skip_ipp             = 1;
    ptr_pph->skip_epp             = 1;
    ptr_pph->color                = 0;   /* Green */
    ptr_pph->tc                   = 7;   /* Max tc */
    ptr_pph->src_idx              = 0;
    ptr_pph->dst_idx              = port;

    // TODO: fill up pph other fields

    return (CLX_E_OK);
}

static void
_hal_mt_namchabarwa_pkt_net_dev_tx_callback(
    const UI32_T                unit,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T     *ptr_sw_gpd,
    struct sk_buff              *ptr_skb)
{
    CLX_ADDR_T                  phy_addr = 0;
    void                        *ptr_virt_addr = NULL;
    /* unmap dma */
    phy_addr = CLX_ADDR_32_TO_64(ptr_sw_gpd->desc.s_addr_hi, ptr_sw_gpd->desc.s_addr_lo);
    ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);

    /* free descriptor buffer */
    osal_dma_free(ptr_virt_addr);

    /* free gpd */
    osal_free(ptr_sw_gpd);
}

static netdev_tx_t
_hal_mt_namchabarwa_pkt_net_dev_tx(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev)
{
    struct net_device_priv      *ptr_priv = netdev_priv(ptr_net_dev);
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T   *ptr_tx_cb;
    /* chip meta */
    unsigned int                unit;
    unsigned int                channel        = 0;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T     *ptr_sw_gpd    = NULL;
    void                        *ptr_virt_addr = NULL;
    CLX_ADDR_T                  phy_addr       = 0x0;

    if (NULL == ptr_priv)
    {
        /* in case that the netdev has been freed/reset somewhere */
        OSAL_PRINT(OSAL_DBG_ERR, "get netdev_priv failed\n");
        return -EFAULT;
    }

    /* check skb */
    if (NULL == ptr_skb)
    {
        ptr_priv->stats.tx_errors++;
        return -EFAULT;
    }

    unit = ptr_priv->unit;

    ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);

    /* for warm de-init procedure, if any net intf not destroyed, it is possible
     * that kernel still has packets to send causing segmentation fault
     */
    if (FALSE == ptr_tx_cb->net_tx_allowed) {
        OSAL_PRINT(OSAL_DBG_ERR, "net tx during sdk de-init\n");
        ptr_priv->stats.tx_dropped++;
        osal_skb_free(ptr_skb);
        return NETDEV_TX_OK;
    }

    /* pad to 60-bytes if skb_len < 60, see: eth_skb_pad(skb) */
    if (ptr_skb->len < ETH_ZLEN)
    {
        skb_pad(ptr_skb, ETH_ZLEN - ptr_skb->len);
        skb_set_tail_pointer(ptr_skb, ETH_ZLEN);
        ptr_skb->len = ETH_ZLEN;
    }

    /* pad 4-bytes for chip-crc */
    skb_pad(ptr_skb, ETH_FCS_LEN);
    skb_set_tail_pointer(ptr_skb, ETH_FCS_LEN);
    ptr_skb->len += ETH_FCS_LEN;

    /* alloc gpd */
    ptr_sw_gpd = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
    if (NULL == ptr_sw_gpd)
    {
        ptr_priv->stats.tx_errors++;
        osal_skb_free(ptr_skb);
    }
    else
    {
        // copy ptr_skb->data to DMA buffer
        ptr_virt_addr = osal_dma_alloc(ptr_skb->len + sizeof(HAL_MT_NAMCHABARWA_PP_HDR_T));
        phy_addr = osal_dma_convertVirtToPhy(ptr_virt_addr);
        memcpy(ptr_virt_addr + sizeof(HAL_MT_NAMCHABARWA_PP_HDR_T),ptr_skb->data,sizeof(ptr_skb->len));

        if (0x0 == phy_addr)
        {
            OSAL_PRINT(OSAL_DBG_ERR, "u=%u, txch=%u, skb dma map err\n",
                            unit, channel);
            ptr_priv->stats.tx_errors++;
            osal_skb_free(ptr_skb);
            osal_free(ptr_sw_gpd);
        }
        else
        {
            /* trans skb to gpd */
            memset(ptr_sw_gpd, 0x0, sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
            ptr_sw_gpd->callback   = (void *)_hal_mt_namchabarwa_pkt_net_dev_tx_callback;
            ptr_sw_gpd->ptr_cookie = (void *)ptr_skb;
            ptr_sw_gpd->desc_num    = 1;
            ptr_sw_gpd->ptr_next   = NULL;
            ptr_sw_gpd->channel    = channel;
            /* prepare gpd */
            hal_mt_namchabarwa_pkt_prepare_descriptor(unit, phy_addr, ptr_skb, ptr_priv->port, ptr_sw_gpd,(HAL_MT_NAMCHABARWA_PP_HDR_T*)ptr_virt_addr);

#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,6,7)
            ptr_net_dev->trans_start = jiffies;
#else
            netdev_get_tx_queue(ptr_net_dev, 0)->trans_start = jiffies;
#endif
            /* send gpd */
            if (CLX_E_OK == hal_mt_namchabarwa_pkt_sendGpd(unit, channel, ptr_sw_gpd))
            {
                ptr_priv->stats.tx_packets++;
                ptr_priv->stats.tx_bytes += ptr_skb->len;
            }
            else
            {
                ptr_priv->stats.tx_fifo_errors++;   /* to record the extreme cases where packets are dropped */
                ptr_priv->stats.tx_dropped++;

                osal_skb_free(ptr_skb);
                osal_free(ptr_sw_gpd);
            }
        }
    }

    return NETDEV_TX_OK;
}

static void
_hal_mt_namchabarwa_pkt_dev_tx_callback(
    const UI32_T                    unit,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T          *ptr_sw_gpd,
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T          *ptr_sw_gpd_usr)
{
    UI32_T                          channel = ptr_sw_gpd->channel;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);

    while (0 != _hal_mt_namchabarwa_pkt_enqueue(&ptr_tx_cb->sw_queue, ptr_sw_gpd))
    {
        ptr_tx_cb->cnt.channel[channel].enque_retry++;
        HAL_MT_NAMCHABARWA_PKT_TX_ENQUE_RETRY_SLEEP();
    }
    ptr_tx_cb->cnt.channel[channel].enque_ok++;

    osal_triggerEvent(&ptr_tx_cb->sync_sema);
    ptr_tx_cb->cnt.channel[channel].trig_event++;
}


ssize_t
_hal_mt_namchabarwa_pkt_dev_tx(
    struct file                     *file,
    const char __user               *buf,
    size_t                          count,
    loff_t                          *pos)
{
    int                             ret = 0;
    int                             idx = 0;
    unsigned int                    unit = 0;
    unsigned int                    channel = 0;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T    tx_cookie;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T       ioctl_gpd;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T          *ptr_sw_gpd_knl = NULL;
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T          *ptr_first_sw_gpd_knl = NULL;

    /* copy the tx-cookie */
    osal_io_copyFromUser(&tx_cookie, (void *)buf, sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T));

    unit    = tx_cookie.unit;
    channel = tx_cookie.channel;

    ptr_sw_gpd_knl = osal_alloc(sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
    ptr_first_sw_gpd_knl = ptr_sw_gpd_knl;

    /* create SW GPD based on the content of each IOCTL GPD */
    while (1)
    {
        osal_io_copyFromUser(&ioctl_gpd,
                             ((void *)((CLX_HUGE_T)tx_cookie.ioctl_gpd_addr))
                                 +idx*sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T),
                             sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_GPD_T));

        ptr_sw_gpd_knl->channel = ioctl_gpd.channel;
        ptr_sw_gpd_knl->desc_num = ioctl_gpd.gpd_num;
        ptr_sw_gpd_knl->ptr_cookie = (void *)ioctl_gpd.cookie;

        /* directly copy user's HW GPD */
        osal_io_copyFromUser(&ptr_sw_gpd_knl->desc,
                             (void *)((CLX_HUGE_T)ioctl_gpd.hw_gpd_addr),
                             sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));

        /* replace the callback */
        ptr_sw_gpd_knl->callback = (void *)_hal_mt_namchabarwa_pkt_dev_tx_callback;

        /* save the first SW GPD address from userspace since
         * we have replaced the original callback
         */
        ptr_sw_gpd_knl->ptr_cookie = (void *)ioctl_gpd.sw_gpd_addr;

        if (1 == ptr_sw_gpd_knl->desc.eop)
        {
            ptr_sw_gpd_knl->ptr_next = NULL;
            break;
        }
        else
        {
            ptr_sw_gpd_knl->ptr_next = (HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T *)osal_alloc(
                                            sizeof(HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T));
            ptr_sw_gpd_knl = ptr_sw_gpd_knl->ptr_next;
            idx++;
        }
    }

    ret = hal_mt_namchabarwa_pkt_sendGpd(unit, channel, ptr_first_sw_gpd_knl);
    if (CLX_E_OK != ret)
    {
        _hal_mt_namchabarwa_pkt_freeTxGpdList(unit, ptr_first_sw_gpd_knl);
    }

    /* return 0 if success */
    return (ret);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_schedRxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T    ioctl_data;
    HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T      ioctl_gpd;
    HAL_MT_NAMCHABARWA_PKT_RX_CB_T             *ptr_rx_cb = HAL_MT_NAMCHABARWA_PKT_GET_RX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_gpd_knl = NULL;
    HAL_MT_NAMCHABARWA_PKT_RX_SW_DESC_T         *ptr_sw_first_gpd_knl = NULL;
    UI32_T                                que_cnt = 0;
    UI32_T                                queue   = 0;
    UI32_T                                idx     = 0;
    UI32_T                                gpd_idx = 0;
    /* copy Rx sw_gpd */
    volatile HAL_MT_NAMCHABARWA_PDMA_DESC_T   *ptr_desc = NULL;
    void                            *ptr_virt_addr = NULL;
    CLX_ADDR_T                      phy_addr = 0;
    UI32_T                          buf_len = 0;
    CLX_ERROR_NO_T                  rc = CLX_E_OK;

    /* normal process */
    if (TRUE == ptr_rx_cb->running)
    {
        /* get queue and count */
        for (idx = 0; idx < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; idx++)
        {
            /* to gurantee the opportunity where each queue can be handler */
            queue = ((ptr_rx_cb->deque_idx + idx) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
            _hal_mt_namchabarwa_pkt_get_queue_count(&ptr_rx_cb->sw_queue[queue], &que_cnt);
            if (que_cnt > 0)
            {
                ptr_rx_cb->deque_idx = ((queue + 1) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
                break;
            }
        }

        /* If all of the queues are empty, wait rxTask event */
        if (0 == que_cnt)
        {
            osal_waitEvent(&ptr_rx_cb->sync_sema);

            ptr_rx_cb->cnt.wait_event++;

            /* re-get queue and count */
            for (queue = 0; queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM; queue++)
            {
                _hal_mt_namchabarwa_pkt_get_queue_count(&ptr_rx_cb->sw_queue[queue], &que_cnt);
                if (que_cnt > 0)
                {
                    ptr_rx_cb->deque_idx = ((queue + 1) % HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM);
                    break;
                }
            }
        }

        /* deque */
        if ((que_cnt > 0) && (queue < HAL_MT_NAMCHABARWA_PKT_RX_QUEUE_NUM))
        {
            rc = _hal_mt_namchabarwa_pkt_dequeue(&ptr_rx_cb->sw_queue[queue], (void **)&ptr_sw_gpd_knl);
            if (CLX_E_OK == rc)
            {
                ptr_rx_cb->cnt.channel[queue].deque_ok++;
                ptr_sw_first_gpd_knl = ptr_sw_gpd_knl;

                osal_io_copyFromUser(&ioctl_data, ptr_cookie, sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_COOKIE_T));

                while (NULL != ptr_sw_gpd_knl)
                {
                    /* get the IOCTL GPD from user */
                    osal_io_copyFromUser(&ioctl_gpd,
                                         ((void *)((CLX_HUGE_T)ioctl_data.ioctl_gpd_addr))
                                             + gpd_idx*sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T),
                                         sizeof(HAL_MT_NAMCHABARWA_PKT_IOCTL_RX_GPD_T));

                    /* get knl buf addr */
                    ptr_desc = &ptr_sw_gpd_knl->desc;
                    phy_addr = CLX_ADDR_32_TO_64(ptr_desc->d_addr_hi, ptr_desc->d_addr_lo);
                    ptr_virt_addr = osal_dma_convertPhyToVirt(phy_addr);

                    buf_len = ptr_desc->size;

                    /* overwrite whole rx_gpd to user
                     * the user should re-assign the correct value to data_buf_addr_hi, data_buf_addr_low
                     * after this IOCTL returns
                     */
                    osal_io_copyToUser((void *)((CLX_HUGE_T)ioctl_gpd.hw_gpd_addr),
                                       &ptr_sw_gpd_knl->desc,
                                       sizeof(HAL_MT_NAMCHABARWA_PDMA_DESC_T));
                    /* copy buf */
                    /* DMA buf address allocated by the user is store in ptr_ioctl_data->gpd[idx].cookie */
                    osal_io_copyToUser((void *)((CLX_HUGE_T)ioctl_gpd.dma_buf_addr),
                                       ptr_virt_addr, buf_len);

                    /* next */
                    ptr_sw_gpd_knl = ptr_sw_gpd_knl->ptr_next;
                    gpd_idx++;
                }

                /* Must free kernel sw_gpd */
                _hal_mt_namchabarwa_pkt_freeRxGpdList(unit, ptr_sw_first_gpd_knl, TRUE);
            }
            else
            {
                ptr_rx_cb->cnt.channel[queue].deque_fail++;
            }
        }
        else
        {
            /* it means that all queue's are flush -> rx stop flow */
            rc = CLX_E_OTHERS;
        }
    }

    return (rc);
}


static CLX_ERROR_NO_T
_hal_mt_namchabarwa_pkt_strictTxDeQueue(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_MT_NAMCHABARWA_PKT_IOCTL_TX_COOKIE_T   *ptr_cookie = ptr_data;
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_MT_NAMCHABARWA_PKT_TX_CB_T             *ptr_tx_cb = HAL_MT_NAMCHABARWA_PKT_GET_TX_CB_PTR(unit);
    HAL_MT_NAMCHABARWA_PKT_TX_SW_GPD_T         *ptr_sw_gpd = NULL;
    CLX_ADDR_T                      sw_gpd_addr;
    UI32_T                          que_cnt = 0;

    /* get queue count */
    _hal_mt_namchabarwa_pkt_get_queue_count(&ptr_tx_cb->sw_queue, &que_cnt);

    /* wait txTask event */
    if (0 == que_cnt)
    {
        osal_waitEvent(&ptr_tx_cb->sync_sema);
        if (FALSE == ptr_tx_cb->running)
        {
            return (CLX_E_OTHERS); /* deinit */
        }

        ptr_tx_cb->cnt.wait_event++;

        /* re-get queue count */
        _hal_mt_namchabarwa_pkt_get_queue_count(&ptr_tx_cb->sw_queue, &que_cnt);
    }

    /* deque */
    if (que_cnt > 0)
    {
        rc = _hal_mt_namchabarwa_pkt_dequeue(&ptr_tx_cb->sw_queue, (void **)&ptr_sw_gpd);
        if (CLX_E_OK == rc)
        {
            ptr_tx_cb->cnt.deque_ok++;

            sw_gpd_addr = (CLX_ADDR_T)ptr_sw_gpd->ptr_cookie;

            /* Give the address of pre-saved SW GPD back to userspace */
            osal_io_copyToUser(&ptr_cookie->done_sw_gpd_addr,
                               &sw_gpd_addr,
                               sizeof(CLX_ADDR_T));

            /* free kernel sw_gpd */
            _hal_mt_namchabarwa_pkt_freeTxGpdList(unit, ptr_sw_gpd);
        }
        else
        {
            ptr_tx_cb->cnt.deque_fail++;
        }
    }
    else
    {
        /* It may happen at last gpd, return error and do not invoke callback. */
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_lockRxChannelAll(
    const UI32_T                    unit,
    void                        *ptr_data)
{
    UI32_T                          rch;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma;

    for (rch = 0; rch < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_takeSemaphore(&ptr_rx_pdma->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    }
    return CLX_E_OK;
}

static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_unlockRxChannelAll(
    const UI32_T                    unit,
    void                        *ptr_data)
{
    UI32_T                          rch;
    HAL_MT_NAMCHABARWA_PKT_RX_PDMA_T            *ptr_rx_pdma;

    for (rch = 0; rch < HAL_MT_NAMCHABARWA_PDMA_TX_CHANNEL_LAST; rch++)
    {
        ptr_rx_pdma = HAL_MT_NAMCHABARWA_PKT_GET_RX_PDMA_PTR(unit, rch);
        osal_giveSemaphore(&ptr_rx_pdma->sema);
    }
    return CLX_E_OK;
}


static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_getRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}
static CLX_ERROR_NO_T
hal_mt_namchabarwa_pkt_clearRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}


CLX_ERROR_NO_T
hal_mt_namchabarwa_register_netif_ioctl(void)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* driver */
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_RX_FREE,
        _hal_mt_namchabarwa_pkt_schedRxDeQueue);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE,
        _hal_mt_namchabarwa_pkt_strictTxDeQueue);

    /* counter */
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
        hal_mt_namchabarwa_pkt_getTxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
        hal_mt_namchabarwa_pkt_getRxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
        hal_mt_namchabarwa_pkt_clearTxKnlCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
        hal_mt_namchabarwa_pkt_clearRxKnlCnt);

    return rc;
}

/*
    SDK Tx pkt flow
    1.  SDK write clx_dev_fd to send pkt
    2.  ptr_cb->pkt_dev_tx
    2.1 register tx_done callback
    2.2 send pkt:set work_idx
    3.  _hal_mt_namchabarwa_pkt_handleTxDoneTask
    3.1 enqueue tx_bulk
    4.  SDK free descriptor buffer via blocking-ioctl cmd:OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE
    4.1 dequeue tx_bulk in _hal_mt_namchabarwa_pkt_strictTxDeQueue
    4.2 SDK free descriptor buffer
*/
/*
    SDK Rx pkt flow
*/
void hal_mt_namchabarwa_register_drv_cb(
    const UI32_T unit)
{
    HAL_PKT_DRV_CB_T            *ptr_cb = HAL_MT_NAMCHABARWA_PKT_GET_DRV_CB_PTR(unit);

    osal_memset(_hal_mt_namchabarwa_pkt_tx_cb, 0x0,
                CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_MT_NAMCHABARWA_PKT_TX_CB_T));
    osal_memset(_hal_mt_namchabarwa_pkt_rx_cb, 0x0,
                CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM*sizeof(HAL_MT_NAMCHABARWA_PKT_RX_CB_T));

    ptr_cb->pkt_init_task = hal_mt_namchabarwa_pkt_initTask;
    ptr_cb->pkt_deinit_task = hal_mt_namchabarwa_pkt_deinit_task;
    ptr_cb->pkt_rx_stop = hal_mt_namchabarwa_pkt_rx_stop;
    ptr_cb->pkt_rx_start = _hal_mt_namchabarwa_pkt_rx_start;
    ptr_cb->pkt_deinit_drv = hal_mt_namchabarwa_pkt_deinit_pkt_drv;
    ptr_cb->pkt_init_drv = hal_mt_namchabarwa_init_drv;
    ptr_cb->pkt_init_irq = NULL;
    ptr_cb->pkt_deinit_irq = NULL;

    ptr_cb->net_dev_tx = _hal_mt_namchabarwa_pkt_net_dev_tx;
    ptr_cb->pkt_dev_tx = _hal_mt_namchabarwa_pkt_dev_tx;
    ptr_cb->lock_all_rx_channel = hal_mt_namchabarwa_pkt_lockRxChannelAll;
    ptr_cb->unlock_all_rx_channel = hal_mt_namchabarwa_pkt_unlockRxChannelAll;

    hal_mt_namchabarwa_register_netif_ioctl();
}