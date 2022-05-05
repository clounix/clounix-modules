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

/* FILE NAME:  osal_mdc.c
 * PURPOSE:
 *  1. Provide device operate from AML interface
 * NOTES:
 *
 */

/* INCLUDE FILE DECLARATIONS
 */
#include <stdio.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>

#include <clx_error.h>
#include <clx_types.h>
#include <osal/osal.h>
#include <osal/osal_mdc.h>

//DIAG_SET_MODULE_INFO(CLX_MODULE_AML, "osal_mdc.c");

/* #define OSAL_MDC_EN_TEST */
#define OSAL_MDC_EN_ADDR_LOOKUP_CACHE

/* NAMING CONSTANT DECLARATIONS
 */
#define OSAL_MDC_DEV_FILE_PATH          "/dev/"OSAL_MDC_DRIVER_NAME
#define OSAL_MDC_ISR_TASK_NAME_LEN      (8)
#define OSAL_MDC_ISR_TASK_STACK_SIZE    (64*1024)
#define OSAL_MDC_ISR_TASK_PRI           (99)

#define OSAL_MDC_DMA_NODE_CACHE_NUM     (4)

/* MACRO FUNCTION DECLARATIONS
 */

UI32_T      _ext_aml_run_mode = HAL_RUN_CHIP_MODE;

/* GLOBAL VARIABLE DECLARATIONS
 */

static OSAL_MDC_CB_T        _osal_mdc_cb;

#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE) && !defined(CLX_EN_DMA_RESERVED)
static OSAL_MDC_DMA_NODE_T  *_osal_mdc_dma_node_cache[OSAL_MDC_DMA_NODE_CACHE_NUM] = {0};
static UI32_T               _osal_mdc_dma_node_cache_oldest_idx = 0;
#endif

static CLX_ERROR_NO_T
_osal_mdc_ioctl(
    const UI32_T                    unit,
    const OSAL_MDC_IOCTL_ACCESS_T   access,
    const OSAL_MDC_IOCTL_TYPE_T     type,
    const UI32_T                    data_size,
    void                            *ptr_data)
{
    OSAL_MDC_CB_T           *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_IOCTL_CMD_T    cmd;
    int                     linux_rc;
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    cmd.value = 0x0;
    cmd.field.access  = access;
    cmd.field.type    = type;
    cmd.field.unit    = unit;
    cmd.field.size    = data_size;

    linux_rc = ioctl(ptr_cb->dev_fd, (long unsigned int)cmd.value, ptr_data);
    if (-1 == linux_rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "do ioctl failed, type=%d, errno=%d", type, errno);
        rc = CLX_E_OTHERS;
    }
    return (rc);
}

#if !defined(CLX_LAMP)

static CLX_ERROR_NO_T
_osal_mdc_searchDmaVirtAddr(
    CMLIB_LIST_T            *ptr_dma_list,
    const void              *ptr_virt_addr,
    CMLIB_LIST_NODE_T       **pptr_node,
    OSAL_MDC_DMA_NODE_T     **pptr_node_data)
{
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    CLX_ERROR_NO_T          rc;

    rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
    while (CLX_E_OK == rc)
    {
        rc = cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
        if (CLX_E_OK == rc)
        {
            if (ptr_curr_node_data->ptr_virt_addr == ptr_virt_addr)
            {
                *pptr_node      = ptr_curr_node;
                *pptr_node_data = ptr_curr_node_data;
                break;
            }
            rc = cmlib_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "get dma node failed, virt addr=%p, rc=%d\n",
                       ptr_virt_addr, rc);
        }
    }
    return (rc);
}
#endif

static CLX_ERROR_NO_T
_osal_mdc_destroyDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    CMLIB_LIST_T            *ptr_dma_list = ptr_dma_info->ptr_dma_list;
    CMLIB_LIST_NODE_T       *ptr_curr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data = NULL;
    CLX_ERROR_NO_T          rc = CLX_E_NOT_INITED;

    if (NULL != ptr_dma_list)
    {
        rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
        while (CLX_E_OK == rc)
        {
            rc = cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
            if ((CLX_E_OK == rc) && (NULL != ptr_curr_node_data))
            {
                rc = cmlib_list_deleteByData(ptr_dma_list, ptr_curr_node_data);
                if (CLX_E_OK == rc)
                {
                    osal_free(ptr_curr_node_data);
                }
            }

            rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
        }
        rc = cmlib_list_destroy(ptr_dma_list, NULL);
        if (CLX_E_OK == rc)
        {
            ptr_dma_info->ptr_dma_list = NULL;
        }
    }
    return (rc);
}

#if defined(CLX_EN_DMA_RESERVED)

#if defined(OSAL_MDC_EN_TEST)
static CLX_ERROR_NO_T
_osal_mdc_dumpRsrvDmaList( void )
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    UI32_T                  node = 0;
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    rc = cmlib_list_locateHead(ptr_dma_info->ptr_dma_list,
                               &ptr_curr_node);
    while (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_info->ptr_dma_list,
                               ptr_curr_node, (void **)&ptr_curr_node_data);
        DIAG_PRINT(HAL_DBG_INFO,
            "node %d. virt addr=%p, phy addr=%p, size=%d, avbl=%d\n", node,
            ptr_curr_node_data->ptr_virt_addr, ptr_curr_node_data->phy_addr,
            ptr_curr_node_data->size, ptr_curr_node_data->available);

        rc = cmlib_list_next(ptr_dma_info->ptr_dma_list,
                             ptr_curr_node, &ptr_curr_node);
        node++;
    }
    return (rc);
}
#endif

