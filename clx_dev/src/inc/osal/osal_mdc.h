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
#ifndef __KERNEL__
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
#define OSAL_MDC_DEV_FILE_PATH              "/dev/" OSAL_MDC_DRIVER_NAME
#define OSAL_MDC_ISR_TASK_NAME_LEN          (8)
#define OSAL_MDC_ISR_TASK_STACK_SIZE        (64*1024)
#define OSAL_MDC_ISR_TASK_PRI               (99)
#define OSAL_MDC_DMA_NODE_CACHE_NUM         (4)

/* This flag value will be specified when user inserts kernel module. */
#define OSAL_DBG_CRIT            (0x1UL << 0)
#define OSAL_DBG_ERR             (0x1UL << 1)
#define OSAL_DBG_WARN            (0x1UL << 2)
#define OSAL_DBG_INFO            (0x1UL << 3)
#define OSAL_DBG_DEBUG           (0x1UL << 4)
#define OSAL_DBG_TX              (0x1UL << 5)
#define OSAL_DBG_RX              (0x1UL << 6)
#define OSAL_DBG_INTF            (0x1UL << 7)
#define OSAL_DBG_PROFILE         (0x1UL << 8)
#define OSAL_DBG_COMMON          (0x1UL << 9)
#define OSAL_DBG_NETLINK         (0x1UL << 10)

#ifdef __KERNEL__
#define OSAL_PRINT(__flag__, fmt, ...)      do                                  \
{                                                                               \
    if (0 != ((__flag__) & (verbosity)))                                        \
    {                                                                           \
        printk("CLX_KERN %s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
    }                                                                           \
}while (0)
#else
#define OSAL_PRINT(__flag__, fmt, ...)      do                                  \
{                                                                               \
    if (0 != ((__flag__) & (verbosity)))                                        \
    {                                                                           \
        printf("%s:%d: " fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);           \
    }                                                                           \
}while (0)
#endif

#define OSAL_CHECK_PTR(__ptr__) do                                              \
    {                                                                           \
        if (NULL == (__ptr__))                                                  \
        {                                                                       \
            OSAL_PRINT(OSAL_DBG_CRIT,"%s is null pointer\n", #__ptr__);         \
            return (CLX_E_BAD_PARAMETER);                                       \
        }                                                                       \
    } while (0)


/* Signal SDK */
#define SIG_CLX_INTR 44

/* NAMING CONSTANT DECLARATIONS
 */

/* linked list node */
#ifdef __KERNEL__

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

#ifdef __KERNEL__
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

    /* network interface */
    OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_INTF ,
    OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF,
    OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_PROFILE,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT,
    /* driver */
    OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_RX_FREE,
    OSAL_MDC_IOCTL_TYPE_NETIF_WAIT_TX_FREE,     /* waitTxFree(ASYNC) */
    OSAL_MDC_IOCTL_TYPE_NETIF_SET_RX_CFG,       /* setRxConfig       */
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CFG,       /* getRxConfig       */
    OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_TASK,      /* deinitTask        */
    OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_DRV,       /* deinitDrv         */
    OSAL_MDC_IOCTL_TYPE_NETIF_INIT_TASK,        /* initTask          */
    OSAL_MDC_IOCTL_TYPE_NETIF_INIT_DRV,         /* initDrv           */
    /* counter */
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
    OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
    /* port attribute */
    OSAL_MDC_IOCTL_TYPE_NETIF_SET_PORT_ATTR,
    OSAL_MDC_IOCTL_TYPE_NETIF_GET_PORT_ATTR,
    /* netlink */
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_SET_INTF_PROPERTY,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_INTF_PROPERTY,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK,
    OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_NETLINK,

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

CLX_ERROR_NO_T
_osal_mdc_registerIoctlCallback(
    const OSAL_MDC_IOCTL_TYPE_T             type,
    const OSAL_MDC_IOCTL_CALLBACK_FUNC_T    func);
#endif /* End of CLX_LINUX_USER_MODE */

typedef enum
{
    CLX_DEVICE_DAWN = 0,
    CLX_DEVICE_LIGHTNING,
    CLX_DEVICE_NAMCHABARWA,
    CLX_DEVICE_NONE,
} CLX_DEVICE_E;


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

CLX_DEVICE_E clx_get_device_type(
    const UI32_T        unit);
#endif  /* OSAL_MDC_H */
