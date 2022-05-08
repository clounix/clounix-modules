#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <termios.h>
#include <sys/types.h>
#include <getopt.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <clx_types.h>
#include <osal/osal_mdc.h>
#include <osal/osal.h>
#include "clx_nb.h"


static HAL_NB_PDMA_DESC_T      *pdma_ring_base[HAL_NB_PDMA_GENERAL_CHANNEL_NUM];

static HAL_NB_PDMA_CH_MODE channel_mode[HAL_NB_PDMA_GENERAL_CHANNEL_NUM] = {
    NB_PCX_DMA_HOSTMEM_TO_LOCALBUS,
    NB_PCX_DMA_LOCALBUS_TO_HOSTMEM, 
    NB_PCX_DMA_LOCALBUS_TO_LOCALBUS, 
    NB_PCX_DMA_HOSTMEM_TO_ECPU, 
    NB_PCX_DMA_ECPU_TO_HOSTMEM, 
    NB_PCX_DMA_ECPU_TO_ECPU, 
    NB_PCX_DMA_LOCALBUS_TO_ECPU, 
    NB_PCX_DMA_ECPU_TO_LOCALBUS
};

static CLX_ERROR_NO_T clxdev_init_general_pdma_ring(UI32_T unit)
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    CLX_ADDR_T              phy_addr = 0x0;
    UI32_T                  channel = 0;
    UI32_T                  idx = 0;
    UI32_T                  byte_swap = HAL_NB_PDMA_BYTE_SWAP_DISABLE;
    UI32_T                  ring_size = 2;
    UI32_T                  enable_channel = HAL_NB_PDMA_ENABLE_CHANNEL;


    for (idx = 0;idx < HAL_NB_PDMA_GENERAL_CHANNEL_NUM;idx ++) {
        pdma_ring_base[idx] = (HAL_NB_PDMA_DESC_T *)osal_mdc_allocDmaMem(ring_size * sizeof(HAL_NB_PDMA_DESC_T));
        if(pdma_ring_base[idx] == NULL) {
            ret = CLX_E_NO_MEMORY;
            return ret;
        }
        memset(pdma_ring_base, 0,ring_size * sizeof(HAL_NB_PDMA_DESC_T));

        channel = idx + HAL_NB_PDMA_GENERAL_CHANNEL_0;
        /*ring base*/
        ret = osal_mdc_convertVirtToPhy(pdma_ring_base[idx],&phy_addr);
        if (ret != CLX_E_OK) {
            osal_printf("get phy addr failed\n");
            return ret;
        }
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_BASE_REG(channel)),
                            &phy_addr,sizeof(CLX_ADDR_T));
        /*ring size*/
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_SIZE_REG(channel)),
                            &ring_size,sizeof(UI32_T));

        /*channel mode*/
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_MODE_REG(channel)),
                            &channel_mode[idx],sizeof(UI32_T));

        /*byte swap*/
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_BYTE_ENDIAN_REG(channel)),
                            &byte_swap,sizeof(UI32_T));

        /*enable channel*/
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ENABLE_REG(channel)),
                            &enable_channel,sizeof(UI32_T));
    }
    return ret ;
}

static CLX_ERROR_NO_T clxdev_deinit_general_pdma_ring(UI32_T unit)
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    UI32_T                  channel = 0;
    UI32_T                  idx = 0;
    UI32_T                  enable_channel = HAL_NB_PDMA_DISABLE_CHANNEL;


    for (idx = 0;idx < HAL_NB_PDMA_GENERAL_CHANNEL_NUM;idx ++) {
        channel = idx + HAL_NB_PDMA_GENERAL_CHANNEL_0;
        /*disable channel*/
        osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ENABLE_REG(channel)),
                            &enable_channel,sizeof(UI32_T));
        ret = osal_mdc_freeDmaMem(pdma_ring_base[idx]);
    }
    
    return ret;
}
CLX_ERROR_NO_T
dcc_dma_writeTbl(
    const UI32_T           unit,
    void                   *ptr_src_buf,
    const UI32_T           addr,
    const UI32_T           entry_len,
    const UI32_T           entry_num )
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    UI32_T                  idx = CHANNEL_CPU_TO_LOCALBUS - HAL_NB_PDMA_GENERAL_CHANNEL_0;
    CLX_ADDR_T              src_phy_addr = 0x0;
    UI32_T                  pop_index = 0,work_index = 0; 
    HAL_NB_PDMA_DESC_T volatile  *descriptor;
    UI32_T              loop_cnt = 0;

    ret = osal_mdc_convertVirtToPhy(ptr_src_buf,&src_phy_addr);
    if (ret != CLX_E_OK) {
        osal_printf("get phy addr failed\n");
        return ret;
    }

    /*
     * TODO:
     *      semaphore
     */

    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(CHANNEL_CPU_TO_LOCALBUS)),
                        &pop_index,sizeof(UI32_T));
    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(CHANNEL_CPU_TO_LOCALBUS)),
                        &work_index,sizeof(UI32_T));

    if(pop_index != work_index) {
        ret = CLX_E_OP_INVALID;
        osal_printf("DMA error\n");
        return ret;
    }
    descriptor = &pdma_ring_base[idx][work_index];
    descriptor->d_addr = addr;
    descriptor->s_addr = src_phy_addr;
    descriptor->sinc = 1;
    descriptor->dinc = 1;
    descriptor->size = entry_len * entry_num;

    work_index += 1;
    work_index %= 2; 
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(CHANNEL_CPU_TO_LOCALBUS)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(CHANNEL_CPU_TO_LOCALBUS)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    osal_printf("unit:%d,src addr:0x%llx,dst addr:0x%llx,ret:%d\n",unit,src_phy_addr,addr,ret);
    return ret;
}

