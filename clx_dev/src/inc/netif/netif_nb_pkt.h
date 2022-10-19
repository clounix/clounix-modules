#ifndef __CLX_NB_H__
#define __CLX_NB_H__

#include <clx_error.h>
#include <netif/netif_common.h>

/*PDMA reg definition*/
#define HAL_NB_PDMA_BASE_ADDR                               (0xE0040000)
#define HAL_NB_PDMA_GET_MMIO(__offset__)                    (HAL_NB_PDMA_BASE_ADDR + (__offset__))

#define HAL_NB_PDMA_CFG_CH_ENABLE                           (0x0)
#define HAL_NB_PDMA_CFG_DESC_LOCATION                       (0x4)
#define HAL_NB_PDMA_CFG_DESC_ENDIAN                         (0x8)
#define HAL_NB_PDMA_CFG_DATA_ENDIAN                         (0xC)
#define HAL_NB_PDMA_CFG_AXI_PROTOCOL_INFO                   (0x10)
#define HAL_NB_PDMA_CFG_AXI0_OUTSTD_SIZE                    (0x14)
#define HAL_NB_PDMA_CFG_AXI1_OUTSTD_SIZE                    (0x18)
#define HAL_NB_PDMA_CFG_AXI2_OUTSTD_SIZE                    (0x1C)
#define HAL_NB_PDMA_CFG_FIFIO_PATH_SEL                      (0x20)
#define HAL_NB_PDMA_CFG_CRC_EN                              (0x24)
#define HAL_NB_PDMA_CFG_P2H_RX_FIFO_ALM_FULL                (0x28)
#define HAL_NB_PDMA_CFG_P2H_TX_FIFO_ALM_FULL                (0x2C)
#define HAL_NB_PDMA_CFG_P2E_RX_FIFO_ALM_FULL                (0x30)
#define HAL_NB_PDMA_CFG_P2E_TX_FIFO_ALM_FULL                (0x34)
#define HAL_NB_PDMA_CFG_FIFO_DEBUG_EN                       (0x38)
#define HAL_NB_PDMA_DHS_P2H_RX_FIFO0_DATA                   (0x3C)
#define HAL_NB_PDMA_DHS_P2H_RX_FIFO1_DATA                   (0x44)
#define HAL_NB_PDMA_DHS_P2H_RX_FIFO2_DATA                   (0x4C)
#define HAL_NB_PDMA_DHS_P2H_RX_FIFO3_DATA                   (0x54)
#define HAL_NB_PDMA_DHS_P2H_TX_FIFO0_DATA                   (0x5C)
#define HAL_NB_PDMA_DHS_P2H_TX_FIFO1_DATA                   (0x64)
#define HAL_NB_PDMA_DHS_P2H_TX_FIFO2_DATA                   (0x6C)
#define HAL_NB_PDMA_DHS_P2H_TX_FIFO3_DATA                   (0x74)
#define HAL_NB_PDMA_DHS_P2E_RX_FIFO_DATA                    (0x7C)
#define HAL_NB_PDMA_DHS_P2E_TX_FIFO_DATA                    (0x84)
#define HAL_NB_PDMA_IRQ_PCIE                                (0x8C)
#define HAL_NB_PDMA_IRQ_PCIE_MSK                            (0x90)
#define HAL_NB_PDMA_IRQ_PCIE_TST                            (0x94)
#define HAL_NB_PDMA_STA_INFO                                (0x98)
#define HAL_NB_PDMA_CFG_SINGLE_PENDING                      (0x9C)
#define HAL_NB_PDMA_CFG_RESET                               (0xA0)
#define HAL_NB_PDMA_CFG_FSM_STATE                           (0xA4)
#define HAL_NB_PDMA_CFG_CH0_RING_BASE                       (0xA8)
#define HAL_NB_PDMA_CFG_CH0_RING_SIZE                       (0x148)
#define HAL_NB_PDMA_CFG_CH0_DESC_WORK_IDX                   (0x198)
#define HAL_NB_PDMA_CFG_CH0_DESC_POP_IDX                    (0x1E8)
#define HAL_NB_PDMA_CFG_CH0_MODE                            (0x238)


