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

// #include <clx_knl.h>
#include <clx_error.h>
#include <clx_types.h>
#include <osal/osal_mdc.h>
#include <hal/common/hal_dev.h>
#include <linux/sched/signal.h>

#if defined(CLX_LINUX_USER_MODE)
#include <linux/miscdevice.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,12,0)
#include <linux/uaccess.h>
#else
#include <asm/uaccess.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
#if defined(OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE)
    #define IOREMAP_API(a, b)       ioremap(a, b)
#else
    #define IOREMAP_API(a, b)       ioremap_nocache(a, b)
#endif
#else
    #define IOREMAP_API(a, b)       ioremap(a, b)
#endif

#include <osal/netif_common.h>
#include <osal/netif_nb_pkt.h>


#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/semaphore.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/bitmap.h>
#include <linux/list.h>
#endif

// UI32_T     verbosity = (HAL_DBG_CRIT | HAL_DBG_ERR | HAL_DBG_WARN );
UI32_T     verbosity = 0xfff;

/* IOCTL */
static OSAL_MDC_IOCTL_CB_T      _osal_mdc_ioctl_cb;
static AML_DEV_T                _osal_mdc_ioctl_dev[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM] = {};

OSAL_MDC_CB_T                   _osal_mdc_cb;
struct pci_dev                  *_ptr_ext_pci_dev;

/* Interface */
static UI32_T                   _osal_mdc_devInited = 0;


static UI32_T                   _osal_mdc_isr_init_bitmap = 0;  /* To record the dev request_irq */
static UI32_T                   _osal_mdc_isr_dev_bitmap;       /* To record the dev bitmap      */
static spinlock_t               _osal_mdc_isr_dev_bitmap_lock;
static wait_queue_head_t        _osal_mdc_isr_wait;
static UI32_T                   _osal_mdc_isr_mask_addr;
static UI32_T                   _osal_mdc_isr_mask_val;

static struct list_head         _osal_mdc_sysDmaList[2];       /* To avoid memory corruption when cold-boot */
static UI32_T                   _osal_mdc_sysCurDmaListIdx = 0;


static struct pci_device_id _osal_mdc_id_table[] =
{
    {PCI_DEVICE(HAL_CLX_VENDOR_ID, PCI_ANY_ID)},
    {PCI_DEVICE(HAL_CL_VENDOR_ID, PCI_ANY_ID)},
    {PCI_DEVICE(HAL_CLX_EDK_VENDOR_ID, HAL_DEVICE_ID_EDK1111)},
};

CLX_DEVICE_E clx_get_device_type(
    const UI32_T        unit)
{
    CLX_DEVICE_E device = CLX_DEVICE_NONE;
    switch (_osal_mdc_ioctl_dev[unit].id.device & 0xFF00)
    {
        case HAL_DEVICE_ID_EDK1100:
            device = CLX_DEVICE_NB;
            break;
        case HAL_DEVICE_ID_CL8500:
            device = CLX_DEVICE_LIGHTNING;
            break;
        case HAL_DEVICE_ID_CL8300:
            device = CLX_DEVICE_DAWN;
            break;    
        default:
            DIAG_PRINT(HAL_DBG_ERR,"Wrong device id:0x%x\n",_osal_mdc_ioctl_dev[unit].id.device);
            break;
    }
    return device;
}