static CLX_ERROR_NO_T
_osal_mdc_createRsrvDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info)
{
    OSAL_MDC_DMA_NODE_T     *ptr_node_data;
    CLX_ERROR_NO_T          rc;

    rc = cmlib_list_create(OSAL_MDC_DMA_LIST_SZ_UNLIMITED,
                           CMLIB_LIST_TYPE_DOUBLY,
                           OSAL_MDC_DMA_LIST_NAME,
                           &ptr_dma_info->ptr_dma_list);
    if (CLX_E_OK == rc)
    {
        /* The first node, which contains all of the reserved memory */
        ptr_node_data = osal_alloc(sizeof(OSAL_MDC_DMA_NODE_T));
        if (NULL != ptr_node_data)
        {
            ptr_node_data->ptr_virt_addr = ptr_dma_info->ptr_rsrv_virt_addr;
            ptr_node_data->phy_addr      = ptr_dma_info->rsrv_phy_addr;
            ptr_node_data->size          = ptr_dma_info->rsrv_size;
            ptr_node_data->available     = TRUE;
            cmlib_list_insertToHead(ptr_dma_info->ptr_dma_list, ptr_node_data);
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "alloc mem size=%zu failed\n",
                       sizeof(OSAL_MDC_DMA_NODE_T));
            rc = CLX_E_NO_MEMORY;
        }
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_searchAvblRsrvDmaNode(
    CMLIB_LIST_T            *ptr_dma_list,
    const UI32_T            size,
    CMLIB_LIST_NODE_T       **pptr_avbl_node )
{
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    CLX_ERROR_NO_T          rc;

    rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
    while (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
        if ((TRUE == ptr_curr_node_data->available) && (ptr_curr_node_data->size >= size))
        {
            *pptr_avbl_node = ptr_curr_node;
            break;
        }
        rc = cmlib_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_splitRsrvDmaNodes(
    CMLIB_LIST_T            *ptr_dma_list,
    CMLIB_LIST_NODE_T       *ptr_ori_node,
    const UI32_T            size,
    OSAL_MDC_DMA_NODE_T     **pptr_new_node_data )
{
    OSAL_MDC_DMA_NODE_T     *ptr_ori_node_data;
    CLX_ERROR_NO_T          rc;

    cmlib_list_getNodeData(ptr_dma_list, ptr_ori_node, (void **)&ptr_ori_node_data);

    *pptr_new_node_data = osal_alloc(sizeof(OSAL_MDC_DMA_NODE_T));

    /* Create a new node */
    (*pptr_new_node_data)->size          = size;
    (*pptr_new_node_data)->phy_addr      = ptr_ori_node_data->phy_addr;
    (*pptr_new_node_data)->ptr_virt_addr = ptr_ori_node_data->ptr_virt_addr;
    (*pptr_new_node_data)->available     = TRUE;

    /* Update the original node */
    ptr_ori_node_data->size              -= size;
    ptr_ori_node_data->phy_addr          += size;
    ptr_ori_node_data->ptr_virt_addr =
        (void *)((CLX_HUGE_T)ptr_ori_node_data->ptr_virt_addr + (CLX_HUGE_T)size);

    rc = cmlib_list_insertBefore(ptr_dma_list, ptr_ori_node, (void *)*pptr_new_node_data);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "insert rsrv dma node to list failed, size=%d, rc=%d\n", size, rc);
        /* Recovery */
        ptr_ori_node_data->size          += size;
        ptr_ori_node_data->phy_addr      -= size;
        ptr_ori_node_data->ptr_virt_addr =
            (void *)((CLX_HUGE_T)ptr_ori_node_data->ptr_virt_addr - (CLX_HUGE_T)size);
        osal_free(*pptr_new_node_data);
    }
    return (rc);
}

static void *
_osal_mdc_allocRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    const UI32_T            size )
{
    CMLIB_LIST_T            *ptr_dma_list = ptr_dma_info->ptr_dma_list;
    CMLIB_LIST_NODE_T       *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_new_node_data;
    void                    *ptr_virt_addr = NULL;
    CLX_ERROR_NO_T          rc;

    rc = _osal_mdc_searchAvblRsrvDmaNode(ptr_dma_list, size, &ptr_node);
    if (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_list, ptr_node, (void **)&ptr_node_data);
        /* If the node size just fit the user's requirement, just give it to user */
        if (ptr_node_data->size == size)
        {
            ptr_node_data->available = FALSE;
            ptr_virt_addr = ptr_node_data->ptr_virt_addr;

        }
        /* or split a new node with user required size. */
        else
        {
            rc = _osal_mdc_splitRsrvDmaNodes(ptr_dma_list, ptr_node, size, &ptr_new_node_data);
            if (CLX_E_OK == rc)
            {
                ptr_new_node_data->available = FALSE;
                ptr_virt_addr = ptr_new_node_data->ptr_virt_addr;
            }
        }
    }
    return (ptr_virt_addr);
}

