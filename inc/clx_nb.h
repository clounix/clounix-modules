#ifndef __CLX_NB_H
#define __CLX_NB_H


/*PDMA reg definition*/
#define HAL_NB_PDMA_BASE_ADDR                       (0x0)
#define HAL_NB_PDMA_GET_MMIO(__offset__)            (HAL_NB_PDMA_BASE_ADDR + (__offset__))

#define HAL_NB_PDMA_INFO_REG                        (0x0)
#define HAL_NB_PDMA_SINGLE_PENDING_REG              (0x4)
#define HAL_NB_PDMA_RESET_REG                       (0x8)
#define HAL_NB_PMDA_FSM_STATE_REG                   (0xC)
#if defined(CLX_EN_LITTLE_ENDIAN)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T   wdata_fsm_state             :4;
        UI32_T   resp_fsm_state              :3;
        UI32_T   fetch_fsm_state             :3;
        UI32_T                               :23;
    } field;
} HAL_NB_PDMA_FSM_STATE_T;
#elif defined(CLX_EN_BIG_ENDIAN)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T                               :23;
        UI32_T   fetch_fsm_state             :3;
        UI32_T   resp_fsm_state              :3;
        UI32_T   wdata_fsm_state             :4;
    } field;
} HAL_NB_PDMA_FSM_STATE_T;
#else
#error "Host PDMA endian is not defined\n"
#endif

#define HAL_NB_PDMA_AXI0_RD_MAX_OUTSTD_SIZE_REG     (0x10)
#define HAL_NB_PDMA_AXI0_WR_MAX_OUTSTD_SIZE_REG     (0x14)
#define HAL_NB_PDMA_AXI1_RD_MAX_OUTSTD_SIZE_REG     (0x18)
#define HAL_NB_PDMA_AXI1_WR_MAX_OUTSTD_SIZE_REG     (0x1C)

