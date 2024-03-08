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

/* FILE NAME:  osal_mdc.h
 * PURPOSE:
 * 1. Provide device operate from AML interface
 * NOTES:
 *
 */

#ifndef OSAL_MDC_H
#define OSAL_MDC_H

/* INCLUDE FILE DECLARATIONS */
#if !defined(CLX_LINUX_KERNEL_MODE)
#include <cmlib/cmlib_list.h>
#endif
#include <clx_types.h>
#define CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM    (16)
#include <aml/aml.h>

#define OSAL_MDC_DRIVER_NAME                "clx_dev"
#define OSAL_MDC_DRIVER_MISC_MAJOR_NUM      (10)
#define OSAL_MDC_DRIVER_MISC_MINOR_NUM      (250)
#define OSAL_MDC_PCI_BUS_WIDTH              (4)

#define OSAL_MDC_DMA_LIST_SZ_UNLIMITED      (0)
#define OSAL_MDC_DMA_LIST_NAME              "RSRV_DMA"
#define OSAL_MDC_DMA_SEMAPHORE_NAME         "DMALIST"

/* NAMING CONSTANT DECLARATIONS
 */

/* linked list node */
#if defined(CLX_LINUX_KERNEL_MODE)

typedef struct OSAL_MDC_LIST_NODE_S
{
    void                        *ptr_data;       /* node data                   */
    struct OSAL_MDC_LIST_NODE_S *ptr_next;       /* point to next link node     */
    struct OSAL_MDC_LIST_NODE_S *ptr_prev;       /* point to previous link node */
} OSAL_MDC_LIST_NODE_T;

/* linked list head */
typedef struct OSAL_MDC_LIST_S
{
    OSAL_MDC_LIST_NODE_T    *ptr_head_node;       /* linked list head node   */
    OSAL_MDC_LIST_NODE_T    *ptr_tail_node;       /* linked list tail node   */
    UI32_T                  capacity;             /* max count of nodes in list
                                                   * size=0: the capacity is unlimited.
                                                   * size>0: the capacity is limited.
                                                   */
    UI32_T                  node_cnt;             /* the count of nodes in the list */
} OSAL_MDC_LIST_T;

#endif /* End of defined(CLX_LINUX_KERNEL_MODE) */

typedef struct
{
    CLX_ADDR_T          phy_addr;
    void                *ptr_virt_addr;
    CLX_ADDR_T          size;
    CLX_ADDR_T          bus_addr;

#if defined(CLX_EN_DMA_RESERVED)
    BOOL_T              available;
#endif

} OSAL_MDC_DMA_NODE_T;

typedef struct
{
#if defined(CLX_EN_DMA_RESERVED)
    void                *ptr_rsrv_virt_addr;
    CLX_ADDR_T          rsrv_phy_addr;
    CLX_ADDR_T          rsrv_size;
#else
    struct device       *ptr_dma_dev;       /* for allocate/free system memory */
#endif

#if defined(CLX_LINUX_KERNEL_MODE)
    OSAL_MDC_LIST_T     *ptr_dma_list;
#else
    CMLIB_LIST_T        *ptr_dma_list;
#endif

    CLX_SEMAPHORE_ID_T  sema;

} OSAL_MDC_DMA_INFO_T;

#if defined(CLX_LINUX_USER_MODE)

/* Data type of IOCTL argument for DMA management */
typedef struct
{
#if defined(CLX_EN_DMA_RESERVED)
    CLX_ADDR_T              rsrv_dma_phy_addr;  /* information of reserved memory */
    CLX_ADDR_T              rsrv_dma_size;
#else
    CLX_ADDR_T              phy_addr;           /* information of system memory */
    CLX_ADDR_T              size;
    CLX_ADDR_T              bus_addr;
#endif
} OSAL_MDC_IOCTL_DMA_DATA_T;