CLX_ERROR_NO_T
dcc_dma_readTbl(
    const UI32_T           unit,
    void                   *ptr_dst_buf,
    const UI32_T           addr,
    const UI32_T           entry_len,
    const UI32_T           entry_num )
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    UI32_T                  idx = CHANNEL_LOCALBUS_TO_CPU - HAL_NB_PDMA_GENERAL_CHANNEL_0;
    CLX_ADDR_T              dst_phy_addr = 0x0;
    UI32_T                  pop_index = 0,work_index = 0; 
    HAL_NB_PDMA_DESC_T volatile  *descriptor;
    UI32_T              loop_cnt = 0;

    ret = osal_mdc_convertVirtToPhy(ptr_dst_buf,&dst_phy_addr);
    if (ret != CLX_E_OK) {
        osal_printf("get phy addr failed\n");
        return ret;
    }

    /*
     * TODO:
     *      semaphore
     */

    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(CHANNEL_LOCALBUS_TO_CPU)),
                        &pop_index,sizeof(UI32_T));
    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(CHANNEL_LOCALBUS_TO_CPU)),
                        &work_index,sizeof(UI32_T));

    if(pop_index != work_index) {
        ret = CLX_E_OP_INVALID;
        osal_printf("DMA error\n");
        return ret;
    }
    descriptor = &pdma_ring_base[idx][work_index];
    descriptor->d_addr = dst_phy_addr;
    descriptor->s_addr = addr;
    descriptor->sinc = 1;
    descriptor->dinc = 1;
    descriptor->size = entry_len * entry_num;

    work_index += 1;
    work_index %= 2; 
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(CHANNEL_LOCALBUS_TO_CPU)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(CHANNEL_LOCALBUS_TO_CPU)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    osal_printf("unit:%d,src addr:0x%llx,dst addr:0x%llx,ret:%d\n",unit,addr,dst_phy_addr,ret);
    return ret;
}

typedef enum
{
    DCC_DMA_D2D_DIR_DESCENDING = 0x0,
    DCC_DMA_D2D_DIR_ASCENDING,
    DCC_DMA_D2D_DIR_LAST
} DCC_DMA_D2D_DIR_T;

CLX_ERROR_NO_T
dcc_dma_copyTbl(
    const UI32_T                unit,
    const UI32_T                src_addr,
    const UI32_T                dst_addr,
    const DCC_DMA_D2D_DIR_T     dir,
    const UI32_T                entry_len,
    const UI32_T                entry_num)
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    UI32_T                  idx = NB_PCX_DMA_LOCALBUS_TO_LOCALBUS - HAL_NB_PDMA_GENERAL_CHANNEL_0;
    UI32_T                  pop_index = 0,work_index = 0; 
    HAL_NB_PDMA_DESC_T volatile  *descriptor;
    UI32_T              loop_cnt = 0;

    /*
     * TODO:
     *      semaphore
     */

    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                        &pop_index,sizeof(UI32_T));
    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                        &work_index,sizeof(UI32_T));

    if(pop_index != work_index) {
        ret = CLX_E_OP_INVALID;
        osal_printf("DMA error\n");
        return ret;
    }
    descriptor = &pdma_ring_base[idx][work_index];
    descriptor->d_addr = dst_addr;
    descriptor->s_addr = src_addr;
    descriptor->sinc = 1;
    descriptor->dinc = 1;
    descriptor->size = entry_len * entry_num;

    work_index += 1;
    work_index %= 2; 
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    osal_printf("unit:%d,src addr:0x%llx,dst addr:0x%llx,ret:%d\n",unit,dst_addr,src_addr,ret);
    return ret;
}

CLX_ERROR_NO_T
dcc_dma_copyTcam(
    const UI32_T                unit,
    const UI32_T                src_addr,
    const UI32_T                dst_addr,
    const DCC_DMA_D2D_DIR_T     dir,
    const UI32_T                entry_len,
    const UI32_T                entry_num)
{
    CLX_ERROR_NO_T          ret = CLX_E_OK;
    UI32_T                  idx = NB_PCX_DMA_LOCALBUS_TO_LOCALBUS - HAL_NB_PDMA_GENERAL_CHANNEL_0;
    UI32_T                  pop_index = 0,work_index = 0; 
    HAL_NB_PDMA_DESC_T volatile  *descriptor;
    UI32_T              loop_cnt = 0;

    /*
     * TODO:
     *      semaphore
     */

    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                        &pop_index,sizeof(UI32_T));
    osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                        &work_index,sizeof(UI32_T));

    if(pop_index != work_index) {
        ret = CLX_E_OP_INVALID;
        osal_printf("DMA error\n");
        return ret;
    }
    descriptor = &pdma_ring_base[idx][work_index];
    descriptor->d_addr = dst_addr;
    descriptor->s_addr = src_addr;
    descriptor->sinc = 1;
    descriptor->dinc = 1;
    descriptor->size = entry_len * entry_num;

    work_index += 1;
    work_index %= 2; 
    osal_mdc_writePciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        osal_mdc_readPciReg(unit,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(NB_PCX_DMA_LOCALBUS_TO_LOCALBUS)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    osal_printf("unit:%d,src addr:0x%llx,dst addr:0x%llx,ret:%d\n",unit,dst_addr,src_addr,ret);
    return ret;
}