static CLX_ERROR_NO_T
_osal_mdc_mergeTwoRsrvDmaNodes(
    CMLIB_LIST_T            *ptr_dma_list,
    OSAL_MDC_DMA_NODE_T     *ptr_first_node_data,
    OSAL_MDC_DMA_NODE_T     *ptr_second_node_data )
{
    ptr_first_node_data->size += ptr_second_node_data->size;

    cmlib_list_deleteByData(ptr_dma_list, ptr_second_node_data);
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_mergeRsrvDmaNodes(
    CMLIB_LIST_T            *ptr_dma_list,
    CMLIB_LIST_NODE_T       *ptr_curr_node )
{
    CMLIB_LIST_NODE_T       *ptr_prev_node;
    CMLIB_LIST_NODE_T       *ptr_next_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_prev_node_data;
    OSAL_MDC_DMA_NODE_T     *ptr_next_node_data;
    CLX_ERROR_NO_T          rc;

    cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);

    /* First, check if the previous node is available */
    rc = cmlib_list_prev(ptr_dma_list, ptr_curr_node, &ptr_prev_node);
    if (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_list, ptr_prev_node, (void **)&ptr_prev_node_data);
        if (TRUE == ptr_prev_node_data->available)
        {
            _osal_mdc_mergeTwoRsrvDmaNodes(ptr_dma_list, ptr_prev_node_data, ptr_curr_node_data);
            ptr_curr_node      = ptr_prev_node;
            ptr_curr_node_data = ptr_prev_node_data;
        }
    }

    /* then, check if the next node is available */
    rc = cmlib_list_next(ptr_dma_list, ptr_curr_node, &ptr_next_node);
    if (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_list, ptr_next_node, (void **)&ptr_next_node_data);
        if (TRUE == ptr_next_node_data->available)
        {
            _osal_mdc_mergeTwoRsrvDmaNodes(ptr_dma_list, ptr_curr_node_data, ptr_next_node_data);
        }
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_freeRsrvDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    void                    *ptr_virt_addr)
{
    CMLIB_LIST_T            *ptr_dma_list = ptr_dma_info->ptr_dma_list;
    CMLIB_LIST_NODE_T       *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data = NULL;
    CLX_ERROR_NO_T          rc;

    rc = _osal_mdc_searchDmaVirtAddr(ptr_dma_list, ptr_virt_addr,
                                     &ptr_node, &ptr_node_data);
    if (CLX_E_OK == rc)
    {
        ptr_node_data->available = TRUE;
        _osal_mdc_mergeRsrvDmaNodes(ptr_dma_list, ptr_node);
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_initRsrvDmaMem(
    OSAL_MDC_CB_T           *ptr_cb )
{
    UI32_T                      unit = 0;
    OSAL_MDC_IOCTL_DMA_DATA_T   ioctl_data = {0};
    OSAL_MDC_DMA_INFO_T         *ptr_dma_info = &ptr_cb->dma_info;
    CLX_ERROR_NO_T              rc = CLX_E_OK;

    osal_memset(&ioctl_data, 0x0, sizeof(OSAL_MDC_IOCTL_DMA_DATA_T));

    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_READ,
                         OSAL_MDC_IOCTL_TYPE_MDC_INIT_RSRV_DMA_MEM,
                         sizeof(OSAL_MDC_IOCTL_DMA_DATA_T), (void *)&ioctl_data);

    if ((CLX_E_OK == rc) && (0x0 != ioctl_data.rsrv_dma_phy_addr))
    {
        ptr_dma_info->ptr_rsrv_virt_addr =
            (void *)mmap(0, ioctl_data.rsrv_dma_size,
                         PROT_READ | PROT_WRITE, MAP_SHARED,
                         ptr_cb->dev_fd, ioctl_data.rsrv_dma_phy_addr);

        ptr_dma_info->rsrv_phy_addr = ioctl_data.rsrv_dma_phy_addr;
        ptr_dma_info->rsrv_size     = ioctl_data.rsrv_dma_size;
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR,
            "init rsrv dma mem failed, get rsrv phy addr=" CLX_ADDR_PRINT ", rc=%d\n",
            ioctl_data.rsrv_dma_phy_addr, rc);
        rc = CLX_E_OTHERS;
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_deinitRsrvDmaMem(
    OSAL_MDC_CB_T           *ptr_cb )
{
    UI32_T                      unit = 0;
    OSAL_MDC_DMA_INFO_T         *ptr_dma_info = &ptr_cb->dma_info;
    CLX_ERROR_NO_T              rc;

    munmap(ptr_dma_info->ptr_rsrv_virt_addr, ptr_dma_info->rsrv_size);
    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_NONE,
                         OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_RSRV_DMA_MEM,
                         0, NULL);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "deinit rsrv dma mem failed, rc=%d\n", rc);
    }
    return (rc);
}

#else

#if defined(OSAL_MDC_EN_TEST)
static CLX_ERROR_NO_T
_osal_mdc_dumpSysDmaList( void )
{

    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
    UI32_T                  node = 0;
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    rc = cmlib_list_locateHead(ptr_dma_info->ptr_dma_list,
                               &ptr_curr_node);
    while (CLX_E_OK == rc)
    {
        cmlib_list_getNodeData(ptr_dma_info->ptr_dma_list,
                               ptr_curr_node, (void **)&ptr_curr_node_data);
        DIAG_PRINT(HAL_DBG_INFO,
            "node %d. virt addr=%p, phy addr=%p, size=%d\n", node,
            ptr_curr_node_data->ptr_virt_addr, ptr_curr_node_data->phy_addr,
            ptr_curr_node_data->size);

        rc = cmlib_list_next(ptr_dma_info->ptr_dma_list,
                             ptr_curr_node, &ptr_curr_node);
        node++;
    }
    return (rc);
}
#endif

