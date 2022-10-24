#ifndef HAL_NETIF_H
#define HAL_NETIF_H

#include <clx_types.h>

/* CLX DEFINITION */
#define CLX_NETIF_NAME_LEN              (16)
#define CLX_NETIF_PROFILE_NUM_MAX       (256)
#define CLX_NETIF_PROFILE_PATTERN_NUM   (4)
#define CLX_NETIF_PROFILE_PATTERN_LEN   (8)

/* Net Link */
#define CLX_NETIF_NETLINK_NUM_MAX                   (256)
#define CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX          (32)

typedef struct
{
    C8_T                                name[CLX_NETIF_NAME_LEN];

} CLX_NETIF_NETLINK_MC_GROUP_T;

typedef struct
{
    UI32_T                              id;
    C8_T                                name[CLX_NETIF_NAME_LEN];
    CLX_NETIF_NETLINK_MC_GROUP_T        mc_group[CLX_NETIF_NETLINK_MC_GROUP_NUM_MAX];
    UI32_T                              mc_group_num;

} CLX_NETIF_NETLINK_T;


/* Port Speed */
typedef enum
{
    CLX_PORT_SPEED_1G   = 1000,
    CLX_PORT_SPEED_10G  = 10000,
    CLX_PORT_SPEED_25G  = 25000,
    CLX_PORT_SPEED_40G  = 40000,
    CLX_PORT_SPEED_50G  = 50000,
    CLX_PORT_SPEED_100G = 100000,
    CLX_PORT_SPEED_200G = 200000,
    CLX_PORT_SPEED_400G = 400000,
    CLX_PORT_SPEED_LAST
} CLX_PORT_SPEED_T;

/************************************************************************/
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
#ifndef __KERNEL__
    CMLIB_QUEUE_T                   *ptr_id;
#else
    CLX_HUGE_T                      que_id;
#endif
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          len;      /* Software CPU queue maximum length.        */
    UI32_T                          weight;   /* The weight for thread de-queue algorithm. */

} HAL_PKT_SW_QUEUE_T;

typedef struct
{
    UI32_T                          tx_wait_mode;
    UI32_T                          rx_sched_mode;
    UI32_T                          ring_size;

    HAL_PKT_SW_QUEUE_T              tx_que;
    HAL_PKT_SW_QUEUE_T              rx_que;

} HAL_PKT_IOCTL_DRV_COOKIE_T;

typedef struct
{
    UI32_T                          error_isr_stack_size;
    UI32_T                          error_thread_pri;
    UI32_T                          tx_isr_stack_size;
    UI32_T                          tx_thread_pri;
    UI32_T                          rx_isr_stack_size;
    UI32_T                          rx_thread_pri;

} HAL_PKT_IOCTL_TASK_COOKIE_T;

typedef enum
{
    HAL_PKT_IOCTL_RX_TYPE_INIT = 0,
    HAL_PKT_IOCTL_RX_TYPE_DEINIT,
    HAL_PKT_IOCTL_RX_TYPE_LAST,

} HAL_PKT_IOCTL_RX_TYPE_T;

typedef struct
{
    UI32_T                              buf_len;            /* setRxCfg[In]                     */
    HAL_PKT_IOCTL_RX_TYPE_T             rx_type;            /* setRxCfg[In]                     */
} HAL_PKT_IOCTL_RX_CFG_COOKIE_T;

typedef struct
{
    UI32_T                          port;
    UI32_T                          status;
    CLX_PORT_SPEED_T                speed;

} HAL_PKT_IOCTL_PORT_COOKIE_T;


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
    HAL_PKT_NETIF_INTF_T            net_intf;       /* addIntf[In,Out], delIntf[In]              */
    HAL_PKT_NETIF_PROFILE_T         net_profile;    /* createProfile[In,Out], destroyProfile[In] */
    HAL_PKT_NETIF_INTF_CNT_T        cnt;
    CLX_ERROR_NO_T                  rc;

} HAL_PKT_IOCTL_NETIF_COOKIE_T;



typedef enum
{
    HAL_NETIF_INTF_PROPERTY_IGR_SAMPLING_RATE,
    HAL_NETIF_INTF_PROPERTY_EGR_SAMPLING_RATE,
    HAL_NETIF_INTF_PROPERTY_SKIP_PORT_STATE_EVENT,
    HAL_NETIF_INTF_PROPERTY_ADMIN_STATE,
    HAL_NETIF_INTF_PROPERTY_PDMA_RX_CNT,
    HAL_NETIF_INTF_PROPERTY_RX_RCH_STATUS,
    HAL_NETIF_INTF_PROPERTY_LAST
} HAL_NETIF_INTF_PROPERTY_T;

typedef struct
{
    /* intf property */
    UI32_T                          intf_id;
    HAL_NETIF_INTF_PROPERTY_T       property;
    UI32_T                          param0;
    UI32_T                          param1;

    /* netlink */
    CLX_NETIF_NETLINK_T             netlink;

    CLX_ERROR_NO_T                  rc;

} HAL_PKT_NL_IOCTL_COOKIE_T;

#endif