static CLX_ERROR_NO_T
_osal_mdc_getPciMmioInfo(
    const UI32_T        unit,
    struct pci_dev      *pdev,
    UI32_T              **pptr_base_addr)
{
    CLX_ERROR_NO_T      rc = CLX_E_OTHERS;
    CLX_ADDR_T          phy_addr;
    UI32_T              reg_space_sz;

    phy_addr     = pci_resource_start(pdev, _osal_mdc_cb.register_bar[unit]);
    reg_space_sz = pci_resource_len(pdev, _osal_mdc_cb.register_bar[unit]);

    if (0 == pci_request_region(pdev, _osal_mdc_cb.register_bar[unit], OSAL_MDC_DRIVER_NAME))
    {
        *pptr_base_addr = IOREMAP_API(phy_addr, reg_space_sz);
        if (NULL != *pptr_base_addr)
        {
            rc = CLX_E_OK;
        }
    }
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_readPciReg(
    const UI32_T        unit,
    const UI32_T        offset,
    UI32_T              *ptr_data,
    const UI32_T        len)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    UI32_T              idx;
    UI32_T              count;
    volatile UI32_T     *ptr_base_addr = _osal_mdc_cb.dev[unit].ptr_mmio_virt_addr;

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
    volatile UI32_T     *ptr_base_addr = _osal_mdc_cb.dev[unit].ptr_mmio_virt_addr;
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

static int
_osal_mdc_probePciCallback(
    struct pci_dev                  *pdev,
    const struct pci_device_id      *id)
{
    int                 linux_rc;
    UI16_T              device_id;
    UI16_T              vendor_id;
    UI8_T               revision_id;
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    AML_DEV_T           *_ptr_osal_mdc_dev = &_osal_mdc_ioctl_dev[_osal_mdc_cb.dev_num];

    linux_rc = pci_enable_device(pdev);
    if (0 == linux_rc)
    {
        _ptr_osal_mdc_dev->if_type = AML_DEV_TYPE_PCI;

        pci_read_config_word(pdev, PCI_DEVICE_ID, &device_id);
        pci_read_config_word(pdev, PCI_VENDOR_ID, &vendor_id);
        pci_read_config_byte(pdev, PCI_REVISION_ID, &revision_id);

        _ptr_osal_mdc_dev->id.device   = (UI32_T)device_id;
        _ptr_osal_mdc_dev->id.vendor   = (UI32_T)vendor_id;
        _ptr_osal_mdc_dev->id.revision = (UI32_T)revision_id;

#if defined(CLX_LINUX_KERNEL_MODE)
        _ptr_osal_mdc_dev->access.read_callback  = osal_mdc_readPciReg;
        _ptr_osal_mdc_dev->access.write_callback = osal_mdc_writePciReg;
#endif

        if(NETIF_KNL_DEVICE_IS_LIGHTNING(device_id) || NETIF_KNL_DEVICE_IS_DAWN(device_id)) 
        {
            if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32))) 
            {
                DIAG_PRINT(HAL_DBG_ERR,"dma_set_mask_and_coherent failed");
            }
            _osal_mdc_cb.register_bar[_osal_mdc_cb.dev_num] = OSAL_MDC_PCI_BAR0_OFFSET;
        }
        else if (NETIF_KNL_DEVICE_IS_NB(device_id))
        {
            if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48))) 
            {
                DIAG_PRINT(HAL_DBG_ERR,"dma_set_mask_and_coherent failed");
            }
            _osal_mdc_cb.register_bar[_osal_mdc_cb.dev_num] = OSAL_MDC_PCI_BAR2_OFFSET;
        }
        
        rc = _osal_mdc_getPciMmioInfo(_osal_mdc_cb.dev_num,pdev,&_osal_mdc_cb.dev[_osal_mdc_cb.dev_num].ptr_mmio_virt_addr);
        if (CLX_E_OK == rc)
        {
            /* Save the database to pdev structure for system callback to release recource,
             * such like disconnecting ISR etc.
             */
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].irq          = pdev->irq;
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].ptr_pci_dev  = pdev;
            _osal_mdc_cb.dev[_osal_mdc_cb.dev_num].unit         = _osal_mdc_cb.dev_num;

            pci_set_drvdata(pdev, &_osal_mdc_cb.dev[_osal_mdc_cb.dev_num]);

            /* To set the bus master bit on device to enable the DMA transaction from PCIe EP to RC
             * The bus master bit gets cleared when pci_disable_device() is called
             */
            pci_set_master(pdev);

#if !defined(CLX_EN_DMA_RESERVED)
            if (NULL == _osal_mdc_cb.dma_info.ptr_dma_dev)
            {
                /* This variable is for dma_alloc_coherent */
                _osal_mdc_cb.dma_info.ptr_dma_dev = &pdev->dev;
            }
#endif
            _osal_mdc_cb.dev_num++;
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR, "enable pci dev failed, linux_rc=%d\n", linux_rc);
    }

    return (0);
}

static void
_osal_mdc_removePciCallback(
    struct pci_dev      *pdev)
{
    OSAL_MDC_DEV_T      *ptr_dev = (OSAL_MDC_DEV_T *)pci_get_drvdata(pdev);

    iounmap(ptr_dev->ptr_mmio_virt_addr);
    pci_release_region(pdev, _osal_mdc_cb.register_bar[_osal_mdc_cb.dev_num]);
    pci_disable_device(pdev);
    _osal_mdc_cb.dev_num--;
}

static struct pci_driver    _osal_mdc_pci_driver =
{
    .name     = OSAL_MDC_DRIVER_NAME,
    .id_table = _osal_mdc_id_table,
    .probe    = _osal_mdc_probePciCallback,
    .remove   = _osal_mdc_removePciCallback,
};

static CLX_ERROR_NO_T
_osal_mdc_probePciDevice(void)
{
    CLX_ERROR_NO_T  rc = CLX_E_OK;

    if (pci_register_driver(&_osal_mdc_pci_driver) < 0)
    {
        DIAG_PRINT(HAL_DBG_ERR,"Cannot find PCI device\n");
        rc = CLX_E_OTHERS;
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_removePciDevice(void)
{
    pci_unregister_driver(&_osal_mdc_pci_driver);
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_tunePciPerf(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_ep_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    struct pci_dev      *ptr_rc_dev = ptr_ep_dev->bus->self;
    int                 ext_cap = 0;
    UI32_T              data_32 = 0;
    UI16_T              data_16 = 0;

    ext_cap = pci_find_ext_capability(ptr_rc_dev, PCI_EXT_CAP_ID_SECPCI);
    if (0 == ext_cap)
    {
        DIAG_PRINT(HAL_DBG_ERR, "Cannot find PCI cap PCI_EXT_CAP_ID_SECPCI\n");
        return CLX_E_OTHERS;
    }

    /* disable response timer */
    osal_mdc_readPciReg(unit, 0x3EE16C, &data_32, sizeof(UI32_T));
#if defined(CLX_EN_BIG_ENDIAN)
    data_32 &= 0x0000FFFF;
#else
    data_32 &= 0xFFFF0000;
#endif
    osal_mdc_writePciReg(unit, 0x3EE16C, &data_32, sizeof(UI32_T));

    /* Link Control 3 Register bit0: Perform Equalization*/
    pci_read_config_word(ptr_rc_dev, ext_cap + 0x4, &data_16);
    data_16 |= 0x1;
    pci_write_config_word(ptr_rc_dev, ext_cap + 0x4, data_16);

    /* Link Control Register bit5: Retrain Link */
    pci_read_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0x10, &data_16);
    data_16 |= 0x20;
    pci_write_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0x10, data_16);

    msleep(100);

    /* clear */
    pci_read_config_word(ptr_rc_dev, ext_cap + 0x4, &data_16);
    data_16 &= ~0x1;
    pci_write_config_word(ptr_rc_dev, ext_cap + 0x4, data_16);

    return CLX_E_OK;
}

static CLX_ERROR_NO_T
_osal_mdc_maskStatus(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_ep_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    struct pci_dev      *ptr_rc_dev = ptr_ep_dev->bus->self;
    int                 ext_cap = 0;
    UI32_T              data_32 = 0;

    ext_cap = pci_find_ext_capability(ptr_rc_dev, PCI_EXT_CAP_ID_ERR);
    if (0 != ext_cap)
    {
        /* Mask */
        pci_read_config_dword(ptr_rc_dev, ext_cap + 0x8, &data_32);
        data_32 |= 0x20;
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x8, data_32);
    }

    return CLX_E_OK;
}