#define HAL_NB_GET_PDMA_CH_RING_BASE_REG(__channel__)       (HAL_NB_PDMA_CFG_CH0_RING_BASE + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_RING_SIZE_REG(__channel__)       (HAL_NB_PDMA_CFG_CH0_RING_SIZE + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(__channel__)   (HAL_NB_PDMA_CFG_CH0_DESC_WORK_IDX + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(__channel__)    (HAL_NB_PDMA_CFG_CH0_DESC_POP_IDX + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MODE(__channel__)                (HAL_NB_PDMA_CFG_CH0_MODE + (0x4 * (__channel__)))


typedef enum {
    NB_PCX_DMA_HOSTMEM_TO_HOSTMEM       = 0,
    NB_PCX_DMA_HOSTMEM_TO_LOCALBUS      = 1,
    NB_PCX_DMA_HOSTMEM_TO_ECPU          = 2,
    NB_PCX_DMA_LOCALBUS_TO_HOSTMEM      = 4,
    NB_PCX_DMA_LOCALBUS_TO_LOCALBUS     = 5,
    NB_PCX_DMA_LOCALBUS_TO_ECPU         = 6,
    NB_PCX_DMA_ECPU_TO_HOSTMEM          = 8,
    NB_PCX_DMA_ECPU_TO_LOCALBUS         = 9,
    NB_PCX_DMA_ECPU_TO_ECPU             = 10
} HAL_NB_PDMA_CH_MODE;

typedef enum {
    NB_DMA_ACCESS_INDIRECT = 0,
    NB_DMA_ACCESS_DIRECT = 1
} HAL_NB_DMA_ACCESS_MODE;

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
#if defined(CLX_EN_LITTLE_ENDIAN)
typedef struct
{
    UI32_T  s_addr_lo           : 32;   
    UI32_T  s_addr_hi           : 16;   
    UI32_T  size                : 16;
    UI32_T  d_addr_lo           : 32;
    UI32_T  d_addr_hi           : 16;

    /*status[127:112]*/
    UI32_T  interrupt           :  1;
    UI32_T  sop                 :  1;
    UI32_T  eop                 :  1;
    UI32_T  err                 :  1;
    UI32_T  sinc                :  1;
    UI32_T  dinc                :  1;
    UI32_T  xfer_size           :  5; // do not care
    UI32_T  reserve             :  5;

} HAL_NB_PDMA_DESC_T;
#elif defined(CLX_EN_BIG_ENDIAN)
typedef struct
{
    /*status[127:112]*/
    UI32_T  reserve             :  5;
    UI32_T  xfer_size           :  5; // do not care
    UI32_T  dinc                :  1;
    UI32_T  sinc                :  1;
    UI32_T  err                 :  1;
    UI32_T  eop                 :  1;
    UI32_T  sop                 :  1;
    UI32_T  interrupt           :  1;

    UI32_T  d_addr_lo           : 32;
    UI32_T  d_addr_hi           : 16;
    UI32_T  size                : 16;
    UI32_T  s_addr_lo           : 32;   
    UI32_T  s_addr_hi           : 16;   

} HAL_NB_PDMA_DESC_T;
#else
#error "Host PDMA endian is not defined\n"
#endif

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
    UI32_T                              :3;
    UI32_T  mac_learn_en                :1;
    UI32_T                              :32;
    UI32_T                              :24;
    UI32_T  tapping_push_t              :1;
    UI32_T  tapping_push_o              :1;
    UI32_T  src_vlan                    :12;
    UI32_T  pvlan_port_type             :2;
    UI32_T  igr_vid_pop_num             :3;
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
    UI32_T  igr_vid_pop_num             :3;
    UI32_T  pvlan_port_type             :2;
    UI32_T  src_vlan                    :12;
    UI32_T  tapping_push_o              :1;
    UI32_T  tapping_push_t              :1;
    UI32_T                              :24;
    UI32_T                              :32;
    UI32_T  mac_learn_en                :1;
    UI32_T                              :3;
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



#define HAL_NETIF_GET_BIT(flags, bit)             (((flags) & (bit)) > 0 ? 1 : 0) /* bit get */
#define HAL_NETIF_SET_BIT(flags, bit)             ((flags) |= (bit))              /* bit set */
#define HAL_NETIF_CLR_BIT(flags, bit)             ((flags) &= ~(bit))             /* bit clear */

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
    HAL_NB_PDMA_TX_CHANNEL_0 = 0,
    HAL_NB_PDMA_TX_CHANNEL_1,
    HAL_NB_PDMA_TX_CHANNEL_2,
    HAL_NB_PDMA_TX_CHANNEL_3,
    HAL_NB_PDMA_TX_CHANNEL_LAST

} HAL_NB_PDMA_TX_CHANNEL_T;