/*PDMA channel reg definition*/
#define HAL_NB_PDMA_CH0_RING_BASE_REG          (0x20)
#define HAL_NB_PDMA_CH0_RING_SIZE_REG          (0xA0)
#define HAL_NB_PDMA_CH0_WORK_IDX_REG           (0xE0)
#define HAL_NB_PDMA_CH0_POP_IDX_REG            (0x120)
#define HAL_NB_PDMA_CH0_DESC_BURST_EN_REG      (0x160)
#define HAL_NB_PDMA_CH0_WRR_WEIGHT_REG         (0x1A0)
#define HAL_NB_PDMA_CH0_BYTE_ENDIAN_REG        (0x1E0)
#define HAL_NB_PDMA_CH0_ENABLE_REG             (0x220)
#define HAL_NB_PDMA_CH0_MODE_REG               (0x260)
#define HAL_NB_PDMA_CH0_DESC_ARLOCK_REG        (0x2A0)
#define HAL_NB_PDMA_CH0_DESC_ARCACHE_REG       (0x2E0)
#define HAL_NB_PDMA_CH0_DESC_ARPROT_REG        (0x320)
#define HAL_NB_PDMA_CH0_DESC_ARQOS_REG         (0x360)
#define HAL_NB_PDMA_CH0_DESC_ARREGION_REG      (0x3A0)
#define HAL_NB_PDMA_CH0_DESC_AWLOCK_REG        (0x3E0)
#define HAL_NB_PDMA_CH0_DESC_AWCACHE_REG       (0x420)
#define HAL_NB_PDMA_CH0_DESC_AWPROT_REG        (0x460)
#define HAL_NB_PDMA_CH0_DESC_AWQOS_REG         (0x4A0)
#define HAL_NB_PDMA_CH0_DESC_AWREGION_REG      (0x4E0)
#define HAL_NB_PDMA_CH0_MST0_ARLOCK_REG        (0x520)
#define HAL_NB_PDMA_CH0_MST0_ARCACHE_REG       (0x560)
#define HAL_NB_PDMA_CH0_MST0_ARPROT_REG        (0x5A0)
#define HAL_NB_PDMA_CH0_MST0_ARQOS_REG         (0x5E0)
#define HAL_NB_PDMA_CH0_MST0_ARREGION_REG      (0x620)
#define HAL_NB_PDMA_CH0_MST0_AWLOCK_REG        (0x660)
#define HAL_NB_PDMA_CH0_MST0_AWCACHE_REG       (0x6A0)
#define HAL_NB_PDMA_CH0_MST0_AWPROT_REG        (0x6E0)
#define HAL_NB_PDMA_CH0_MST0_AWQOS_REG         (0x720)
#define HAL_NB_PDMA_CH0_MST0_AWREGION_REG      (0x760)
#define HAL_NB_PDMA_CH0_MST1_ARLOCK_REG        (0x7A0)
#define HAL_NB_PDMA_CH0_MST1_ARCACHE_REG       (0x7E0)
#define HAL_NB_PDMA_CH0_MST1_ARPROT_REG        (0x820)
#define HAL_NB_PDMA_CH0_MST1_ARQOS_REG         (0x860)
#define HAL_NB_PDMA_CH0_MST1_ARREGION_REG      (0x8A0)
#define HAL_NB_PDMA_CH0_MST1_AWLOCK_REG        (0x8E0)
#define HAL_NB_PDMA_CH0_MST1_AWCACHE_REG       (0x920)
#define HAL_NB_PDMA_CH0_MST1_AWPROT_REG        (0x960)
#define HAL_NB_PDMA_CH0_MST1_AWQOS_REG         (0x9A0)
#define HAL_NB_PDMA_CH0_MST1_AWREGION_REG      (0x9E0)
#define HAL_NB_PDMA_CH0_INT_MSG_REG            (0xA20)
#define HAL_NB_PDMA_CH0_MSG_PER_DESC_REG       (0xA60)
#define HAL_NB_PDMA_CH0_INT_DONE_ADDR_REG      (0xAA0)
#define HAL_NB_PDMA_CH0_INT_ERROR_ADDR_REG     (0xB20)
#define HAL_NB_PDMA_CH0_INT_MSG_DATA_REG       (0xBA0)
#define HAL_NB_PDMA_CH0_ERROR_STATUS_REG       (0xBE0)
#define HAL_NB_PDMA_CH0_FETCH_NEEDED_REG       (0xC20)
#define HAL_NB_PDMA_CH0_CHANNEL_RDY_REG        (0xC60)
#define HAL_NB_PDMA_CH0_PENDING_READS_REG      (0xCA0)
#define HAL_NB_PDMA_CH0_PENDING_ACK_REG        (0xCE0)
#define HAL_NB_PDMA_CH0_DESC_VALID_REG         (0xD20)
#define HAL_NB_PDMA_CH0_RXFIFO_CTL_VALID_REG   (0xD60)
#define HAL_NB_PDMA_CH0_RXFIFO_EOP_REG         (0xDA0)
#define HAL_NB_PDMA_CH0_DESC_SADDR_REG         (0xDE0)
#define HAL_NB_PDMA_CH0_DESC_DADDR_REG         (0xE60)
#define HAL_NB_PDMA_CH0_DESC_SIZE_REG          (0xEE0)
#define HAL_NB_PDMA_CH0_DESC_INT_REG           (0xF20)
#define HAL_NB_PDMA_CH0_DESC_SOP_REG           (0xF60)
#define HAL_NB_PDMA_CH0_DESC_EOP_REG           (0xFA0)
#define HAL_NB_PDMA_CH0_DESC_ERR_REG           (0xFE0)
#define HAL_NB_PDMA_CH0_DESC_FIFOSZ_REG        (0x1020)
#define HAL_NB_PDMA_CH0_DESC_OFFSET_REG        (0x1060)
#define HAL_NB_PDMA_CH0_DESC_SINC_REG          (0x10A0)
#define HAL_NB_PDMA_CH0_DESC_DINC_REG          (0x10E0)
#define HAL_NB_PDMA_CH0_DESC_XFER_SIZE_REG     (0x1120)