static CLX_ERROR_NO_T
_osal_mdc_clearStatus(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_ep_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    struct pci_dev      *ptr_rc_dev = ptr_ep_dev->bus->self;
    int                 ext_cap = 0;
    UI32_T              data_32 = 0;

    ext_cap = pci_find_ext_capability(ptr_rc_dev, PCI_EXT_CAP_ID_ERR);
    if (0 != ext_cap)
    {
        /* Clear */
        pci_write_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0xa, 0x04);
        pci_write_config_word(ptr_rc_dev, ptr_rc_dev->pcie_cap + 0x12, 0x8000);
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x4, 0x20);

        /* UnMask */
        pci_read_config_dword(ptr_rc_dev, ext_cap + 0x8, &data_32);
        data_32 &= ~0x20;
        pci_write_config_dword(ptr_rc_dev, ext_cap + 0x8, data_32);
    }

    return CLX_E_OK;
}

static CLX_ERROR_NO_T
_osal_mdc_savePciConfig(
    const UI32_T        unit)
{
    struct pci_dev      *ptr_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    rc = _osal_mdc_maskStatus(unit);
    if (CLX_E_OK == rc)
    {
        pci_save_state(ptr_dev);
    }

    return rc;
}

static CLX_ERROR_NO_T
_osal_mdc_restorePciConfig(
    const UI32_T        unit)
{
#define OSAL_MDC_PCI_PRESENT_POLL_CNT           (100)
#define OSAL_MDC_PCI_PRESENT_POLL_INTERVAL      (10)   /* ms */

    struct pci_dev      *ptr_dev = _osal_mdc_cb.dev[unit].ptr_pci_dev;
    UI32_T              poll_cnt = 0;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    /* standard: at least 100ms for link recovery */
    msleep(100);

    /* make sure pci device is there before restoring the config space */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
    while ((0 == pci_device_is_present(ptr_dev)) &&
#else
    while ((0 == pci_dev_present(_osal_mdc_id_table)) &&
#endif
           (poll_cnt < OSAL_MDC_PCI_PRESENT_POLL_CNT))
    {
        msleep(OSAL_MDC_PCI_PRESENT_POLL_INTERVAL);
        poll_cnt++;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 13, 0)
    if (1 != pci_device_is_present(ptr_dev))
#else
    if (1 != pci_dev_present(_osal_mdc_id_table))
#endif
    {
        DIAG_PRINT(HAL_DBG_ERR, "detect pci device failed\n");
        return CLX_E_OTHERS;
    }

    pci_restore_state(ptr_dev);
    rc = _osal_mdc_clearStatus(unit);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "Failed to clear pci status\n");
        return rc;
    }

    if(NETIF_KNL_DEVICE_IS_LIGHTNING(ptr_dev->device))
    {
        rc = _osal_mdc_tunePciPerf(unit);
    }

    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_tunePciDevice(
    AML_DEV_T           *ptr_dev,
    const UI32_T        dev_num)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    UI32_T              idx = 0;

    for (idx = 0; (idx < dev_num) && (CLX_E_OK == rc); idx++)
    {
        if(NETIF_KNL_DEVICE_IS_LIGHTNING(ptr_dev[idx].id.device))
        {
            rc = _osal_mdc_tunePciPerf(idx);
        }
    }

    return rc;
}


CLX_ERROR_NO_T
osal_mdc_initDevice(
    AML_DEV_T           *ptr_dev_list,
    UI32_T              *ptr_dev_num)
{
    OSAL_MDC_CB_T       *ptr_cb = &_osal_mdc_cb;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    memset(ptr_cb, 0x0, sizeof(OSAL_MDC_CB_T));

    rc = _osal_mdc_probePciDevice();
    *ptr_dev_num = ptr_cb->dev_num;

    if (CLX_E_OK == rc)
    {
        rc = _osal_mdc_tunePciDevice(ptr_dev_list, *ptr_dev_num);
        _ptr_ext_pci_dev = _osal_mdc_cb.dev[0].ptr_pci_dev;
    }

    return (rc);
}


