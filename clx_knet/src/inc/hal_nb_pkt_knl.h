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

#ifndef HAL_NB_PKT_KNL_H
#define HAL_NB_PKT_KNL_H

#include <clx_error.h>
#include <netif_knl.h>

#define COSIM_GET_BIT(flags, bit)             ((((flags) & (bit)) > 0)? 1 : 0)
#define COSIM_SET_BIT(bitmap, mask_bitmap) (bitmap = ((bitmap) | (mask_bitmap)))
#define COSIM_CLEAR_BIT(bitmap, mask_bitmap) (bitmap = ((bitmap) & (~(mask_bitmap))))

/*PDMA reg definition*/
#define HAL_NB_PDMA_BASE_ADDR                               (0x51c1400)
#define HAL_NB_PDMA_GET_MMIO(__offset__)                    (HAL_NB_PDMA_BASE_ADDR + (__offset__))

#define HAL_NB_PDMA_CFG_CH_ENABLE                           (0x4)
#define HAL_NB_PDMA_CFG_DESC_LOCATION                       (0x8)
#define HAL_NB_PDMA_CFG_DESC_ENDIAN                         (0xC)
#define HAL_NB_PDMA_CFG_DATA_ENDIAN                         (0x10)
#define HAL_NB_PDMA_CFG_FIFIO_PATH_SEL                      (0x44)
#define HAL_NB_PDMA_CFG_CRC_EN                              (0x48)
#define HAL_NB_PDMA_CFG_P2H_RX_FIFO_ALM_FULL                (0x4C)
#define HAL_NB_PDMA_CFG_P2H_TX_FIFO_ALM_FULL                (0x50)
#define HAL_NB_PDMA_CFG_P2E_RX_FIFO_ALM_FULL                (0x54)
#define HAL_NB_PDMA_CFG_P2E_TX_FIFO_ALM_FULL                (0x58)
#define HAL_NB_PDMA_CFG_FIFO_DEBUG_EN                       (0x64)
#define HAL_NB_PDMA_CFG_CH0_RING_BASE                       (0xD0)
#define HAL_NB_PDMA_CFG_CH0_RING_SIZE                       (0x170)
#define HAL_NB_PDMA_CFG_CH0_DESC_WORK_IDX                   (0x1C0)
#define HAL_NB_PDMA_CFG_CH0_DESC_POP_IDX                    (0x210)
#define HAL_NB_PDMA_CFG_CH0_MODE                            (0x260)
#define HAL_NB_PDMA_CFW_GLOBAL_RESET                        (0x360)
#define HAL_NB_PDMA_CFW_CHANNEL_RESET                       (0x364)
#define HAL_NB_PDMA_CFW_CHANNEL_RESTART                     (0x368)
#define HAL_NB_PDMA_IRQ_ABNORMAL_INTR                       (0x36c)

#define HAL_NB_GET_PDMA_CH_RING_BASE_REG(__channel__)       (HAL_NB_PDMA_CFG_CH0_RING_BASE + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_RING_SIZE_REG(__channel__)       (HAL_NB_PDMA_CFG_CH0_RING_SIZE + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(__channel__)   (HAL_NB_PDMA_CFG_CH0_DESC_WORK_IDX + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(__channel__)    (HAL_NB_PDMA_CFG_CH0_DESC_POP_IDX + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MODE(__channel__)                (HAL_NB_PDMA_CFG_CH0_MODE + (0x4 * (__channel__)))


/*enable pdma channel*/
#define HAL_NB_PDMA_ENABLE_CHANNEL                  0x1
#define HAL_NB_PDMA_DISABLE_CHANNEL                 0x0

typedef enum {
    HAL_NB_PDMA_CH_MODE_HOSTMEM_TO_HOSTMEM   = 0,
    HAL_NB_PDMA_CH_MODE_LOCALBUS_TO_HOSTMEM  = 1,
    HAL_NB_PDMA_CH_MODE_HOSTMEM_TO_LOCALBUS  = 2,
    HAL_NB_PDMA_CH_MODE_LOCALBUS_TO_LOCALBUS = 3
} HAL_NB_PDMA_CH_MODE;

