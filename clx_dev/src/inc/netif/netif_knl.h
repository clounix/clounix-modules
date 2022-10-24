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

/* FILE NAME:   netif/netif_knl.h
 * NOTES:
 */
#ifndef NETIF_KNL_H
#define NETIF_KNL_H

#include <clx_types.h>
#include <clx_knl.h>
#include <hal/hal_pkt_rsrc_knl.h>
#include <hal/hal_netif.h>
#include <linux/netdevice.h>

#define HAL_PKT_NET_PROFILE_NUM_MAX         (256)


extern UI32_T     verbosity; 

#define HAL_INVALID_GROUP_LABEL           (0)
#define HAL_INVALID_NVO3_ENCAP_IDX        (0x3FFF)
#define HAL_INVALID_NVO3_ADJ_IDX          (0xFF)
#define HAL_INVALID_LCL_INTF_GRP          (0x1FFF)
#define HAL_INVALID_CNT_MTR_IDX           (0x7FFF)
#define HAL_INVALID_FDID                  (0)       /* means fdid is not created                         */
#define HAL_INVALID_L3_INTF               (0)       /* means L3 interface is disabled                    */
#define HAL_INVALID_FRR_STATE_IDX         (0)       /* means ECMP FRR is disabled                        */
#define HAL_INVALID_SEG_VMID              (0xFFFFFF)
#define HAL_INVALID_CPU_ID                (0x1F)
#define HAL_INVALID_DOS_IDX               (0xF)
#define HAL_INVALID_HW_BUM_OFFSET         (0x3)
#define HAL_INVALID_IEV_RSLT_IDX          (0x3FFFF)
#define HAL_INVALID_PHB_HW_IDX            (0x1F)    /* SRV INTF uses invalid index                       */
#define HAL_DEFAULT_PHB_HW_IDX            (0x1F)    /* LCL INTF uses default index                       */
#define HAL_RSV_FDID                      (0x3FFF)  /* HW reserved; all 1 means use outer vid as FDID    */
#define HAL_RSV_L3_INTF                   (0x3FFF)  /* HW reserved; all 1 means use outer vid as L3 Intf */
                                                    /*              all 1 means invalid at egress side   */
#define HAL_INVALID_ADJ_IDX               (0x3FFFF) /* For 18bit adj_idx                                 */
#define HAL_INVALID_MTR_HW_IDX            (0x1FFFF)
#define HAL_RSV_MEL_IDX                   (0x1FFF)

#define HAL_LAG_PORT_NUM                  (512)
#define HAL_L2_MGID_NUM                   (16384)   /* non-replicate mgid */
#define HAL_L3_MGID_NUM                   (8192)    /* replicate mgid */
#define HAL_MGID_NUM                      (HAL_L2_MGID_NUM + HAL_L3_MGID_NUM)
#define HAL_EXCPT_CPU_NUM                 (256)
#define HAL_EXCPT_DROP_NUM                (256)
#define HAL_DROP_NUM                      (512)
#define HAL_MIRROR_NUM                    (256)
#define HAL_REDIRECT_CPU_NUM              (2048)
#define HAL_TUNNEL_NUM                    (8192)
#define HAL_NSH_NUM                       (8192)
#define HAL_FRR_NUM                       (4096)
#define HAL_ECMP_NUM                      (2048)
#define HAL_INVALID_NUM                   (10240)

#define HAL_EXCPT_CPU_BASE_ID             (28 * 1024)
#define HAL_EXCPT_CPU_NON_L3_MIN          (0)
#define HAL_EXCPT_CPU_NON_L3_MAX          (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM - 1)
#define HAL_EXCPT_CPU_L3_MIN              (HAL_EXCPT_CPU_NON_L3_MIN + HAL_EXCPT_CPU_NUM)
#define HAL_EXCPT_CPU_L3_MAX              (HAL_EXCPT_CPU_L3_MIN     + HAL_EXCPT_CPU_NUM - 1)



/* PKT definitions */
#define HAL_PKT_TX_MAX_LEN              (9216)
#define HAL_PKT_RX_MAX_LEN              (9216 + 86) /* EPP tunnel header */
#define HAL_PKT_MIN_LEN                 (64)        /* Ethernet definition */