static CLX_ERROR_NO_T
_osal_mdc_clearSysDmaList(
    UI32_T          dmaListIdx)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_curr_node_data = NULL;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_next_node_data = NULL;

    list_for_each_entry_safe(ptr_curr_node_data, ptr_next_node_data,
                             &_osal_mdc_sysDmaList[dmaListIdx], list)
    {
        list_del(&(ptr_curr_node_data->list));
        dma_free_coherent(ptr_dma_info->ptr_dma_dev,
                          ptr_curr_node_data->size, 
                          phys_to_virt(ptr_curr_node_data->phy_addr),
                          ptr_curr_node_data->phy_addr);
        kfree(ptr_curr_node_data);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_getPciInfoToIoctlData(
    const OSAL_MDC_DEV_T        *ptr_dev_list,
    OSAL_MDC_IOCTL_DEV_DATA_T   *ptr_dev_data)
{
    UI32_T  idx;

    /* Search for PCIe device and get the MMIO base address. */
    for (idx = 0; idx < _osal_mdc_cb.dev_num; idx++)
    {
        if (NULL != ptr_dev_list[idx].ptr_pci_dev)
        {
            ptr_dev_data->pci_mmio_phy_start[idx] =
                pci_resource_start(ptr_dev_list[idx].ptr_pci_dev, _osal_mdc_cb.register_bar[idx]);

            ptr_dev_data->pci_mmio_size[idx] =
                pci_resource_len(ptr_dev_list[idx].ptr_pci_dev, _osal_mdc_cb.register_bar[idx]);
        }
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_getDeviceIdToIoctlData(
    AML_DEV_T                   *ptr_dev,
    OSAL_MDC_IOCTL_DEV_DATA_T   *ptr_dev_data,
    const UI32_T                dev_num)
{
    UI32_T  idx;

    for (idx = 0; idx < ptr_dev_data->dev_num; idx++)
    {
        ptr_dev_data->id[idx].device   = ptr_dev[idx].id.device;
        ptr_dev_data->id[idx].vendor   = ptr_dev[idx].id.vendor;
        ptr_dev_data->id[idx].revision = ptr_dev[idx].id.revision;
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_initDeviceCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_CB_T                   *ptr_cb = &_osal_mdc_cb;
    OSAL_MDC_DEV_T                  *ptr_dev_list = _osal_mdc_cb.dev;
    OSAL_MDC_IOCTL_DEV_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DEV_DATA_T *)ptr_data;

    /* "dev" is just created for invoking  osal_mdc_initDevice,
     * it is no use once the device IDs are copy to ptr_ioctl_data.
     */

    CLX_ERROR_NO_T                  rc = CLX_E_OK;

    if (0 == _osal_mdc_devInited)
    {
        rc = osal_mdc_initDevice(_osal_mdc_ioctl_dev, &ptr_ioctl_data->dev_num);
    }
    else
    {
        /* ptr_cb->dev_num was initialized in osal_mdc_initDevice(); */
        ptr_ioctl_data->dev_num = ptr_cb->dev_num;
    }

    if (ptr_cb->dev_num >= CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM)
    {
        DIAG_PRINT(HAL_DBG_ERR,"dev num=%d > max support num=%d\n",
                     ptr_ioctl_data->dev_num, CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM);
    }

#if !defined(CLX_EN_DMA_RESERVED)
    if (0 == _osal_mdc_devInited)
    {
        /* Create two DMA memory lists and use 1st. */
        INIT_LIST_HEAD(&_osal_mdc_sysDmaList[0]);
        INIT_LIST_HEAD(&_osal_mdc_sysDmaList[1]);
        _osal_mdc_sysCurDmaListIdx = 0;
    }
    else
    {
        /* Delay free the old list until the chip is reset.
         * When we kill the process, the chip continues to write to the DMA memory.
         * If we free the old DMA memory before stopping the chip, there could be memory corruption.
         */
        _osal_mdc_sysCurDmaListIdx = ((_osal_mdc_sysCurDmaListIdx + 1) & 0x1);
        rc = _osal_mdc_clearSysDmaList(_osal_mdc_sysCurDmaListIdx);
    }
#endif

    if (CLX_E_OK == rc)
    {
        rc = _osal_mdc_getDeviceIdToIoctlData(_osal_mdc_ioctl_dev, ptr_ioctl_data, ptr_ioctl_data->dev_num);
    }
    if (CLX_E_OK == rc)
    {
        rc = _osal_mdc_getPciInfoToIoctlData(ptr_dev_list, ptr_ioctl_data);
    }

    _osal_mdc_devInited = 1;
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_deinitDevice(void)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    if (NULL != _ptr_ext_pci_dev)
    {
        rc = _osal_mdc_removePciDevice();
        _ptr_ext_pci_dev = NULL;
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_deinitDeviceCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    CLX_ERROR_NO_T  rc = CLX_E_OK;

#if !defined(CLX_EN_DMA_RESERVED)
    _osal_mdc_clearSysDmaList(0);
    _osal_mdc_clearSysDmaList(1);
#endif

    if (0 != _osal_mdc_devInited)
    {
        hal_netif_pkt_exit(0);
        rc = osal_mdc_deinitDevice();
        _osal_mdc_devInited = 0;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_allocSysDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_IOCTL_DMA_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DMA_DATA_T *)ptr_data;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_node_data = NULL;
    void *virt_addr;

/* To defense the compatible data type of 32bit and 64bit are not synchronized */
#if defined(CONFIG_ARCH_DMA_ADDR_T_64BIT)    /* Bus addressing is 64-bit */\
&& !defined(CLX_EN_64BIT_ADDR)               /* SDK follows HOST with 32-bit addr*/\
&& (defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN) || defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN))/* HOST is 32-bit */
#error "The DMA address of OS is 64bit. Please enable CLX_EN_64BIT_ADDR in SDK."
#endif

#if !defined(CONFIG_ARCH_DMA_ADDR_T_64BIT) && defined(CLX_EN_64BIT_ADDR)
#error "The DMA address of OS is not 64bit. Please disable CLX_EN_64BIT_ADDR in SDK."
#endif

	ptr_ioctl_data->size = round_up(ptr_ioctl_data->size, PAGE_SIZE);    
    virt_addr = dma_alloc_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                                   (dma_addr_t *)&ptr_ioctl_data->phy_addr, GFP_KERNEL | GFP_DMA32);
    if (virt_addr == NULL)
    {
        return (CLX_E_NO_MEMORY);
    }

    ptr_node_data = kmalloc(sizeof(OSAL_MDC_USER_MODE_DMA_NODE_T), GFP_KERNEL);
    if (NULL != ptr_node_data)
    {
        memset(ptr_node_data, 0, sizeof(OSAL_MDC_USER_MODE_DMA_NODE_T));
        ptr_node_data->phy_addr      = ptr_ioctl_data->phy_addr;
        ptr_node_data->size          = ptr_ioctl_data->size;
        list_add(&(ptr_node_data->list), &_osal_mdc_sysDmaList[_osal_mdc_sysCurDmaListIdx]);
        ptr_ioctl_data->phy_addr      = virt_to_phys(virt_addr);
    }
    else
    {
        dma_free_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                          virt_addr, ptr_ioctl_data->phy_addr);

        return (CLX_E_NO_MEMORY);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_freeSysDmaMemCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    OSAL_MDC_DMA_INFO_T             *ptr_dma_info = &_osal_mdc_cb.dma_info;
    OSAL_MDC_IOCTL_DMA_DATA_T       *ptr_ioctl_data = (OSAL_MDC_IOCTL_DMA_DATA_T *)ptr_data;

    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_curr_node_data = NULL;
    OSAL_MDC_USER_MODE_DMA_NODE_T   *ptr_next_node_data = NULL;


    list_for_each_entry_safe(ptr_curr_node_data, ptr_next_node_data,
                             &_osal_mdc_sysDmaList[_osal_mdc_sysCurDmaListIdx], list)
    {
        if (ptr_curr_node_data->phy_addr == ptr_ioctl_data->phy_addr)
        {
            list_del(&(ptr_curr_node_data->list));
            kfree(ptr_curr_node_data);
            break;
        }
    }

    dma_free_coherent(ptr_dma_info->ptr_dma_dev, ptr_ioctl_data->size,
                      phys_to_virt(ptr_ioctl_data->phy_addr), ptr_ioctl_data->phy_addr);

    return (CLX_E_OK);
}

static inline CLX_ERROR_NO_T
_osal_mdc_initInterrupt(void)
{
    /* init top and bottom halves */
    init_waitqueue_head(&_osal_mdc_isr_wait);

    /* init lock and clear device bitmap */
    spin_lock_init(&_osal_mdc_isr_dev_bitmap_lock);
    _osal_mdc_isr_dev_bitmap = 0;
    _osal_mdc_isr_init_bitmap = 0;

    /* clear chip interrupt mask address and value */
    _osal_mdc_isr_mask_addr = 0;
    _osal_mdc_isr_mask_val = 0;

    return (CLX_E_OK);
}


#define SIG_CLX_INTR 44
static inline CLX_ERROR_NO_T
_osal_mdc_notifyUserProcess(
    const UI32_T        unit)
{
    unsigned long       flags = 0;
    struct siginfo info;

    /* mask chip interrupt */
    osal_mdc_writePciReg(unit, _osal_mdc_isr_mask_addr,
        &_osal_mdc_isr_mask_val, sizeof(UI32_T));

    /* set the device bitmap. */
    spin_lock_irqsave(&_osal_mdc_isr_dev_bitmap_lock, flags);
    _osal_mdc_isr_dev_bitmap |= (1U << unit);
    spin_unlock_irqrestore(&_osal_mdc_isr_dev_bitmap_lock, flags);

    //Sending info to SDK
    memset(&info, 0, sizeof(struct siginfo));
    info.si_signo = SIG_CLX_INTR;
    info.si_code = SI_QUEUE;
    info.si_int = _osal_mdc_isr_dev_bitmap;

    if (_osal_mdc_cb.intr_task != NULL) {
        DIAG_PRINT(HAL_DBG_DEBUG,"Sending signal to pid:%d\n",_osal_mdc_cb.intr_task->pid);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,20,0)
        if(send_sig_info(SIG_CLX_INTR, (struct kernel_siginfo *)&info, _osal_mdc_cb.intr_task) < 0) {
#else
        if(send_sig_info(SIG_CLX_INTR, &info, _osal_mdc_cb.intr_task) < 0) {
#endif
            DIAG_PRINT(HAL_DBG_ERR,"Unable to send signal\n");
            return (CLX_E_OP_INCOMPLETE);
        }
    }
    return (CLX_E_OK);
}

static irqreturn_t
_osal_mdc_systemIntrCallback(
    int                 irq,
    void                *ptr_cookie)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    int                 linux_rc = IRQ_HANDLED;
    OSAL_MDC_DEV_T      *ptr_dev = (OSAL_MDC_DEV_T *)ptr_cookie;

    /* Invoke kernel callback, the callback function exist only in below cases:
     * 1. SDK built in kernel mode
     * 2. SDK built in user mode, NetIF kernel module is enabled
     */
    if (NULL != ptr_dev->isr_callback)
    {
        rc = ptr_dev->isr_callback(ptr_dev->ptr_isr_data);
        if (CLX_E_OK != rc)
        {
            DIAG_PRINT(HAL_DBG_ERR, "handle irq failed, rc=%d\n", rc);
            linux_rc = IRQ_NONE;
        }
    }

#if defined(CLX_LINUX_USER_MODE)
    /* Notify user process */
    rc = _osal_mdc_notifyUserProcess(ptr_dev->unit);
    if (CLX_E_OK != rc)
    {
        DIAG_PRINT(HAL_DBG_ERR, "notify intr to usr failed, rc=%d\n", rc);
        linux_rc = IRQ_NONE;
    }
#endif

    return (linux_rc);
}