static CLX_ERROR_NO_T
_osal_mdc_searchSysDmaPhyAddrRange(
    CMLIB_LIST_T            *ptr_dma_list,
    const CLX_ADDR_T        phy_addr,
    OSAL_MDC_DMA_NODE_T     **pptr_node_data)
{
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
    UI32_T                  idx;
    UI32_T                  cache_idx;
#endif
    CLX_ERROR_NO_T          rc = CLX_E_OTHERS;

#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
    for (idx = 0; idx < OSAL_MDC_DMA_NODE_CACHE_NUM; idx ++)
    {
        cache_idx = (_osal_mdc_dma_node_cache_oldest_idx + idx + 1) % OSAL_MDC_DMA_NODE_CACHE_NUM;
        ptr_curr_node_data = _osal_mdc_dma_node_cache[cache_idx];
        if (NULL != ptr_curr_node_data)
        {
            if ((phy_addr >= ptr_curr_node_data->phy_addr) &&
                (phy_addr < ptr_curr_node_data->phy_addr + (CLX_ADDR_T)ptr_curr_node_data->size))
            {
                *pptr_node_data = ptr_curr_node_data;
                rc = CLX_E_OK;
                break;
            }
        }
    }
#endif
    if (NULL == *pptr_node_data)
    {
        rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
        while (CLX_E_OK == rc)
        {
            rc = cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
            if (CLX_E_OK == rc)
            {
                if ((phy_addr >= ptr_curr_node_data->phy_addr) &&
                    (phy_addr < ptr_curr_node_data->phy_addr + (CLX_ADDR_T)ptr_curr_node_data->size))
                {
                    *pptr_node_data = ptr_curr_node_data;
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
                    _osal_mdc_dma_node_cache[_osal_mdc_dma_node_cache_oldest_idx] = ptr_curr_node_data;
                    _osal_mdc_dma_node_cache_oldest_idx =
                        (_osal_mdc_dma_node_cache_oldest_idx + OSAL_MDC_DMA_NODE_CACHE_NUM - 1) % OSAL_MDC_DMA_NODE_CACHE_NUM;
#endif
                    break;
                }
                rc = cmlib_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
            }
            else
            {
                rc = CLX_E_OTHERS;
                DIAG_PRINT(HAL_DBG_ERR, "get dma node failed, phy addr=" CLX_ADDR_PRINT "\n", phy_addr);
            }
        }
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_searchSysDmaVirtAddrRange(
    CMLIB_LIST_T            *ptr_dma_list,
    const void              *ptr_virt_addr,
    OSAL_MDC_DMA_NODE_T     **pptr_node_data)
{
    CMLIB_LIST_NODE_T       *ptr_curr_node;
    OSAL_MDC_DMA_NODE_T     *ptr_curr_node_data;
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
    UI32_T                  idx;
    UI32_T                  cache_idx;
#endif
    CLX_ERROR_NO_T          rc = CLX_E_OTHERS;

#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
    for (idx = 0; idx < OSAL_MDC_DMA_NODE_CACHE_NUM; idx ++)
    {
        cache_idx = (_osal_mdc_dma_node_cache_oldest_idx + idx + 1) % OSAL_MDC_DMA_NODE_CACHE_NUM;
        ptr_curr_node_data = _osal_mdc_dma_node_cache[cache_idx];
        if (NULL != ptr_curr_node_data)
        {
            if (((CLX_HUGE_T)ptr_virt_addr >= (CLX_HUGE_T)ptr_curr_node_data->ptr_virt_addr) &&
                ((CLX_HUGE_T)ptr_virt_addr < (CLX_HUGE_T)ptr_curr_node_data->ptr_virt_addr + (CLX_HUGE_T)ptr_curr_node_data->size))
            {
                *pptr_node_data = ptr_curr_node_data;
                rc = CLX_E_OK;
                break;
            }
        }
    }
#endif
    if (NULL == *pptr_node_data)
    {
        rc = cmlib_list_locateHead(ptr_dma_list, &ptr_curr_node);
        while (CLX_E_OK == rc)
        {
            rc = cmlib_list_getNodeData(ptr_dma_list, ptr_curr_node, (void **)&ptr_curr_node_data);
            if (CLX_E_OK == rc)
            {
                if (((CLX_HUGE_T)ptr_virt_addr >= (CLX_HUGE_T)ptr_curr_node_data->ptr_virt_addr) &&
                    ((CLX_HUGE_T)ptr_virt_addr < (CLX_HUGE_T)ptr_curr_node_data->ptr_virt_addr + (CLX_HUGE_T)ptr_curr_node_data->size))
                {
                    *pptr_node_data = ptr_curr_node_data;
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
                    _osal_mdc_dma_node_cache[_osal_mdc_dma_node_cache_oldest_idx] = ptr_curr_node_data;
                    _osal_mdc_dma_node_cache_oldest_idx =
                        (_osal_mdc_dma_node_cache_oldest_idx + OSAL_MDC_DMA_NODE_CACHE_NUM - 1) % OSAL_MDC_DMA_NODE_CACHE_NUM;
#endif
                    break;
                }
                rc = cmlib_list_next(ptr_dma_list, ptr_curr_node, &ptr_curr_node);
            }
            else
            {
                rc = CLX_E_OTHERS;
                DIAG_PRINT(HAL_DBG_ERR, "get dma node failed, virt addr=%p\n", ptr_virt_addr);
            }
        }
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_createSysDmaNodeList(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info )
{
    CLX_ERROR_NO_T          rc;

    rc = cmlib_list_create(OSAL_MDC_DMA_LIST_SZ_UNLIMITED,
                           CMLIB_LIST_TYPE_SINGLY,
                           OSAL_MDC_DMA_LIST_NAME,
                           &ptr_dma_info->ptr_dma_list);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                   "invoke cmlib_list_create failed, rc=%d\n", rc);
    }
    return (rc);
}
#if !defined(CLX_LAMP)

static void *
_osal_mdc_allocSysDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    const UI32_T            size )
{
    OSAL_MDC_IOCTL_DMA_DATA_T   ioctl_data;
    UI32_T                      unit = 0;
    OSAL_MDC_DMA_NODE_T         *ptr_node_data;
    void                        *ptr_user_addr = NULL;
    CLX_ERROR_NO_T              rc;

    ioctl_data.size     = size;
    ioctl_data.phy_addr = 0x0;

    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_READ_WRITE,
                         OSAL_MDC_IOCTL_TYPE_MDC_ALLOC_SYS_DMA_MEM,
                         sizeof(OSAL_MDC_IOCTL_DMA_DATA_T), (void *)&ioctl_data);

    if (CLX_E_OK == rc)
    {
        /* io_data.dma_node.ptr_virt_addr is an kernel-space address, we should
         * translate it into the user-space address.
         */
        ptr_user_addr = (void *)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED,
                                     _osal_mdc_cb.dev_fd, ioctl_data.phy_addr);
        
        if (MAP_FAILED != ptr_user_addr)
        {
            ptr_node_data = osal_alloc(sizeof(OSAL_MDC_DMA_NODE_T));
            ptr_node_data->phy_addr      = ioctl_data.phy_addr;
            ptr_node_data->ptr_virt_addr = ptr_user_addr;
            ptr_node_data->size          = size;

            rc = cmlib_list_insertToHead(ptr_dma_info->ptr_dma_list, ptr_node_data);
            if (CLX_E_OK != rc)
            {
                /* Release the allocated DMA memory. */
                ioctl_data.size          = ptr_node_data->size;
                ioctl_data.phy_addr      = ptr_node_data->phy_addr;

                _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_WRITE,
                                OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
                                sizeof(OSAL_MDC_IOCTL_DMA_DATA_T), (void *)&ioctl_data);

                osal_free(ptr_node_data);
                ptr_user_addr = NULL;
                DIAG_PRINT(HAL_DBG_ERR, "insert dma node to list failed, size=%d, rc=%d\n", size, rc);
            }
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "mmap failed, size=%d, errno=%d\n", size, errno);
            ptr_user_addr = NULL;
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR, "alloc dma mem size=%d failed, rc=%d\n", size, rc);
    }
    return (ptr_user_addr);
}