typedef enum {
    NB_DMA_ACCESS_INDIRECT = 0,
    NB_DMA_ACCESS_DIRECT = 1
} HAL_NB_DMA_ACCESS_MODE;

/*select pdma byte endian*/
#define HAL_NB_PDMA_BYTE_SWAP_DISABLE          0x0
#define HAL_NB_PDMA_BYTE_SWAP_ENABLE           0x1

/***************************************************************************************/

/*
 * descriptor
    63---------------47-----------------------------------------------0
    |       size     |                  s_addr                        |
    |      [63:48]   |                  [47:0]                        |
0x8  ----------------|------------------------------------------------|
    |      status    |                  d_addr                        |
    |      [63:48]   |                  [47:0]                        |                                               
0x10 ----------------|------------------------------------------------|

*/
typedef struct
{
    UI64_T  s_addr              : 48;   
    UI64_T  size                : 16;
    UI64_T  d_addr              : 48;

    /*status[127:112]*/
    UI64_T  interrupt           :  1;
    UI64_T  err                 :  1;
    UI64_T  eop                 :  1;
    UI64_T  sop                 :  1;
    UI64_T  sinc                :  1;
    UI64_T  dinc                :  1;
    UI64_T  xfer_size           :  5;
    UI64_T  limit_xfer_en       :  1;
    UI64_T  reserve             :  4;

} HAL_NB_PDMA_DESC_T;


typedef enum
{
    HAL_NB_PDMA_RX_CHANNEL_0 = 0,
    HAL_NB_PDMA_RX_CHANNEL_1,
    HAL_NB_PDMA_RX_CHANNEL_2,
    HAL_NB_PDMA_RX_CHANNEL_3,
    HAL_NB_PDMA_RX_CHANNEL_LAST

} HAL_NB_PDMA_RX_CHANNEL_T;
typedef enum
{
    HAL_NB_PDMA_TX_CHANNEL_0 = 4,
    HAL_NB_PDMA_TX_CHANNEL_1,
    HAL_NB_PDMA_TX_CHANNEL_2,
    HAL_NB_PDMA_TX_CHANNEL_3,
    HAL_NB_PDMA_TX_CHANNEL_LAST

} HAL_NB_PDMA_TX_CHANNEL_T;

typedef enum
{
    HAL_NB_PDMA_GENERAL_CHANNEL_0 = 8,
    HAL_NB_PDMA_GENERAL_CHANNEL_1,
    HAL_NB_PDMA_GENERAL_CHANNEL_2,
    HAL_NB_PDMA_GENERAL_CHANNEL_3,
    HAL_NB_PDMA_GENERAL_CHANNEL_4,
    HAL_NB_PDMA_GENERAL_CHANNEL_5,
    HAL_NB_PDMA_GENERAL_CHANNEL_6,
    HAL_NB_PDMA_GENERAL_CHANNEL_7,
    HAL_NB_PDMA_GENERAL_CHANNEL_LAST

} HAL_NB_PDMA_GENERAL_CHANNEL_T;

#define HAL_NB_PDMA_L2_LEARNING_FIFO_CHANNEL 18
#define HAL_NB_PDMA_IOAM_FIFO_CHANNEL 19

#define HAL_NB_PDMA_GENERAL_RING_SIZE   (1024)

#define HAL_NB_PDMA_PKT_CHANNEL_NUM    (HAL_NB_PDMA_TX_CHANNEL_LAST) 

#if defined(CLX_EN_HOST_64_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_64_BIT_LITTLE_ENDIAN)
#define HAL_NB_PDMA_RING_BASE_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFFFFFFFFFF0)
#else
#define HAL_NB_PDMA_RING_BASE_ALIGN_ADDR(pdma_addr, align_sz) (((pdma_addr) + (align_sz)) & 0xFFFFFFF0)
#endif

#define HAL_NB_PDMA_DESC_DATA_MAX_LEN       (UI16_T)(65536)
#define HAL_NB_PDMA_DESC_DATA_LEN           (9216)

#define HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP   (10 * 1000) /* int */

#define HAL_NB_PCI_DEV_BUS_WIDTH           (32)

/*PPH struct*/
#if defined(CLX_EN_LITTLE_ENDIAN)