CLX_ERROR_NO_T
osal_mdc_registerIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    void                *ptr_cookie)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];

    ptr_dev->isr_callback = handler;
    ptr_dev->ptr_isr_data = (void *)((CLX_HUGE_T)unit);

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
osal_mdc_connectIsr(
    const UI32_T        unit,
    AML_DEV_ISR_FUNC_T  handler,
    AML_DEV_ISR_DATA_T  *ptr_cookie)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];
    int                 linux_rc = 0;
    CLX_ERROR_NO_T      rc = CLX_E_OK;

#if defined(CLX_LINUX_USER_MODE)
    if (NULL != ptr_cookie)
    {
        _osal_mdc_isr_mask_addr = ptr_cookie->mask_addr;
        _osal_mdc_isr_mask_val  = ptr_cookie->mask_val;
    }
#endif

    if (NULL == ptr_dev->isr_callback)
    {
#if defined(CLX_LINUX_KERNEL_MODE)
        /* In user mode, the following database is created in user space. */
        ptr_dev->isr_callback = handler;
        ptr_dev->ptr_isr_data = (void *)((CLX_HUGE_T)unit);
#endif

#if defined(OSAL_MDC_EN_MSI)
        /* If "no_msi" flag is set, it means the device doesn't support MSI.  */
        if (1 != ptr_dev->ptr_pci_dev->no_msi)
        {
            linux_rc = pci_enable_msi(ptr_dev->ptr_pci_dev);
            if (0 == linux_rc)
            {
                /* The system gives a new irq number if MSI is enabled sucessfully. */
                ptr_dev->irq = ptr_dev->ptr_pci_dev->irq;
            }
            else
            {
                DIAG_PRINT(HAL_DBG_ERR, "pci_enable_msi() failed, rc=%d\n", linux_rc);
                rc = CLX_E_OTHERS;
            }
        }
#endif

        linux_rc = request_irq(ptr_dev->irq, _osal_mdc_systemIntrCallback,
                               0, OSAL_MDC_DRIVER_NAME, (void *)ptr_dev);

        if (0 != linux_rc)
        {
            DIAG_PRINT(HAL_DBG_ERR, "request_irq() failed, rc=%d\n", linux_rc);
            rc = CLX_E_OTHERS;
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR, "double req isr err\n");
        rc = CLX_E_OTHERS;
    }
    return (rc);
}