static CLX_ERROR_NO_T
_osal_mdc_freeSysDmaMem(
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info,
    void                    *ptr_virt_addr )
{
    OSAL_MDC_IOCTL_DMA_DATA_T   ioctl_data;
    CMLIB_LIST_NODE_T           *ptr_node = NULL;
    OSAL_MDC_DMA_NODE_T         *ptr_node_data = NULL;
    UI32_T                      unit = 0;
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
    OSAL_MDC_DMA_NODE_T         *ptr_curr_node_data;
    UI32_T                      idx;
#endif
    CLX_ERROR_NO_T              rc;

    rc = _osal_mdc_searchDmaVirtAddr(ptr_dma_info->ptr_dma_list, ptr_virt_addr,
                                     &ptr_node, &ptr_node_data );
    if (CLX_E_OK == rc)
    {
        ioctl_data.size         = ptr_node_data->size;
        ioctl_data.phy_addr     = ptr_node_data->phy_addr;

        rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_WRITE,
                             OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
                             sizeof(OSAL_MDC_IOCTL_DMA_DATA_T), (void *)&ioctl_data);

        if (CLX_E_OK == rc)
        {
#if defined(OSAL_MDC_EN_ADDR_LOOKUP_CACHE)
            for (idx = 0; idx < OSAL_MDC_DMA_NODE_CACHE_NUM; idx ++)
            {
                ptr_curr_node_data = _osal_mdc_dma_node_cache[idx];
                if (NULL != ptr_curr_node_data)
                {
                    if ((ptr_node_data->phy_addr >= ptr_curr_node_data->phy_addr) &&
                            (ptr_node_data->phy_addr < ptr_curr_node_data->phy_addr + (CLX_ADDR_T)ptr_curr_node_data->size))
                    {
                        _osal_mdc_dma_node_cache[idx] = NULL;
                        break;
                    }
                }
            }
#endif

            munmap(ptr_node_data->ptr_virt_addr, ptr_node_data->size);
            cmlib_list_deleteByData(ptr_dma_info->ptr_dma_list, ptr_node_data);
            osal_free(ptr_node_data);
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "free dma mem failed, virt addr=%p, rc=%d\n", ptr_virt_addr, rc);
        }
    }
    return (rc);
}
#endif
#endif  /* End of CLX_EN_DMA_RESERVED */

void *
osal_mdc_allocDmaMem(
    const UI32_T        size)
{
    void                    *ptr_virt_addr = NULL;

#if defined(CLX_LAMP)
    ptr_virt_addr =  osal_alloc(size);
#else
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;

    osal_takeSemaphore(&ptr_dma_info->sema, CLX_SEMAPHORE_WAIT_FOREVER);

#if defined(CLX_EN_DMA_RESERVED)
    ptr_virt_addr = _osal_mdc_allocRsrvDmaMem(ptr_dma_info, size);
#else

    if (HAL_RUN_PCIE_BYPASS_MODE == _ext_aml_run_mode)
    {
        ptr_virt_addr =  osal_alloc(size);
    }
    else
    {
        ptr_virt_addr = _osal_mdc_allocSysDmaMem(ptr_dma_info, size);
    }

#endif
    osal_giveSemaphore(&ptr_dma_info->sema);
#endif


    return ptr_virt_addr;
}

CLX_ERROR_NO_T
osal_mdc_freeDmaMem(
    void            *ptr_virt_addr)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

#if defined(CLX_LAMP)
    osal_free(ptr_virt_addr);
#else
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;

    osal_takeSemaphore(&ptr_dma_info->sema, CLX_SEMAPHORE_WAIT_FOREVER);

#if defined(CLX_EN_DMA_RESERVED)
    rc = _osal_mdc_freeRsrvDmaMem(ptr_dma_info, ptr_virt_addr);
#else
    if (HAL_RUN_PCIE_BYPASS_MODE == _ext_aml_run_mode)
    {
        osal_free(ptr_virt_addr);
    }
    else
    {
        rc = _osal_mdc_freeSysDmaMem(ptr_dma_info, ptr_virt_addr);
    }