#pragma pack (1)
typedef struct 
{
    UI32_T  timestamp                   :32;
    UI32_T  int_role                    :2;
    UI32_T  int_profile                 :3;
    UI32_T  int_mm_mode                 :1;
    UI32_T  ptp_info                    :32;
    UI32_T                              :2;
    UI32_T  mac_learn_en                :1;
    UI32_T                              :32;
    UI32_T                              :22;
    UI32_T  tapping_push_t              :1;
    UI32_T  tapping_push_o              :1;
    UI32_T  src_vlan                    :12;
    UI32_T  pvlan_port_type             :2;
    UI32_T  igr_vid_pop_num             :3;
    UI32_T  ecn                         :2;
    UI32_T  ecn_enable                  :1;
    UI32_T  mpls_ctl                    :4;
    UI32_T  tnl_bd                      :9;
    UI32_T  tnl_idx                     :13;
    UI32_T  mpls_pwcw_vld               :1;
    UI32_T  evpn_esi                    :20;
    UI32_T  igr_is_fab                  :1;
    UI32_T  decap_act                   :3;
    UI32_T  src_bdi                     :14;
    UI32_T  cpu_reason                  :10;
    UI32_T  mirror_bmap                 :8;
    UI32_T  skip_ipp                    :1;
    UI32_T  slice_id                    :3;
    UI32_T  die_id                      :1;
    UI32_T  port_num                    :6;
    UI32_T  pkt_journal                 :1;
    UI32_T  qos_pcp_dei_val             :4;
    UI32_T  qos_tnl_uniform             :1;
    UI32_T  qos_dnt_modify              :1;
    UI32_T  igr_acl_label               :16;
    UI32_T  skip_epp                    :1;
    UI32_T  src_idx                     :14;
    UI32_T  dst_idx                     :16;
    UI32_T  hash_val                    :16;
    UI32_T  color                       :2;
    UI32_T  tc                          :3;
    UI32_T  fwd_op                      :2;

} HAL_NB_PP_HDR_T; //L2
#pragma pack ()

#elif defined(CLX_EN_BIG_ENDIAN)

#pragma pack (1)
typedef struct
{
    UI32_T  fwd_op                      :2;
    UI32_T  tc                          :3;
    UI32_T  color                       :2;
    UI32_T  hash_val                    :16;
    UI32_T  dst_idx                     :16;
    UI32_T  src_idx                     :14;
    UI32_T  skip_epp                    :1;
    UI32_T  igr_acl_label               :16;
    UI32_T  qos_dnt_modify              :1;
    UI32_T  qos_tnl_uniform             :1;
    UI32_T  qos_pcp_dei_val             :4;
    UI32_T  pkt_journal                 :1;
    UI32_T  port_num                    :6;
    UI32_T  die_id                      :1;
    UI32_T  slice_id                    :3;
    UI32_T  skip_ipp                    :1;
    UI32_T  mirror_bmap                 :8;
    UI32_T  cpu_reason                  :10;
    UI32_T  src_bdi                     :14;
    UI32_T  decap_act                   :3;
    UI32_T  igr_is_fab                  :1;
    UI32_T  evpn_esi                    :20;
    UI32_T  mpls_pwcw_vld               :1;
    UI32_T  tnl_idx                     :13;
    UI32_T  tnl_bd                      :9;
    UI32_T  mpls_ctl                    :4;
    UI32_T  ecn_enable                  :1;
    UI32_T  ecn                         :2;
    UI32_T  igr_vid_pop_num             :3;
    UI32_T  pvlan_port_type             :2;
    UI32_T  src_vlan                    :12;
    UI32_T  tapping_push_o              :1;
    UI32_T  tapping_push_t              :1;
    UI32_T                              :22;
    UI32_T                              :32;
    UI32_T  mac_learn_en                :1;
    UI32_T                              :2;
    UI32_T  ptp_info                    :32;
    UI32_T  int_mm_mode                 :1;
    UI32_T  int_profile                 :3;
    UI32_T  int_role                    :2;
    UI32_T  timestamp                   :32;

} HAL_NB_PP_HDR_T; //L2
#pragma pack ()
#else
#error "Host PDMA endian is not defined\n"
#endif