#define HAL_NB_GET_PDMA_CH_RING_BASE_REG(__channel__)       (HAL_NB_PDMA_CH0_RING_BASE_REG + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_RING_SIZE_REG(__channel__)       (HAL_NB_PDMA_CH0_RING_SIZE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(__channel__)   (HAL_NB_PDMA_CH0_WORK_IDX_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(__channel__)    (HAL_NB_PDMA_CH0_POP_IDX_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_BURST_EN_REG(__channel__)   (HAL_NB_PDMA_CH0_DESC_BURST_EN_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_WRR_WEIGHT_REG(__channel__)      (HAL_NB_PDMA_CH0_WRR_WEIGHT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_BYTE_ENDIAN_REG(__channel__)     (HAL_NB_PDMA_CH0_BYTE_ENDIAN_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_ENABLE_REG(__channel__)          (HAL_NB_PDMA_CH0_ENABLE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MODE_REG(__channel__)            (HAL_NB_PDMA_CH0_MODE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ARLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_DESC_ARLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ARCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_DESC_ARCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ARPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_DESC_ARPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ARQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_DESC_ARQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ARREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_DESC_ARREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_AWLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_DESC_AWLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_AWCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_DESC_AWCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_AWPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_DESC_AWPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_AWQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_DESC_AWQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_AWREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_DESC_AWREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_ARLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_MST0_ARLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_ARCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_MST0_ARCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_ARPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_MST0_ARPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_ARQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_MST0_ARQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_ARREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_MST0_ARREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_AWLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_MST0_AWLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_AWCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_MST0_AWCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_AWPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_MST0_AWPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_AWQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_MST0_AWQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST0_AWREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_MST0_AWREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_ARLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_MST1_ARLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_ARCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_MST1_ARCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_ARPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_MST1_ARPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_ARQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_MST1_ARQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_ARREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_MST1_ARREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_AWLOCK_REG(__channel__)     (HAL_NB_PDMA_CH0_MST1_AWLOCK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_AWCACHE_REG(__channel__)    (HAL_NB_PDMA_CH0_MST1_AWCACHE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_AWPROT_REG(__channel__)     (HAL_NB_PDMA_CH0_MST1_AWPROT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_AWQOS_REG(__channel__)      (HAL_NB_PDMA_CH0_MST1_AWQOS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MST1_AWREGION_REG(__channel__)   (HAL_NB_PDMA_CH0_MST1_AWREGION_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_INT_MSG_REG(__channel__)         (HAL_NB_PDMA_CH0_INT_MSG_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_MSG_PER_DESC_REG(__channel__)    (HAL_NB_PDMA_CH0_MSG_PER_DESC_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_INT_DONE_ADDR_REG(__channel__)   (HAL_NB_PDMA_CH0_INT_DONE_ADDR_REG + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_INT_ERROR_ADDR_REG(__channel__)  (HAL_NB_PDMA_CH0_INT_ERROR_ADDR_REG + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_INT_MSG_DATA_REG(__channel__)    (HAL_NB_PDMA_CH0_INT_MSG_DATA_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_ERROR_STATUS_REG(__channel__)    (HAL_NB_PDMA_CH0_ERROR_STATUS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_FETCH_NEEDED_REG(__channel__)    (HAL_NB_PDMA_CH0_FETCH_NEEDED_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_CHANNEL_RDY_REG(__channel__)     (HAL_NB_PDMA_CH0_CHANNEL_RDY_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_PENDING_READS_REG(__channel__)   (HAL_NB_PDMA_CH0_PENDING_READS_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_PENDING_ACK_REG(__channel__)     (HAL_NB_PDMA_CH0_PENDING_ACK_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_VALID_REG(__channel__)      (HAL_NB_PDMA_CH0_DESC_VALID_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_RXFIFO_CTL_VALID_REG(__channel__)    (HAL_NB_PDMA_CH0_RXFIFO_CTL_VALID_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_RXFIFO_EOP_REG(__channel__)      (HAL_NB_PDMA_CH0_RXFIFO_EOP_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_SADDR_REG(__channel__)      (HAL_NB_PDMA_CH0_DESC_SADDR_REG + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_DADDR_REG(__channel__)      (HAL_NB_PDMA_CH0_DESC_DADDR_REG + (0x8 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_SIZE_REG(__channel__)       (HAL_NB_PDMA_CH0_DESC_SIZE_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_INT_REG(__channel__)        (HAL_NB_PDMA_CH0_DESC_INT_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_SOP_REG(__channel__)        (HAL_NB_PDMA_CH0_DESC_SOP_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_EOP_REG(__channel__)        (HAL_NB_PDMA_CH0_DESC_EOP_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_ERR_REG(__channel__)        (HAL_NB_PDMA_CH0_DESC_ERR_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_FIFIOSZ_REG(__channel__)    (HAL_NB_PDMA_CH0_DESC_FIFOSZ_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_OFFSET_REG(__channel__)     (HAL_NB_PDMA_CH0_DESC_OFFSET_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_SINC_REG(__channel__)       (HAL_NB_PDMA_CH0_DESC_SINC_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_DINC_REG(__channel__)       (HAL_NB_PDMA_CH0_DESC_DINC_REG + (0x4 * (__channel__)))
#define HAL_NB_GET_PDMA_CH_DESC_XFER_SIZE_REG(__channel__)  (HAL_NB_PDMA_CH0_DESC_XFER_SIZE_REG + (0x4 * (__channel__)))