#endif
    osal_giveSemaphore(&ptr_dma_info->sema);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "free dma mem failed, virt addr=%p not found\n",
                   ptr_virt_addr);
    }
#endif

    return (rc);
}


CLX_ERROR_NO_T
osal_mdc_initDmaMem( void )
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    OSAL_MDC_CB_T           *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &ptr_cb->dma_info;

    rc = osal_createSemaphore(OSAL_MDC_DMA_SEMAPHORE_NAME,
                              CLX_SEMAPHORE_BINARY, &ptr_dma_info->sema);
    if (CLX_E_OK == rc)
    {
#if defined(CLX_EN_DMA_RESERVED)
        rc = _osal_mdc_initRsrvDmaMem(ptr_cb);
        if (CLX_E_OK == rc)
        {
            rc = _osal_mdc_createRsrvDmaNodeList(ptr_dma_info);
        }
#else
        rc = _osal_mdc_createSysDmaNodeList(ptr_dma_info);
#endif
    }
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_deinitDmaMem( void )
{
    OSAL_MDC_CB_T           *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &ptr_cb->dma_info;

    /* Common function for both reserved/system memory. */
    _osal_mdc_destroyDmaNodeList(ptr_dma_info);

#if defined(CLX_EN_DMA_RESERVED)
    _osal_mdc_deinitRsrvDmaMem(ptr_cb);
#endif

    osal_destroySemaphore(&ptr_dma_info->sema);

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
osal_mdc_convertPhyToVirt(
    const CLX_ADDR_T    phy_addr,
    void                **pptr_virt_addr)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info  = &_osal_mdc_cb.dma_info;
#if defined(CLX_EN_DMA_RESERVED)
    CLX_HUGE_T              rsrv_virt_base = (CLX_HUGE_T)ptr_dma_info->ptr_rsrv_virt_addr;
    CLX_ADDR_T              rsrv_phy_base  = ptr_dma_info->rsrv_phy_addr;
    CLX_ERROR_NO_T          rc = CLX_E_OK;
#else
    CMLIB_LIST_T            *ptr_dma_list  = _osal_mdc_cb.dma_info.ptr_dma_list;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data = NULL;
    CLX_ERROR_NO_T          rc;
#endif

#if defined(CLX_EN_DMA_RESERVED)
    *pptr_virt_addr = (void *)(rsrv_virt_base + (CLX_HUGE_T)(phy_addr - rsrv_phy_base));
#else
    osal_takeSemaphore(&ptr_dma_info->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = _osal_mdc_searchSysDmaPhyAddrRange(ptr_dma_list, phy_addr, &ptr_node_data);
    if (CLX_E_OK == rc)
    {
        *pptr_virt_addr = (void *)(ptr_node_data->ptr_virt_addr
                                   + (CLX_HUGE_T)(phy_addr - ptr_node_data->phy_addr));
    }
    osal_giveSemaphore(&ptr_dma_info->sema);
#endif

#if defined(AML_EN_CUSTOM_DMA_ADDR)
    if (CLX_E_OK != rc)
    {
        /* Here the user may invoke the API for their private DMA
         * address conversion.
         */
    }
#endif
    return (rc);
}

#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN)
#define OSAL_MDC_CAST_PTR_TO_CLX_ADDR_T(__ptr__)        (((CLX_ADDR_T)((CLX_HUGE_T)__ptr__)) & 0xFFFFFFFF)
#else
#define OSAL_MDC_CAST_PTR_TO_CLX_ADDR_T(__ptr__)        (((CLX_ADDR_T)((CLX_HUGE_T)__ptr__)) & 0xFFFFFFFFFFFFFFFF)
#endif

CLX_ERROR_NO_T
osal_mdc_convertVirtToPhy(
    void            *ptr_virt_addr,
    CLX_ADDR_T      *ptr_phy_addr)
{
    OSAL_MDC_DMA_INFO_T     *ptr_dma_info = &_osal_mdc_cb.dma_info;
#if defined(CLX_EN_DMA_RESERVED)
    CLX_HUGE_T              rsrv_virt_base = (CLX_HUGE_T)ptr_dma_info->ptr_rsrv_virt_addr;
    CLX_ADDR_T              rsrv_phy_base  = ptr_dma_info->rsrv_phy_addr;
    CLX_ERROR_NO_T          rc = CLX_E_OK;
#else
    CMLIB_LIST_T            *ptr_dma_list  = _osal_mdc_cb.dma_info.ptr_dma_list;
    OSAL_MDC_DMA_NODE_T     *ptr_node_data = NULL;
    CLX_ERROR_NO_T          rc;
#endif

#if defined(CLX_EN_DMA_RESERVED)
    *ptr_phy_addr = (CLX_ADDR_T)((CLX_HUGE_T)rsrv_phy_base
                                 + (CLX_HUGE_T)ptr_virt_addr - rsrv_virt_base);
#else
    osal_takeSemaphore(&ptr_dma_info->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = _osal_mdc_searchSysDmaVirtAddrRange(ptr_dma_list, ptr_virt_addr, &ptr_node_data);
    if (CLX_E_OK == rc)
    {
        *ptr_phy_addr = ptr_node_data->phy_addr + OSAL_MDC_CAST_PTR_TO_CLX_ADDR_T(ptr_virt_addr)
                        - OSAL_MDC_CAST_PTR_TO_CLX_ADDR_T(ptr_node_data->ptr_virt_addr);
    }
    osal_giveSemaphore(&ptr_dma_info->sema);
#endif

#if defined(AML_EN_CUSTOM_DMA_ADDR)
    if (CLX_E_OK != rc)
    {
        /* Here the user may invoke the API for their private DMA
         * address conversion.
         */
    }
#endif
    return (rc);
}

static void
_osal_mdc_waitIntrTask(
    void    *ptr_argv)
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DEV_T      *ptr_dev;
    UI32_T              isr_bitmap;
    I32_T               idx = -1;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    osal_initRunThread();
    do
    {
        /* Considering the way to kill this task, we should check
         * if the handler exists before jumping to handler
         */
        if (sizeof(UI32_T) == read(ptr_cb->dev_fd, (void *)&isr_bitmap, sizeof(UI32_T)))
        {
            if (CLX_E_OK != osal_isRunThread())
            {
                break; /* deinit-thread */
            }

            while (CLX_E_OK == cmlib_bit_getIdxOfSetBit(isr_bitmap, &idx))
            {
                ptr_dev = &_osal_mdc_cb.dev[idx];
                if (NULL != ptr_dev->isr_callback)
                {
                    rc = ptr_dev->isr_callback(ptr_dev->ptr_isr_data);
                    if (CLX_E_OK != rc)
                    {
                        DIAG_PRINT(HAL_DBG_ERR, "handle irq failed, rc=%d\n", rc);
                    }
                }
            }
            /* sleep while interrupt not ready or deinitilized */
            if (0 == isr_bitmap)
            {
                osal_sleepThread(1000);
            }
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "read irq status failed, bitmap=0x%08x\n", isr_bitmap);
        }
    }
    while (CLX_E_OK == osal_isRunThread());
    osal_exitRunThread();
}