/* Data type of IOCTL argument for device initialization */
#pragma  pack (push,1)
typedef struct
{
    AML_DEV_ID_T            id[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    CLX_ADDR_T              pci_mmio_phy_start[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    CLX_ADDR_T              pci_mmio_size[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    UI32_T                  dev_num;
} OSAL_MDC_IOCTL_DEV_DATA_T;
#pragma pack (pop)

typedef enum
{
    OSAL_MDC_IOCTL_ACCESS_READ = 0,
    OSAL_MDC_IOCTL_ACCESS_WRITE,
    OSAL_MDC_IOCTL_ACCESS_READ_WRITE,
    OSAL_MDC_IOCTL_ACCESS_NONE,
    OSAL_MDC_IOCTL_ACCESS_LAST

} OSAL_MDC_IOCTL_ACCESS_T;

typedef enum
{
    OSAL_MDC_IOCTL_TYPE_MDC_INIT_DEV = 0,
    OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_DEV,
    OSAL_MDC_IOCTL_TYPE_MDC_INIT_RSRV_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_RSRV_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_ALLOC_SYS_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
    OSAL_MDC_IOCTL_TYPE_MDC_CONNECT_ISR,
    OSAL_MDC_IOCTL_TYPE_MDC_DISCONNECT_ISR,
    OSAL_MDC_IOCTL_TYPE_MDC_SAVE_PCI_CONFIG,
    OSAL_MDC_IOCTL_TYPE_MDC_RESTORE_PCI_CONFIG,
    OSAL_MDC_IOCTL_TYPE_LAST

} OSAL_MDC_IOCTL_TYPE_T;

typedef union
{
    UI32_T      value;
    struct
    {
        UI32_T  access      : 2;    /* 0:read, 1:write, 2:read and write, 3:none */
        UI32_T  unit        : 6;    /* Maximum unit number is 64.                */
        UI32_T  size        :14;    /* Maximum IOCTL data size is 16KB.          */
        UI32_T  type        :10;    /* Maximum 1024 IOCTL types                  */
    } field;
} OSAL_MDC_IOCTL_CMD_T;

typedef CLX_ERROR_NO_T
(*OSAL_MDC_IOCTL_CALLBACK_FUNC_T)(
    const UI32_T        unit,
    void                *ptr_data);

#endif /* End of CLX_LINUX_USER_MODE */


/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
CLX_ERROR_NO_T
osal_mdc_readPciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    UI32_T              *ptr_data,
    const UI32_T        len);

CLX_ERROR_NO_T
osal_mdc_writePciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    const UI32_T        *ptr_data,
    const UI32_T        len);

CLX_ERROR_NO_T
osal_mdc_initDevice(
    AML_DEV_T           *ptr_dev_list,
    UI32_T              *ptr_dev_num);

CLX_ERROR_NO_T
osal_mdc_deinitDevice(void);

CLX_ERROR_NO_T
osal_mdc_initDmaMem(void);

CLX_ERROR_NO_T
osal_mdc_deinitDmaMem(void);

void *
osal_mdc_allocDmaMem(
    const UI32_T        size);

CLX_ERROR_NO_T
osal_mdc_freeDmaMem(
    void                *ptr_virt_addr);

CLX_ERROR_NO_T
osal_mdc_convertVirtToPhy(
    void                *ptr_virt_addr,
    CLX_ADDR_T          *ptr_phy_addr);

CLX_ERROR_NO_T
osal_mdc_convertPhyToVirt(
    const CLX_ADDR_T    phy_addr,
    void                **pptr_virt_addr);

CLX_ERROR_NO_T
osal_mdc_registerIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    void                *ptr_cookie);

CLX_ERROR_NO_T
osal_mdc_connectIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    AML_DEV_ISR_DATA_T  *ptr_cookie);

CLX_ERROR_NO_T
osal_mdc_disconnectIsr(
    const UI32_T        unit);

CLX_ERROR_NO_T
osal_mdc_flushCache(
    void                *ptr_virt_addr,
    const UI32_T        size);

CLX_ERROR_NO_T
osal_mdc_invalidateCache(
    void                *ptr_virt_addr,
    const UI32_T        size);

CLX_ERROR_NO_T
osal_mdc_savePciConfig(
    const UI32_T        unit);

CLX_ERROR_NO_T
osal_mdc_restorePciConfig(
    const UI32_T        unit);

#endif  /* OSAL_MDC_H */