/* ----------------------------------------------------------------------------------- CLX_EN_NETIF */
#if defined (CLX_EN_NETIF)
#define HAL_NB_PKT_DRIVER_MAJOR_NUM    (10)
#define HAL_NB_PKT_DRIVER_MINOR_NUM    (252) /* DO NOT use MISC_DYNAMIC_MINOR */
#define HAL_NB_PKT_DRIVER_NAME         "clx_netif"
#define HAL_NB_PKT_DRIVER_PATH         "/dev/" HAL_NB_PKT_DRIVER_NAME


#define HAL_NB_PORT_NUM                    (256)


/* NAMING DECLARATIONS
 */
/* PKT definitions */
#define HAL_NB_PKT_PPH_HDR_SZ              (56)
#define HAL_NB_PKT_TX_MAX_LEN              (9216 + HAL_NB_PKT_PPH_HDR_SZ)
#define HAL_NB_PKT_RX_MAX_LEN              (9216 + HAL_NB_PKT_PPH_HDR_SZ)
#define HAL_NB_PKT_MIN_LEN                 (64)        /* Ethernet definition */
#define HAL_NB_PKT_CRC_LEN                 (4)



/*****************************************************************************
 * MACRO VLAUE DECLARATIONS
 *****************************************************************************
 */
/* Sleep Time Definitions */
#define HAL_NB_PKT_TX_DEQUE_SLEEP()            osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_RX_DEQUE_SLEEP()            osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_TX_ENQUE_RETRY_SLEEP()      osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_RX_ENQUE_RETRY_SLEEP()      osal_sleepThread(1000) /* us */
#define HAL_NB_PKT_ALLOC_MEM_RETRY_SLEEP()     osal_sleepThread(1000) /* us */

/* Network Device Definitions */
/* In case that the watchdog alarm during warm-boot if intf isn't killed */
#define HAL_NB_PKT_TX_TIMEOUT                  (30*HZ)
#define HAL_NB_PKT_MAX_ETH_FRAME_SIZE          (HAL_NB_PKT_RX_MAX_LEN)
#define HAL_NB_PKT_MAX_PORT_NUM                (HAL_NB_PORT_NUM + 1) /* CPU port */

#define HAL_NB_PKT_NET_PROFILE_NUM_MAX         (256)


#define HAL_NB_PKT_PORT_STATUS_UP          (1)
#define HAL_NB_PKT_PORT_STATUS_DOWN        (0)
#ifndef SPEED_400000
#define SPEED_400000 400000
#endif
#ifndef SPEED_200000
#define SPEED_200000 200000
#endif


/* These requirements come from CLX_NETIF APIs.
 * clx_netif -> hal_nb_pkt_drv -> hal_nb_pkt_knl
 */

typedef struct
{
    UI32_T          tx_pkt;
    UI32_T          tx_queue_full;
    UI32_T          tx_error;
    UI32_T          rx_pkt;

} HAL_NB_PKT_NETIF_INTF_CNT_T;

typedef struct
{
    /* unique key */
    UI32_T                      id;
    C8_T                        name[CLX_NETIF_NAME_LEN];
    UI32_T                      port;       /* only support unit port and local port */

    /* metadata */
    UI8_T                       mac[6];

#define HAL_NB_PKT_NETIF_INTF_FLAGS_MAC        (1UL << 0)
    UI32_T                      flags;


} HAL_NB_PKT_NETIF_INTF_T;

#if defined(NETIF_EN_NETLINK)
typedef struct
{
    C8_T                        name[CLX_NETIF_NAME_LEN];
    C8_T                        mc_group_name[CLX_NETIF_NAME_LEN];
} HAL_NB_PKT_NETIF_RX_DST_NETLINK_T;
#endif