CLX_ERROR_NO_T
osal_mdc_connectIsr(
    const UI32_T            unit,
    AML_DEV_ISR_FUNC_T      handler,
    AML_DEV_ISR_DATA_T      *ptr_cookie)
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];
    CLX_ERROR_NO_T      rc = CLX_E_OTHERS;

    if (NULL == ptr_dev->isr_callback)
    {
        /* hook the user dispatcher. */
        ptr_dev->isr_callback = handler;
        ptr_dev->ptr_isr_data = (void *)((CLX_HUGE_T)unit);

        /* create the task to read device bitmap from kernel. */
        rc = osal_createThread("ISRTASK", OSAL_MDC_ISR_TASK_STACK_SIZE,
                               OSAL_MDC_ISR_TASK_PRI,
                               _osal_mdc_waitIntrTask,
                               NULL, &ptr_cb->intr_poll_task);
        if (CLX_E_OK == rc)
        {
            /* request_irq and hook kernel interrupt callback. */
            rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_WRITE,
                                 OSAL_MDC_IOCTL_TYPE_MDC_CONNECT_ISR,
                                 sizeof(AML_DEV_ISR_DATA_T), (void *)ptr_cookie);
            if (CLX_E_OK != rc)
            {
                DIAG_PRINT(HAL_DBG_ERR, "connect isr failed, rc=%d\n", rc);
            }
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR, "create isr task failed, rc=%d\n", rc);
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR, "double req isr err\n");
    }
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_disconnectIsr(
    const UI32_T        unit)
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* set stop flag. */
    osal_stopThread(&ptr_cb->intr_poll_task);

    /* free_irq, clear the kernel interrupt callback, and return the task from kernel space. */
    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_NONE,
                         OSAL_MDC_IOCTL_TYPE_MDC_DISCONNECT_ISR, 0, NULL);
    if (CLX_E_OK == rc)
    {
        /* clear the user dispatcher. */
        ptr_dev->isr_callback = NULL;
        ptr_dev->ptr_isr_data = NULL;
    }

    /* destroy the task. */
    osal_destroyThread(&ptr_cb->intr_poll_task);

    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_readPciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    UI32_T              *ptr_data,
    const UI32_T        len)
{
    UI32_T              idx;
    UI32_T              count;
    volatile void       *ptr_base_addr = _osal_mdc_cb.ptr_pci_mmio_base[unit];
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (NULL != ptr_base_addr)
    {
        if (OSAL_MDC_PCI_BUS_WIDTH == len)
        {
            *ptr_data = *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset));
        }
        else
        {
            if (0 == (len % OSAL_MDC_PCI_BUS_WIDTH))
            {
                count = len / OSAL_MDC_PCI_BUS_WIDTH;
                for (idx = 0; idx < count; idx++)
                {
                    *(ptr_data + idx) = *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset + idx * 4));
                }
            }
            else
            {
                rc = CLX_E_OTHERS;
            }
        }
    }
    else
    {
        rc = CLX_E_NOT_INITED;
    }

    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_writePciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    const UI32_T        *ptr_data,
    const UI32_T        len)
{
    UI32_T              idx;
    UI32_T              count;
    volatile void       *ptr_base_addr = _osal_mdc_cb.ptr_pci_mmio_base[unit];
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (NULL != ptr_base_addr)
    {

        if (OSAL_MDC_PCI_BUS_WIDTH == len)
        {
            *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset)) = *ptr_data;
        }
        else
        {
            if (0 == (len % OSAL_MDC_PCI_BUS_WIDTH))
            {
                count = len / OSAL_MDC_PCI_BUS_WIDTH;
                for (idx = 0; idx < count; idx++)
                {
                    *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset + idx * 4)) = *(ptr_data + idx);
                }
            }
            else
            {
                rc = CLX_E_OTHERS;
            }
        }
    }
    else
    {
        rc = CLX_E_NOT_INITED;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_attachAccessCallback(
    const UI32_T    dev_num,
    AML_DEV_T       *ptr_dev_list)
{
    UI32_T          idx;

    for (idx = 0; idx < dev_num; idx++)
    {
        if (AML_DEV_TYPE_PCI == ptr_dev_list[idx].if_type)
        {
            ptr_dev_list[idx].access.read_callback  = osal_mdc_readPciReg;
            ptr_dev_list[idx].access.write_callback = osal_mdc_writePciReg;
        }
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_getDeviceIdfromIoctlData(
    AML_DEV_ID_T                *ptr_id,
    AML_DEV_T                   *ptr_dev_list,
    const UI32_T                dev_num)
{
    UI32_T          dev_idx;

    for (dev_idx = 0; dev_idx < dev_num; dev_idx++)
    {
        ptr_dev_list[dev_idx].id.device   = ptr_id[dev_idx].device;
        ptr_dev_list[dev_idx].id.vendor   = ptr_id[dev_idx].vendor;
        ptr_dev_list[dev_idx].id.revision = ptr_id[dev_idx].revision;
    }
    return (CLX_E_OK);
}

CLX_ERROR_NO_T
osal_mdc_initDevice(
    AML_DEV_T       *ptr_dev_list,
    UI32_T          *ptr_dev_num)
{
    OSAL_MDC_CB_T               *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_IOCTL_DEV_DATA_T   ioctl_data;
    UI32_T                      unit = 0;
    UI32_T                      idx;
    CLX_ERROR_NO_T              rc = CLX_E_OTHERS;

    /* Create and open the I/O device. */
    if (0 != mknod(OSAL_MDC_DEV_FILE_PATH,
                   S_IFCHR,
                   makedev(OSAL_MDC_DRIVER_MISC_MAJOR_NUM, OSAL_MDC_DRIVER_MISC_MINOR_NUM)))
    {
        if (EEXIST == errno)
        {
            /* the mknod API may return fail if the OSAL_MDC_DEV_FILE_PATH is created in rootfs by default */
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR,
                       "mknod failed, path=%s, major num=%d, minor num=%d, errno=%d\n",
                       OSAL_MDC_DEV_FILE_PATH, OSAL_MDC_DRIVER_MISC_MAJOR_NUM,
                       OSAL_MDC_DRIVER_MISC_MINOR_NUM, errno);
            return (CLX_E_OTHERS);
        }
    }

    osal_memset(ptr_cb, 0x0, sizeof(OSAL_MDC_CB_T));

    ptr_cb->dev_fd = open(OSAL_MDC_DEV_FILE_PATH, O_RDWR | O_SYNC );
    if (ptr_cb->dev_fd > 0)
    {
        /* Init the interface. */
        rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_READ,
                             OSAL_MDC_IOCTL_TYPE_MDC_INIT_DEV,
                             sizeof(OSAL_MDC_IOCTL_DEV_DATA_T),
                             (void *)&ioctl_data);
        if (CLX_E_OK == rc)
        {
            *ptr_dev_num = ioctl_data.dev_num;
            ptr_cb->dev_num = ioctl_data.dev_num;

            /* Update the database of AML. */
            _osal_mdc_getDeviceIdfromIoctlData(ioctl_data.id, ptr_dev_list, ioctl_data.dev_num);
            for (idx = 0; idx < *ptr_dev_num; idx++)
            {
                ptr_cb->ptr_pci_mmio_base[idx] =
                    (void *)mmap(0, ioctl_data.pci_mmio_size[idx],
                                 PROT_READ | PROT_WRITE, MAP_SHARED,
                                 _osal_mdc_cb.dev_fd, ioctl_data.pci_mmio_phy_start[idx]);

                ptr_cb->pci_mmio_size[idx] = ioctl_data.pci_mmio_size[idx];
            }

            if (CLX_E_OK == rc)
            {
                _osal_mdc_attachAccessCallback(*ptr_dev_num, ptr_dev_list);
            }
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR, "open dev %s failed\n", OSAL_MDC_DEV_FILE_PATH);
    }
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_deinitDevice( void )
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    UI32_T              idx;
    UI32_T              unit = 0;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* Clear the database of AML. */
    for (idx = 0; idx < ptr_cb->dev_num; idx++)
    {
        munmap(ptr_cb->ptr_pci_mmio_base[idx], ptr_cb->pci_mmio_size[idx]);
    }

    /* Deinit the interface. */
    _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_NONE,
                    OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_DEV, 0, NULL);

    /* Close and destroy the I/O device. */
    close(ptr_cb->dev_fd);
    if (0 != remove(OSAL_MDC_DEV_FILE_PATH))
    {
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

#define OSAL_MDC_GET_PAGE_START_ADDR(__maddr__, __page__)                           \
    do {                                                                            \
        UI32_T _page_sz_ = getpagesize();                                           \
        __page__ = ((CLX_HUGE_T)__maddr__) & (~(((CLX_HUGE_T)_page_sz_)-1));        \
    } while(0)


CLX_ERROR_NO_T
osal_mdc_flushCache(
    void            *ptr_virt_addr,
    const UI32_T    size)
{
    /* The user needs to port this function based on
     * the OS and DMA memory cache settings
     */
    return (CLX_E_OK);
}

CLX_ERROR_NO_T
osal_mdc_invalidateCache(
    void            *ptr_virt_addr,
    const UI32_T    size)
{
    /* The user needs to port this function based on
     * the OS and DMA memory cache settings
     */
    return (CLX_E_OK);
}

CLX_ERROR_NO_T
osal_mdc_savePciConfig(
    const UI32_T        unit)
{
    CLX_ERROR_NO_T      rc;

    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_NONE,
                         OSAL_MDC_IOCTL_TYPE_MDC_SAVE_PCI_CONFIG, 0, NULL);
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_restorePciConfig(
    const UI32_T        unit)
{
    CLX_ERROR_NO_T      rc;

    rc = _osal_mdc_ioctl(unit, OSAL_MDC_IOCTL_ACCESS_NONE,
                         OSAL_MDC_IOCTL_TYPE_MDC_RESTORE_PCI_CONFIG, 0, NULL);
    return (rc);
}