#define HAL_NB_PORT_NUM                           (256)

#define HAL_NB_PKT_RX_QUEUE_NUM             (HAL_NB_PDMA_RX_CHANNEL_LAST)
#define HAL_NB_DFLT_RX_RING_SIZE            (HAL_DFLT_CFG_PKT_RX_GPD_NUM)
#define HAL_NB_DFLT_TX_RING_SIZE            (HAL_DFLT_CFG_PKT_TX_GPD_NUM)
#define HAL_NB_PKT_TX_WAIT_MODE             (HAL_PKT_TX_WAIT_ASYNC)

#define HAL_NB_PKT_PDMA_MAX_GPD_PER_PKT     (10)   /* <= 256   */
#define HAL_NB_PKT_PDMA_TX_INTR_TIMEOUT     (10 * 1000) /* us */
#define HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP    (10 * 1000) /* int */

typedef struct
{
    UI32_T                          unit;
    UI32_T                          channel;

} HAL_NB_PKT_ISR_COOKIE_T;



typedef void
(*HAL_NB_PKT_TX_FUNC_T)(
    const UI32_T                        unit,
    const void                          *ptr_sw_gpd,    /* SW-GPD to be processed  */
    void                                *ptr_coockie);  /* Private data of SDK     */


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

typedef struct HAL_NB_PKT_TX_SW_GPD_S
{
    HAL_NB_PKT_TX_FUNC_T                callback;       /* (unit, ptr_sw_gpd, ptr_cookie) */
    void                                *ptr_cookie;    /* Pointer of CLX_PKT_TX_PKT_T    */
    HAL_NB_PDMA_DESC_T                  desc;
    UI32_T                              desc_num;
    struct HAL_NB_PKT_TX_SW_GPD_S       *ptr_next;

    UI32_T                              channel;        /* For counter */

} HAL_NB_PKT_TX_SW_GPD_T;

typedef struct
{
    /* CLX_SEMAPHORE_ID_T           sema; */

    /* since the Tx GPD ring may be accessed by multiple process including
     * ndo_start_xmit (SW IRQ), it must be protected with an ISRLOCK
     * instead of the original semaphore
     */
    CLX_ISRLOCK_ID_T                ring_lock;

    UI32_T                          free_idx;
    UI32_T                          used_desc_num;
    UI32_T                          free_desc_num;

    UI32_T                          ring_size;
    HAL_NB_PDMA_DESC_T              *ring_base;
    HAL_NB_PDMA_DESC_T              *ring_base_align;
    BOOL_T                          err_flag;

    /* ASYNC */
    HAL_NB_PKT_TX_SW_GPD_T              **pptr_sw_gpd_ring;
    HAL_NB_PKT_TX_SW_GPD_T              **pptr_sw_gpd_bulk; /* temporary store packets to be enque */

    /* SYNC_INTR */
    CLX_SEMAPHORE_ID_T              sync_intr_sema;

} HAL_NB_PKT_TX_PDMA_T;


typedef struct
{
    UI32_T                              send_ok;
    UI32_T                              gpd_empty;
    UI32_T                              poll_timeout;

    /* queue */
    UI32_T                              enque_ok;
    UI32_T                              enque_retry;

    /* event */
    UI32_T                              trig_event;

    /* normal interrupt */
    UI32_T                              tx_done;

    /* TODO:abnormal interrupt */
    
    /* others */
    UI32_T                              err_recover;
    UI32_T                              error;

} HAL_NB_PKT_TX_CHANNEL_CNT_T;

typedef struct
{
    HAL_NB_PKT_TX_CHANNEL_CNT_T         channel[HAL_NB_PDMA_TX_CHANNEL_LAST];
    UI32_T                              invoke_gpd_callback;
    UI32_T                              no_memory;

    /* queue */
    UI32_T                              deque_ok;
    UI32_T                              deque_fail;

    /* event */
    UI32_T                              wait_event;

} HAL_NB_PKT_TX_CNT_T;

