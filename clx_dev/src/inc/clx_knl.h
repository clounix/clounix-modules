#ifndef CLX_KNL_H
#define CLX_KNL_H
#include <clx_types.h>
#include <osal/osal_mdc.h>
#include <hal/hal_dev.h>

#ifdef __KERNEL__
#include <linux/version.h>
#include <linux/types.h>
#include <linux/compat.h>
#include <linux/wait.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/pci.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/delay.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
#if defined(OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE)
    #define IOREMAP_API(a, b)       ioremap(a, b)
#else
    #define IOREMAP_API(a, b)       ioremap_nocache(a, b)
#endif
#else
    #define IOREMAP_API(a, b)       ioremap(a, b)
#endif
#endif
/* #define OSAL_MDC_EN_MSI */
/* #define OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE */
/* #define OSAL_MDC_EN_TEST */

/* NAMING CONSTANT DECLARATIONS
 */
#define OSAL_MDC_PCI_BAR0_OFFSET            (0x0)
#define OSAL_MDC_PCI_BAR1_OFFSET            (0x1)
#define OSAL_MDC_PCI_BAR2_OFFSET            (0x2)

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef struct
{
    UI32_T                      unit;

    struct pci_dev              *ptr_pci_dev;
    UI32_T                      *ptr_mmio_virt_addr;
    int                         irq;
    AML_DEV_ISR_FUNC_T          isr_callback;
    void                        *ptr_isr_data;

} OSAL_MDC_DEV_T;

typedef struct
{
    /* ISR related */
    OSAL_MDC_DEV_T          dev[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    UI32_T                  dev_num;

    /* Signaling to SDK */
    struct task_struct      *intr_task;

    CLX_THREAD_ID_T         intr_poll_task;
    /* IOCTL and mmap */
    int                     dev_fd;
    /* PCIe MMIO access */
    int                     register_bar[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    void                    *ptr_pci_mmio_base[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    CLX_ADDR_T              pci_mmio_size[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
    /* DMA node management */
    OSAL_MDC_DMA_INFO_T     dma_info;

} OSAL_MDC_CB_T;

#if defined(CLX_LINUX_USER_MODE)
typedef struct
{
    OSAL_MDC_IOCTL_CALLBACK_FUNC_T   callback[OSAL_MDC_IOCTL_TYPE_LAST];

} OSAL_MDC_IOCTL_CB_T;

#if (!defined(CLX_EN_DMA_RESERVED)) && defined(__KERNEL__)
typedef struct
{
    CLX_ADDR_T                  phy_addr;
    UI32_T                      size;
    struct list_head            list;

} OSAL_MDC_USER_MODE_DMA_NODE_T;
#endif

#define NETIF_KNL_DEVICE_IS_NAMCHABARWA(__dev_id__)                (HAL_DEVICE_ID_EDK1100 == (__dev_id__ & 0xFF00))
#define NETIF_KNL_DEVICE_IS_LIGHTNING(__dev_id__)         (HAL_DEVICE_ID_CL8500 == (__dev_id__ & 0xFF00))
#define NETIF_KNL_DEVICE_IS_DAWN(__dev_id__)              (HAL_DEVICE_ID_CL8300 == (__dev_id__ & 0xFF00))
#endif

#endif //CLX_KNL_H
