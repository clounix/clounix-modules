/*******************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of Hangzhou Clounix Technology Limited. (C) 2013-2021
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("CLOUNIX SOFTWARE")
*  RECEIVED FROM CLOUNIX AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. CLOUNIX EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES CLOUNIX PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE CLOUNIX SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. CLOUNIX SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY CLOUNIX SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND CLOUNIX'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE CLOUNIX SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT CLOUNIX'S OPTION, TO REVISE OR REPLACE THE CLOUNIX SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  CLOUNIX FOR SUCH CLOUNIX SOFTWARE AT ISSUE.
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*******************************************************************************/

/* FILE NAME:  osal_dma.h
 * PURPOSE:
 *  1. Provide DCC DMA buffer management initialization and de-initialization
 *     function APIs.
 *  2. Provide DCC DMA buffer allocation and free APIs.
 *
 * NOTES:
 *
 */

#ifndef OSAL_DMA_H
#define OSAL_DMA_H

/* INCLUDE FILE DECLARATIONS
 */
#include <clx_error.h>
#include <clx_types.h>

/* NAMING CONSTANT DECLARATIONS
 */


/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

typedef struct OSAL_DMA_MPOOL_CONFIG_S
{
    UI32_T                   alloc_size; /* Must be 4 byte aligned*/
    UI32_T                   entry_cnt;
    UI32_T                   init_alloc;
} OSAL_DMA_MPOOL_CONFIG_T;


CLX_ERROR_NO_T
osal_dma_freePkt(
   const void  *ptr_dma_mem);

CLX_ERROR_NO_T
osal_dma_configTxPktPool(
    UI32_T unit);

CLX_ERROR_NO_T
osal_dma_configRxPktPool(
    UI32_T unit,
    const OSAL_DMA_MPOOL_CONFIG_T *ptr_rx_pool_config);

void *
osal_dma_allocTxPkt(
    const UI32_T size);

void *
osal_dma_allocRxPkt(void);

CLX_ERROR_NO_T
osal_dma_convertVirtToPhy(
    void            *ptr_virt_addr,
    CLX_ADDR_T      *ptr_phy_addr);


/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME:   osal_dma_initDmaMem
 * PURPOSE:
 *      osal_dma_initDmaMem() is responsible for DCC DMA memory management
 *      initialization.
 *
 * INPUT:
 *      unit          -- The unit number that would like to initialize.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully initialize DCC DMA resource.
 *      CLX_E_OTHERS  -- Fail to complete DCC DMA resource initialization.
 *
 * NOTES:
 *      This function will be invoked by init module's initialization
 *      framework.
 *
 */
CLX_ERROR_NO_T
osal_dma_initDmaMem(
    const UI32_T unit);

/* FUNCTION NAME:   osal_dma_deInitDmaMem
 * PURPOSE:
 *      osal_dma_deInitDmaMem() is responsible for DCC DMA memeory management
 *      de-initialization.
 *
 * INPUT:
 *      unit          -- The unit number that would like to de-initialize.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully de-initialize DCC DMA resource.
 *      CLX_E_OTHERS  -- Fail to complete DCC DMA resource de-initialization.
 *
 * NOTES:
 *      This function will be invoked by init module's initialization
 *      framework.
 *
 */
CLX_ERROR_NO_T
osal_dma_deInitDmaMem(
    const UI32_T unit);


/* FUNCTION NAME:   osal_dma_alloc
 * PURPOSE:
 *      osal_dma_alloc() is responsible for DCC DMA memory allocation.
 *
 * INPUT:
 *      size          -- The allocated DMA memory size.
 * OUTPUT:
 *      None
 * RETURN:
 *      void pointer  -- Successfully allocate this size DCC DMA memory.
 *      NULL          -- Fail to allocate DCC DMA memory.
 *
 * NOTES:
 *      None
 *
 */
void *
osal_dma_alloc(
    const UI32_T size);

/* FUNCTION NAME:   osal_dma_free
 * PURPOSE:
 *      osal_dma_free() is responsible for DCC DMA memory free.
 *
 * INPUT:
 *      ptr_dma_mem   -- The DMA memory that would like to be freed.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK      -- Successfully free DCC DMA memory.
 *      CLX_E_OTHERS  -- Fail to free DCC DMA memory.
 *
 * NOTES:
 *      None.
 *
 */
CLX_ERROR_NO_T
osal_dma_free(
    void *ptr_dma_mem);

CLX_ERROR_NO_T
osal_dma_showDmaUsage(
    const UI32_T unit);

/* GLOBAL VARIABLE EXTERN DECLARATIONS
*/

#endif  /* #ifndef OSAL_DMA_H */