typedef enum
{
    HAL_NB_PKT_NETIF_RX_DST_SDK = 0,
#if defined(NETIF_EN_NETLINK)
    HAL_NB_PKT_NETIF_RX_DST_NETLINK,
#endif
    HAL_NB_PKT_NETIF_RX_DST_LAST
} HAL_NB_PKT_NETIF_RX_DST_TYPE_T;

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
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_PORT      (1UL << 0)
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_REASON    (1UL << 1)
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_PATTERN_0 (1UL << 2)
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_PATTERN_1 (1UL << 3)
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_PATTERN_2 (1UL << 4)
#define HAL_NB_PKT_NETIF_PROFILE_FLAGS_PATTERN_3 (1UL << 5)
    UI32_T                              flags;

    HAL_NB_PKT_NETIF_RX_DST_TYPE_T     dst_type;
#if defined(NETIF_EN_NETLINK)
    HAL_NB_PKT_NETIF_RX_DST_NETLINK_T  netlink;
#endif

} HAL_NB_PKT_NETIF_PROFILE_T;


/* These requirements come from CLX_PKT APIs.
 * clx_pkt -> hal_nb_pkt_srv -> hal_nb_pkt_drv -> hal_nb_pkt_knl
 */
typedef enum
{
    /* network interface */
    HAL_NB_PKT_IOCTL_TYPE_CREATE_INTF = 0,
    HAL_NB_PKT_IOCTL_TYPE_DESTROY_INTF,
    HAL_NB_PKT_IOCTL_TYPE_GET_INTF,
    HAL_NB_PKT_IOCTL_TYPE_CREATE_PROFILE,
    HAL_NB_PKT_IOCTL_TYPE_DESTROY_PROFILE,
    HAL_NB_PKT_IOCTL_TYPE_GET_PROFILE,
    HAL_NB_PKT_IOCTL_TYPE_GET_INTF_CNT,
    HAL_NB_PKT_IOCTL_TYPE_CLEAR_INTF_CNT,
    /* driver */
    HAL_NB_PKT_IOCTL_TYPE_WAIT_RX_FREE,
    HAL_NB_PKT_IOCTL_TYPE_WAIT_TX_FREE,     /* waitTxFree(ASYNC) */
    HAL_NB_PKT_IOCTL_TYPE_SET_RX_CFG,       /* setRxConfig       */
    HAL_NB_PKT_IOCTL_TYPE_GET_RX_CFG,       /* getRxConfig       */
    HAL_NB_PKT_IOCTL_TYPE_DEINIT_TASK,      /* deinitTask        */
    HAL_NB_PKT_IOCTL_TYPE_DEINIT_DRV,       /* deinitDrv         */
    HAL_NB_PKT_IOCTL_TYPE_INIT_TASK,        /* initTask          */
    HAL_NB_PKT_IOCTL_TYPE_INIT_DRV,         /* initDrv           */
    /* counter */
    HAL_NB_PKT_IOCTL_TYPE_GET_TX_CNT,
    HAL_NB_PKT_IOCTL_TYPE_GET_RX_CNT,
    HAL_NB_PKT_IOCTL_TYPE_CLEAR_TX_CNT,
    HAL_NB_PKT_IOCTL_TYPE_CLEAR_RX_CNT,
    /* port attribute */
    HAL_NB_PKT_IOCTL_TYPE_SET_PORT_ATTR,
#if defined(NETIF_EN_NETLINK)
    HAL_NB_PKT_IOCTL_TYPE_NL_SET_INTF_PROPERTY,
    HAL_NB_PKT_IOCTL_TYPE_NL_GET_INTF_PROPERTY,
    HAL_NB_PKT_IOCTL_TYPE_NL_CREATE_NETLINK,
    HAL_NB_PKT_IOCTL_TYPE_NL_DESTROY_NETLINK,
    HAL_NB_PKT_IOCTL_TYPE_NL_GET_NETLINK,
#endif
    HAL_NB_PKT_IOCTL_TYPE_LAST

} HAL_NB_PKT_IOCTL_TYPE_T;

typedef enum
{
    HAL_NB_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_NB_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_NB_PKT_IOCTL_RX_TYPE_LAST,

} HAL_NB_PKT_IOCTL_RX_TYPE_T;

typedef struct
{
    UI32_T                          unit;
    HAL_NB_PKT_NETIF_INTF_T         net_intf;       /* addIntf[In,Out], delIntf[In]              */
    HAL_NB_PKT_NETIF_PROFILE_T      net_profile;    /* createProfile[In,Out], destroyProfile[In] */
    HAL_NB_PKT_NETIF_INTF_CNT_T     cnt;
    CLX_ERROR_NO_T                  rc;

} HAL_NB_PKT_IOCTL_NETIF_COOKIE_T;

