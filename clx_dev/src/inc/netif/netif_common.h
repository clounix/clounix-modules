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

/* FILE NAME:  hal_nb_pkt_knl.h
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 */

#ifndef HAL_PKT_KNL_H
#define HAL_PKT_KNL_H

#include <clx_error.h>
#include <netif/netif_knl.h>


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

typedef enum
{
    HAL_PKT_TX_WAIT_ASYNC     = 0,
    HAL_PKT_TX_WAIT_SYNC_INTR = 1,
    HAL_PKT_TX_WAIT_SYNC_POLL = 2

} HAL_PKT_TX_WAIT_T;

typedef enum
{
    HAL_PKT_RX_SCHED_RR       = 0,
    HAL_PKT_RX_SCHED_WRR      = 1

} HAL_PKT_RX_SCHED_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    UI32_T                      port;       /* only support unit port and local port */

    /* metadata */
    UI8_T                       mac[6];

#define HAL_PKT_NETIF_INTF_FLAGS_MAC        (1UL << 0)
    UI32_T                      flags;


} HAL_PKT_NETIF_INTF_T;

typedef struct
{
    C8_T                        name[CLX_NETIF_NAME_LEN];
    C8_T                        mc_group_name[CLX_NETIF_NAME_LEN];
} HAL_PKT_NETIF_RX_DST_NETLINK_T;

typedef enum
{
    HAL_PKT_NETIF_RX_DST_SDK = 0,

    HAL_PKT_NETIF_RX_DST_NETLINK,

    HAL_PKT_NETIF_RX_DST_LAST
} HAL_PKT_NETIF_RX_DST_TYPE_T;

typedef struct
{
    /* unique key */
    UI32_T                              id;
    C8_T                                name[CLX_NETIF_NAME_LEN];
    UI32_T                              priority;

    /* match fields */
    UI32_T                              port;     /* only support unit port and local port */
    HAL_PKT_RX_REASON_BITMAP_T          reason_bitmap;
    UI8_T                               pattern[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI8_T                               mask[CLX_NETIF_PROFILE_PATTERN_NUM][CLX_NETIF_PROFILE_PATTERN_LEN];
    UI32_T                              offset[CLX_NETIF_PROFILE_PATTERN_NUM];

    /* for each flag 1:must hit, 0:don't care */
#define HAL_PKT_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define HAL_PKT_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define HAL_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define HAL_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define HAL_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define HAL_PKT_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T                              flags;

    HAL_PKT_NETIF_RX_DST_TYPE_T     dst_type;

    HAL_PKT_NETIF_RX_DST_NETLINK_T  netlink;

} HAL_PKT_NETIF_PROFILE_T;

typedef struct
{
    UI32_T          tx_pkt;
    UI32_T          tx_queue_full;
    UI32_T          tx_error;
    UI32_T          rx_pkt;

} HAL_PKT_NETIF_INTF_CNT_T;

typedef struct
{
    UI32_T                          unit;
    HAL_PKT_NETIF_INTF_T         net_intf;       /* addIntf[In,Out], delIntf[In]              */
    HAL_PKT_NETIF_PROFILE_T      net_profile;    /* createProfile[In,Out], destroyProfile[In] */
    HAL_PKT_NETIF_INTF_CNT_T     cnt;
    CLX_ERROR_NO_T                  rc;

} HAL_PKT_IOCTL_NETIF_COOKIE_T;


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


typedef struct
{
    UI32_T                          port;
    UI32_T                          status;
    CLX_PORT_SPEED_T                speed;

} HAL_PKT_IOCTL_PORT_COOKIE_T;

typedef struct
{
    /* intf property */
    UI32_T                          intf_id;
    CLX_NETIF_INTF_PROPERTY_T       property;
    UI32_T                          param0;
    UI32_T                          param1;

    /* netlink */
    CLX_NETIF_NETLINK_T             netlink;

    CLX_ERROR_NO_T                  rc;

} HAL_PKT_NL_IOCTL_COOKIE_T;

typedef struct
{
    CLX_HUGE_T                      que_id;
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          len;      /* Software CPU queue maximum length.        */
    UI32_T                          weight;   /* The weight for thread de-queue algorithm. */

} HAL_PKT_SW_QUEUE_T;

typedef CLX_ERROR_NO_T
(*NETIF_CALLBACK_FUNC_T)(
    const UI32_T        unit);
    
typedef ssize_t
(*PKT_DEV_TX)(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos);

#include <linux/netdevice.h>
typedef netdev_tx_t
(*NET_DEV_TX)(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev);

typedef enum
{
    HAL_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_PKT_IOCTL_RX_TYPE_LAST,

} HAL_PKT_IOCTL_RX_TYPE_T;
typedef struct
{
    UI32_T                              unit;
    UI32_T                              channel;            /* getRxCnt[In], clearRxInt[In]     */
    CLX_ADDR_T                          ioctl_gpd_addr;     /* waitRxFree[Out]                  */
    UI32_T                              buf_len;            /* setRxCfg[In]                     */
    HAL_PKT_IOCTL_RX_TYPE_T             rx_type;            /* setRxCfg[In]                     */

} HAL_PKT_IOCTL_RX_COOKIE_T;

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
_hal_pkt_resumeAllIntf(
    const UI32_T                        unit);

CLX_ERROR_NO_T
_hal_pkt_suspendAllIntf(
    const UI32_T                        unit);

CLX_ERROR_NO_T
_hal_pkt_stopAllIntf(
    const UI32_T                        unit);

CLX_ERROR_NO_T
hal_register_netif_common_ioctl(void);

#endif /* end of HAL_PKT_KNL_H */