/*PDMA mem reg definition*/
#define HAL_NB_PDMA_MEM_CTRL_REG                   (0x1160)
#if defined(CLX_EN_LITTLE_ENDIAN)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T   ecc_capture_double_only     :1;
        UI32_T   ecc_correct_en              :1;
        UI32_T   ecc_detect_en               :1;
        UI32_T   ecc_hw_gen_en               :1;
        UI32_T                               :28;
    } field;
} PDMA_MEM_CTRL_T;
#elif defined(CLX_EN_BIG_ENDIAN)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T                               :28;
        UI32_T   ecc_capture_double_only     :1;
        UI32_T   ecc_correct_en              :1;
        UI32_T   ecc_detect_en               :1;
        UI32_T   ecc_hw_gen_en               :1;
    } field;
} PDMA_MEM_CTRL_T;
#else
#error "Host PDMA endian is not defined\n"
#endif

#define HAL_NB_PDMA_MEM_ECC_ERR_CAPTURE_EN_REG      (0x1164)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T   mem_ecc_err_capture_en      :8;
        UI32_T                               :24;
    } field;
} PDMA_MEM_ECC_CAPTURE_CTRL_T;

#define HAL_NB_IRQ_PDMA_MEM_ECC_REG                 (0x1168)
#define HAL_NB_IRQ_PDMA_MEM_ECC_MSK_REG             (0x116C)
#define HAL_NB_IRQ_PDMA_MEM_ECC_TST_REG             (0x1170)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T   ecc_single_err_int          :1;
        UI32_T   ecc_double_err_int          :1;
        UI32_T                               :30;
    } field;
} PDMA_MEM_ECC_IRQ_T;

#define HAL_NB_IRQ_PDMA_MEM_SERR_REG                (0x1174)
#define HAL_NB_IRQ_PDMA_MEM_SERR_MSK_REG            (0x1178)
#define HAL_NB_IRQ_PDMA_MEM_SERR_TST_REG            (0x117C)
typedef union
{
    UI32_T reg;
    struct  {
        UI32_T   rx_fifo_mem_ecc_err_single_set      :4;
        UI32_T   tx_fifo_mem_ecc_err_single_set      :4;
        UI32_T                                       :24;
    } field;
} PDMA_MEM_SERR_IRQ_T;

#define HAL_NB_IRQ_PDMA_MEM_DERR_REG                (0x1180)
#define HAL_NB_IRQ_PDMA_MEM_DERR_MSK_REG            (0x1184)
#define HAL_NB_IRQ_PDMA_MEM_DERR_TST_REG            (0x1188)
#define HAL_NB_SYM_PDMA_MEM_ECC_ERR_INFO_REG        (0x118C)
#define HAL_NB_CP_PAR_ERR_ADDR_REG                  (0x1194)
#define HAL_NB_CP_PAR_ERR_INJ_REG                   (0x1198)



/*enable pdma channel*/
#define HAL_NB_PDMA_ENABLE_CHANNEL                  0x1
#define HAL_NB_PDMA_DISABLE_CHANNEL                 0x0

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
    UI64_T  sop                 :  1;
    UI64_T  eop                 :  1;
    UI64_T  err                 :  1;
    UI64_T  sinc                :  1;
    UI64_T  dinc                :  1;
    UI64_T  xfer_size           :  5;
    UI64_T  reserve             :  5;

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

#endif /* end of __CLX_NB_H */