/* Network Device Definitions */
/* In case that the watchdog alarm during warm-boot if intf isn't killed */
#define HAL_PORT_NUM                        (256)
#define HAL_PKT_TX_TIMEOUT                  (30*HZ)
#define HAL_PKT_MAX_ETH_FRAME_SIZE          (HAL_PKT_RX_MAX_LEN)
#define HAL_PKT_MAX_PORT_NUM                (HAL_PORT_NUM + 1)


typedef struct HAL_PKT_PROFILE_NODE_S
{
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile;
    struct HAL_PKT_PROFILE_NODE_S   *ptr_next_node;

} HAL_PKT_PROFILE_NODE_T;

typedef struct
{
    HAL_PKT_NETIF_INTF_T            meta;
    struct net_device                   *ptr_net_dev;
    HAL_PKT_PROFILE_NODE_T          *ptr_profile_list;  /* the profiles binding to this interface */

} HAL_PKT_NETIF_PORT_DB_T;



struct net_device_priv
{
    struct net_device               *ptr_net_dev;
    struct net_device_stats         stats;
    UI32_T                          unit;
    UI32_T                          id;
    UI32_T                          port;
    UI16_T                          vlan;
    UI32_T                          speed;
};

typedef CLX_ERROR_NO_T
(*NETIF_CALLBACK_FUNC_T)(
    const UI32_T        unit,
    void                *ptr_data);
    
typedef ssize_t
(*PKT_DEV_TX)(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos);

typedef netdev_tx_t
(*NET_DEV_TX)(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev);


typedef enum{
    HAL_PKT_INIT_START = 0,
    HAL_PKT_INIT_DRV ,
    HAL_PKT_INIT_TASK,
    HAL_PKT_INIT_INTR,
    HAL_PKT_INIT_RX_START,
    HAL_PKT_INIT_DONE = HAL_PKT_INIT_RX_START
} HAL_NETIF_INIT_STAGE_E;

typedef struct
{
    /* Rx system configuration */
    UI32_T                          buf_len;

    NETIF_CALLBACK_FUNC_T           pkt_init_task;
    NETIF_CALLBACK_FUNC_T           pkt_deinit_task;
    NETIF_CALLBACK_FUNC_T           pkt_rx_stop;
    NETIF_CALLBACK_FUNC_T           pkt_rx_start;
    NETIF_CALLBACK_FUNC_T           pkt_deinit_drv;
    NETIF_CALLBACK_FUNC_T           pkt_init_drv;
    NETIF_CALLBACK_FUNC_T           pkt_init_irq;
    NETIF_CALLBACK_FUNC_T           pkt_deinit_irq;
    NETIF_CALLBACK_FUNC_T           lock_all_rx_channel;
    NETIF_CALLBACK_FUNC_T           unlock_all_rx_channel;
    NET_DEV_TX                      net_dev_tx;
    PKT_DEV_TX                      pkt_dev_tx;

    /* INTR dispatcher */
    CLX_ISRLOCK_ID_T                intr_lock;
    UI32_T                          intr_bitmap;
    
    /* a bitmap to record the init status */
    HAL_NETIF_INIT_STAGE_E          init_stage;

} HAL_PKT_DRV_CB_T;

/* Declaration */
CLX_ERROR_NO_T
hal_netif_pkt_init(
    const UI32_T            unit);

CLX_ERROR_NO_T
hal_netif_pkt_exit(
    const UI32_T                        unit);

ssize_t
hal_pkt_dev_tx(
    struct file                         *file,
    const char __user                   *buf,
    size_t                              count,
    loff_t                              *pos);

CLX_ERROR_NO_T
hal_pkt_resumeAllIntf(
    const UI32_T                        unit);

CLX_ERROR_NO_T
hal_pkt_suspendAllIntf(
    const UI32_T                        unit);

CLX_ERROR_NO_T
hal_pkt_stopAllIntf(
    const UI32_T                        unit);


#endif  /* End of NETIF_KNL_H */