typedef struct
{
    CLX_ADDR_T                      callback;       /* (unit, ptr_sw_gpd, ptr_cookie) */
    CLX_ADDR_T                      cookie;         /* Pointer of CLX_PKT_TX_PKT_T    */
    UI32_T                          channel;
    UI32_T                          gpd_num;
    CLX_ADDR_T                      hw_gpd_addr;
    CLX_ADDR_T                      sw_gpd_addr;

} HAL_NB_PKT_IOCTL_TX_GPD_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;            /* sendGpd[In]      */
    CLX_ADDR_T                      ioctl_gpd_addr;     /* sendGpd[In]      */
    CLX_ADDR_T                      done_sw_gpd_addr;   /* waitTxFree[Out]  */

} HAL_NB_PKT_IOCTL_TX_COOKIE_T;

typedef struct
{
    BOOL_T                          rx_complete;        /* FALSE when PDMA error occurs                 */
    CLX_ADDR_T                      hw_gpd_addr;        /* Pointer to HW GPD in user's SW GPD struct    */
    CLX_ADDR_T                      dma_buf_addr;       /* Pointer to DMA buffer allocated by the user (virtual) */

} HAL_NB_PKT_IOCTL_RX_GPD_T;

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;            /* getRxCnt[In], clearRxInt[In]     */
    CLX_ADDR_T                      ioctl_gpd_addr;     /* waitRxFree[Out]                  */
    UI32_T                          buf_len;            /* setRxCfg[In]                     */
    HAL_NB_PKT_IOCTL_RX_TYPE_T      rx_type;            /* setRxCfg[In]                     */

} HAL_NB_PKT_IOCTL_RX_COOKIE_T;

typedef struct
{
    UI32_T                          port;
    UI32_T                          status;
    CLX_PORT_SPEED_T                speed;

} HAL_NB_PKT_IOCTL_PORT_COOKIE_T;


#if defined(NETIF_EN_NETLINK)

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

} HAL_NB_PKT_NL_IOCTL_COOKIE_T;


#endif  /* End of NETIF_EN_NETLINK */

typedef union
{
    UI32_T                          value;
    struct
    {
        UI32_T                      unit :  6;      /* Maximum unit number is 64.       */
        HAL_NB_PKT_IOCTL_TYPE_T     type : 10;      /* Maximum 1024 IOCTL types         */
        UI32_T                      rsvd : 16;
    } field;

} HAL_NB_PKT_IOCTL_CMD_T;



typedef struct
{
    UI32_T                              intr_reg;
    CLX_SEMAPHORE_ID_T                  intr_event;
    UI32_T                              intr_cnt;

} HAL_NB_PKT_INTR_VEC_T;

typedef struct HAL_NB_PKT_PROFILE_NODE_S
{
    HAL_NB_PKT_NETIF_PROFILE_T         *ptr_profile;
    struct HAL_NB_PKT_PROFILE_NODE_S   *ptr_next_node;

} HAL_NB_PKT_PROFILE_NODE_T;

typedef struct
{
    HAL_NB_PKT_NETIF_INTF_T            meta;
    struct net_device                  *ptr_net_dev;
    HAL_NB_PKT_PROFILE_NODE_T          *ptr_profile_list;  /* the profiles binding to this interface */

} HAL_NB_PKT_NETIF_PORT_DB_T;

#endif /* End of CLX_EN_NETIF */

#ifdef __KERNEL__
CLX_ERROR_NO_T
hal_nb_pkt_init(
    const UI32_T                        unit);

CLX_ERROR_NO_T
hal_nb_pkt_exit(
    const UI32_T                        unit);

ssize_t
hal_nb_pkt_dev_tx(
    struct file                         *file,
    const char __user                   *buf,
    size_t                              count,
    loff_t                              *pos);

long
hal_nb_pkt_dev_ioctl(
    struct file                         *filp,
    unsigned int                        cmd,
    unsigned long                       arg);
#endif //__KERNEL__



#endif