CLX_ERROR_NO_T
osal_mdc_disconnectIsr(
    const UI32_T        unit)
{
    OSAL_MDC_DEV_T      *ptr_dev = &_osal_mdc_cb.dev[unit];

    free_irq(ptr_dev->irq, (void *)ptr_dev);

#if defined(OSAL_MDC_EN_MSI)
    /* Must free the irq before disabling MSI */
    pci_disable_msi(ptr_dev->ptr_pci_dev);
#endif

#if defined(CLX_LINUX_KERNEL_MODE)
    ptr_dev->isr_callback = NULL;
    ptr_dev->ptr_isr_data = NULL;
#endif

#if defined(CLX_LINUX_USER_MODE)
    _osal_mdc_isr_mask_addr = 0x0;
    _osal_mdc_isr_mask_val  = 0x0;
#endif

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_connectIsrCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    CLX_ERROR_NO_T  rc = CLX_E_OK;
    _osal_mdc_cb.intr_task = get_current();
    if (0 == (_osal_mdc_isr_init_bitmap & (1U << unit)))
    {
        rc = osal_mdc_connectIsr(unit, NULL, ptr_data);
        if (CLX_E_OK == rc)
        {
            _osal_mdc_isr_init_bitmap |= (1U << unit);
        }
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_disconnectIsrCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    struct task_struct *ref_task = get_current();

    /* To make the user-space polling task return from read.  */
    _osal_mdc_notifyUserProcess(unit);

    osal_mdc_disconnectIsr(unit);
    _osal_mdc_isr_init_bitmap &= ~(1U << unit);
    
    if(ref_task == _osal_mdc_cb.intr_task) {
        _osal_mdc_cb.intr_task = NULL;
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_savePciConfigCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    return _osal_mdc_savePciConfig(unit);
}

static CLX_ERROR_NO_T
_osal_mdc_ioctl_restorePciConfigCallback(
    const UI32_T    unit,
    void            *ptr_data)
{
    return _osal_mdc_restorePciConfig(unit);
}


CLX_ERROR_NO_T
_osal_mdc_registerIoctlCallback(
    const OSAL_MDC_IOCTL_TYPE_T             type,
    const OSAL_MDC_IOCTL_CALLBACK_FUNC_T    func)
{
    OSAL_MDC_IOCTL_CB_T     *ptr_cb = &_osal_mdc_ioctl_cb;
    CLX_ERROR_NO_T          rc = CLX_E_OTHERS;

    if (type < OSAL_MDC_IOCTL_TYPE_LAST)
    {
        if (NULL == ptr_cb->callback[type])
        {
            ptr_cb->callback[type] = func;
            rc = CLX_E_OK;
        }
        else
        {
            DIAG_PRINT(HAL_DBG_ERR,"register ioctl callback failed, type=%d exist\n", type);
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR,"register ioctl callback failed, type=%d >= max=%d\n",
                     type, OSAL_MDC_IOCTL_TYPE_LAST);
    }
    return (rc);
}

static CLX_ERROR_NO_T
_osal_mdc_register_mdc_ioctl(void)
{
    memset(&_osal_mdc_ioctl_cb, 0x0, sizeof(OSAL_MDC_IOCTL_CB_T));

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_INIT_DEV,
                                    _osal_mdc_ioctl_initDeviceCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_DEINIT_DEV,
                                    _osal_mdc_ioctl_deinitDeviceCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_ALLOC_SYS_DMA_MEM,
                                    _osal_mdc_ioctl_allocSysDmaMemCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_FREE_SYS_DMA_MEM,
                                    _osal_mdc_ioctl_freeSysDmaMemCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_CONNECT_ISR,
                                    _osal_mdc_ioctl_connectIsrCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_DISCONNECT_ISR,
                                    _osal_mdc_ioctl_disconnectIsrCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_SAVE_PCI_CONFIG,
                                    _osal_mdc_ioctl_savePciConfigCallback);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_MDC_RESTORE_PCI_CONFIG,
                                    _osal_mdc_ioctl_restorePciConfigCallback);
    return (CLX_E_OK);
}