typedef struct
{
    HAL_PKT_TX_WAIT_T                     wait_mode;
    HAL_NB_PKT_TX_PDMA_T                  pdma[HAL_NB_PDMA_TX_CHANNEL_LAST];
    HAL_NB_PKT_TX_CNT_T                   cnt;

    /* handleTxDoneTask */
    CLX_THREAD_ID_T                       isr_task_id[HAL_NB_PDMA_TX_CHANNEL_LAST];
    HAL_NB_PKT_ISR_COOKIE_T               isr_task_cookie[HAL_NB_PDMA_TX_CHANNEL_LAST];

    /* txTask */
    HAL_PKT_SW_QUEUE_T                    sw_queue;
    CLX_SEMAPHORE_ID_T                    sync_sema;
    BOOL_T                                running;/* TRUE when Init txTask
                                                  * FALSE when Destroy txTask
                                                  */
    /* to block net intf Tx in driver level since netif_tx_disable()
     * cannot always prevent intf from Tx in time
     */
    BOOL_T                                net_tx_allowed;
} HAL_NB_PKT_TX_CB_T;

typedef struct
{
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          cur_idx; /*TODO */
    UI32_T                          ring_size;

    HAL_NB_PDMA_DESC_T              *ring_base;
    HAL_NB_PDMA_DESC_T              *ring_base_align;
    BOOL_T                          err_flag;
    struct sk_buff                  **pptr_skb_ring;
} HAL_NB_PKT_RX_PDMA_T;

typedef struct
{
    /* queue */
    UI32_T                              enque_ok;
    UI32_T                              enque_retry;
    UI32_T                              deque_ok;
    UI32_T                              deque_fail;

    /* event */
    UI32_T                              trig_event;

    /* normal interrupt */
    UI32_T                              rx_done;

    /* abnormal interrupt */
    //TODO

    /* others */
    UI32_T                              err_recover;
    UI32_T                              ecc_err;

    /* it means that user doesn't create intf on that port */
    UI32_T                              netdev_miss;

} HAL_NB_PKT_RX_CHANNEL_CNT_T;

typedef struct
{
    HAL_NB_PKT_RX_CHANNEL_CNT_T         channel[HAL_NB_PDMA_RX_CHANNEL_LAST];
    UI32_T                              invoke_gpd_callback;
    UI32_T                              no_memory;

    /* event */
    UI32_T                              wait_event;

} HAL_NB_PKT_RX_CNT_T;

typedef struct
{
    HAL_PKT_RX_SCHED_T              sched_mode;
    HAL_NB_PKT_RX_PDMA_T            pdma[HAL_NB_PDMA_RX_CHANNEL_LAST];
    HAL_NB_PKT_RX_CNT_T             cnt;

    /* handleRxDoneTask */
    CLX_THREAD_ID_T                 isr_task_id[HAL_NB_PDMA_RX_CHANNEL_LAST];
    HAL_NB_PKT_ISR_COOKIE_T         isr_task_cookie[HAL_NB_PDMA_RX_CHANNEL_LAST];

    /* rxTask */
    HAL_PKT_SW_QUEUE_T              sw_queue[HAL_NB_PDMA_RX_CHANNEL_LAST];
    UI32_T                          deque_idx;
    CLX_SEMAPHORE_ID_T              sync_sema;
    CLX_SEMAPHORE_ID_T              deinit_sema; /* To sync-up the Rx-stop and thread flush queues */
    BOOL_T                          running;     /* TRUE when rxStart
                                                  * FALSE when rxStop
                                                  */

} HAL_NB_PKT_RX_CB_T;

typedef struct HAL_NB_PKT_RX_SW_DESC_S
{
    BOOL_T                          rx_complete;    /* FALSE when PDMA error occurs */
    HAL_NB_PDMA_DESC_T              desc;
    
    struct HAL_NB_PKT_RX_SW_DESC_S  *ptr_next;

    HAL_NB_PP_HDR_T                 *pph;
    void                            *ptr_cookie;    /* Pointer of virt-addr */
} HAL_NB_PKT_RX_SW_DESC_T;


void hal_nb_register_drv_cb(
    const UI32_T unit);
#endif /* end of __CLX_NB_H__ */