static int
_osal_mdc_open(
    struct inode    *ptr_inode,
    struct file     *ptr_file)
{
    hal_netif_pkt_init(0);
    return (0);
}

static ssize_t
_osal_mdc_read(
    struct file         *filep,
    char __user         *buf,
    size_t              count,
    loff_t              *ppos)
{
    return (0);
}

static ssize_t
_osal_mdc_write(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    long ret = -1;

    ret = hal_pkt_dev_tx(file, buf, count, pos);

    return (ret);
}
static int
_osal_mdc_release(
    struct inode    *ptr_inode,
    struct file     *ptr_file)
{
    hal_netif_pkt_exit(0);
    return 0;
}

static long
_osal_mdc_ioctl(
    struct file     *filp,
    unsigned int    cmd,
    unsigned long   arg)
{
#define OSAL_MDC_IOCTL_LOCAL_BUF_SIZE           (128)

    OSAL_MDC_IOCTL_CB_T     *ptr_cb   = &_osal_mdc_ioctl_cb;
    OSAL_MDC_IOCTL_CMD_T    *ptr_cmd  = (OSAL_MDC_IOCTL_CMD_T *)&cmd;
    UI32_T                  unit      = ptr_cmd->field.unit;
    OSAL_MDC_IOCTL_TYPE_T   type      = ptr_cmd->field.type;
    OSAL_MDC_IOCTL_ACCESS_T access    = ptr_cmd->field.access;
    UI32_T                  data_size = ptr_cmd->field.size;
    UI8_T                   temp_buf[OSAL_MDC_IOCTL_LOCAL_BUF_SIZE];
    UI8_T                   *ptr_temp_buf;
    int                     linux_rc  = 0;

    if (NULL != ptr_cb->callback[type])
    {
        if (data_size > OSAL_MDC_IOCTL_LOCAL_BUF_SIZE)
        {
            ptr_temp_buf = kmalloc(data_size, GFP_KERNEL);
        }
        else
        {
            ptr_temp_buf = temp_buf;
        }

        /*************************************************************/
        if (OSAL_MDC_IOCTL_ACCESS_WRITE == access)
        {
            /* type: FREE_SYS_DMA_MEM   : DMA physical address
             *       CONNECT_ISR        : Chip interrupt mask address and value
             */
            if (copy_from_user(ptr_temp_buf, (int __user *)arg, data_size))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (CLX_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
                {
                    linux_rc = -EFAULT;
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_READ == access)
        {
            /* type: INIT_DEV           : PCIe device and vendor ID, mmio address and size
             *       INIT_RSRV_DMA_MEM  : Reserved DMA physical address and size
             */
            if (CLX_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (copy_to_user((int __user *)arg, ptr_temp_buf, data_size))
                {
                    linux_rc = -EFAULT;
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_READ_WRITE == access)
        {
            /* type: ALLOC_SYS_DMA_MEM  : DMA physical address
             */
            if (copy_from_user(ptr_temp_buf, (int __user *)arg, data_size))
            {
                linux_rc = -EFAULT;
            }
            else
            {
                if (CLX_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
                {
                    linux_rc = -EFAULT;
                }
                else
                {
                    if (copy_to_user((int __user *)arg, ptr_temp_buf, data_size))
                    {
                        linux_rc = -EFAULT;
                    }
                }
            }
        }
        else if (OSAL_MDC_IOCTL_ACCESS_NONE == access)
        {
            /* type: DEINIT_DEV
             *       DEINIT_RSRV_DMA_MEM
             *       DISCONNECT_ISR
             *       SAVE_PCI_CONFIG
             *       RESTORE_PCI_CONFIG
             */
            if (CLX_E_OK != ptr_cb->callback[type](unit, (void *)ptr_temp_buf))
            {
                linux_rc = -EFAULT;
            }
        }
        /*************************************************************/

        if (data_size > OSAL_MDC_IOCTL_LOCAL_BUF_SIZE)
        {
            kfree(ptr_temp_buf);
        }
    }
    else
    {
        DIAG_PRINT(HAL_DBG_ERR,"invalid ioctl, cmd=%u, arg=%lu, type=%d\n", cmd, arg, type);
    }
    return (linux_rc);
}

#ifdef CONFIG_COMPAT
static long
_osal_mdc_compat_ioctl(
    struct file     *filp,
    unsigned int    cmd,
    unsigned long   arg)
{
    return _osal_mdc_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static struct vm_operations_struct  _osal_mdc_remap_vm_ops =
{
    .open  = NULL,
    .close = NULL,
};

static int
_osal_mdc_mmap(
    struct file             *filp,
    struct vm_area_struct   *vma)
{
    size_t  size     = vma->vm_end - vma->vm_start;
    int     linux_rc = 0;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(3,0,48)
    pgprot_val(vma->vm_page_prot) |= (_PAGE_NO_CACHE | _PAGE_GUARDED);
#else
    UI32_T          dev_idx;
    OSAL_MDC_DEV_T  *ptr_dev;
    CLX_ADDR_T      phy_addr = vma->vm_pgoff << PAGE_SHIFT;

    /* check mmio base phy addr */
    for (dev_idx = 0, ptr_dev = &_osal_mdc_cb.dev[0];
         dev_idx < _osal_mdc_cb.dev_num;
         dev_idx++, ptr_dev++)
    {
        if ((NULL != ptr_dev->ptr_pci_dev) &&
            (phy_addr == pci_resource_start(ptr_dev->ptr_pci_dev, _osal_mdc_cb.register_bar[dev_idx])))
        {
            vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
            break;
        }
    }
#endif

    vma->vm_flags |= VM_IO;
    if (io_remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
                           size, vma->vm_page_prot))
    {
        linux_rc = -EAGAIN;
    }
    vma->vm_ops = &_osal_mdc_remap_vm_ops;
    return (linux_rc);
}

static struct file_operations _osal_mdc_fops =
{
    .owner          = THIS_MODULE,
    .open           = _osal_mdc_open,
    .read           = _osal_mdc_read,
    .write          = _osal_mdc_write,
    .release        = _osal_mdc_release,
    .unlocked_ioctl = _osal_mdc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = _osal_mdc_compat_ioctl,
#endif
    .mmap           = _osal_mdc_mmap,
};

static struct miscdevice _osal_mdc_misc =
{
    .minor  = OSAL_MDC_DRIVER_MISC_MINOR_NUM,
    .name   = OSAL_MDC_DRIVER_NAME,
    .fops   = & _osal_mdc_fops,
};

static int __init
osal_mdc_module_init(void)
{
    int     linux_rc;

    _osal_mdc_register_mdc_ioctl();      /* To register IOCTL callback functions. */
    _osal_mdc_initInterrupt();  /* To init structs for top and bottom halves */

    linux_rc = misc_register(&_osal_mdc_misc);
    if (0 != linux_rc)
    {
        DIAG_PRINT(HAL_DBG_ERR,"register dev %s failed, linux_rc=%d\n", OSAL_MDC_DRIVER_NAME, linux_rc);
    }
    return (linux_rc);
}

static void __exit
osal_mdc_module_exit(void)
{
    int     unit = 0;
#if LINUX_VERSION_CODE <= KERNEL_VERSION(4,2,8)
    int     linux_rc;

    linux_rc = misc_deregister(&_osal_mdc_misc);
    if (0 != linux_rc)
    {
        DIAG_PRINT(HAL_DBG_ERR,"de-register dev %s failed, linux_rc=%d\n", OSAL_MDC_DRIVER_NAME, linux_rc);
    }
#else
    misc_deregister(&_osal_mdc_misc);
#endif

    /* ref: _osal_mdc_ioctl_disconnectIsrCallback */
    for (unit = 0; unit < CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM; unit++)
    {
        if (0 != (_osal_mdc_isr_init_bitmap & (1U << unit)))
        {
            osal_mdc_disconnectIsr(unit);
            _osal_mdc_isr_init_bitmap &= ~(1U << unit);
        }
    }

    /* ref: _osal_mdc_ioctl_deinitRsrvDmaMemCallback */
#if defined(CLX_EN_DMA_RESERVED)
    if (1 == _osal_mdc_rsvDmaInited)
    {
        _osal_mdc_deinitRsrvDmaMem(&_osal_mdc_cb.dma_info);
        _osal_mdc_rsvDmaInited = 0;
    }
#endif

    /* ref: _osal_mdc_ioctl_deinitDeviceCallback */
    if (1 == _osal_mdc_devInited)
    {
#if !defined(CLX_EN_DMA_RESERVED)
        _osal_mdc_clearSysDmaList(0);
        _osal_mdc_clearSysDmaList(1);
#endif
        osal_mdc_deinitDevice();
        _osal_mdc_devInited = 0;
    }
}

module_init(osal_mdc_module_init);
module_exit(osal_mdc_module_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION("SDK Kernel Module");
