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

/* FILE NAME:  netif_knl.c
 * PURPOSE:
 *      To provide operations registered to kernel callback and
 *      dispatch the operations to different chip drivers.
 *
 * NOTES:
 *
 */

/*****************************************************************************
 * INCLUDE FILE DECLARATIONS
 *****************************************************************************
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/compat.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>

#include <hal_lightning_pkt_knl.h>
#include <hal_dawn_pkt_knl.h>
#include <hal/common/hal_dev.h>
#include <osal/osal_mdc.h>
#include <netif_osal.h>

typedef ssize_t
(*NETIF_KNL_DEV_TX_FUNC_T)(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos);

typedef long
(*NETIF_KNL_DEV_IOCTL_FUNC_T)(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg);

typedef CLX_ERROR_NO_T
(*NETIF_KNL_DEV_INIT_T)(
    const UI32_T            unit);

typedef CLX_ERROR_NO_T
(*NETIF_KNL_DEV_EXIT_T)(
    const UI32_T            unit);

typedef struct
{
    NETIF_KNL_DEV_IOCTL_FUNC_T          ioctl;
    NETIF_KNL_DEV_TX_FUNC_T             tx;
    NETIF_KNL_DEV_INIT_T                init;
    NETIF_KNL_DEV_EXIT_T                exit;
} NETIF_KNL_DEV_OPS_T;

typedef struct
{
    UI16_T                              dev_id;
    NETIF_KNL_DEV_OPS_T                 ops;

} NETIF_KNL_CB_T;

extern struct pci_dev                   *_ptr_ext_pci_dev;
static NETIF_KNL_CB_T                   _netif_knl_cb;

UI32_T     verbosity = 0x3;
module_param(verbosity, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(verbosity, "bit0:critical, bit1:error, bit2:warning, bit3:infor," \
        "bit4:debug, bit5:tx, bit6:rx, bit7:intf, bit8:profile, "   \
                               "bit9:common, bit10:netlink");

static CLX_ERROR_NO_T
_netif_knl_initDevOps(
    const UI16_T                        dev_id,
          NETIF_KNL_DEV_OPS_T           *ptr_ops)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;

    switch(dev_id) {
        case HAL_DEVICE_ID_CL8500:
            ptr_ops->init  = hal_lightning_pkt_init;
            ptr_ops->exit  = hal_lightning_pkt_exit;
            ptr_ops->tx    = hal_lightning_pkt_dev_tx;
            ptr_ops->ioctl = hal_lightning_pkt_dev_ioctl;
            break;
        case HAL_DEVICE_ID_CL8300:
            ptr_ops->init  = hal_dawn_pkt_init;
            ptr_ops->exit  = hal_dawn_pkt_exit;
            ptr_ops->tx    = hal_dawn_pkt_dev_tx;
            ptr_ops->ioctl = hal_dawn_pkt_dev_ioctl;
            break;
        default:
            HAL_KNL_DBG(HAL_KNL_COMMON,
                    "unknown chip family, dev_id=0x%x\n",
                    dev_id);
            return CLX_E_OTHERS;
    }

    return (rc);
}

static CLX_ERROR_NO_T
_netif_knl_getDevId(
    UI16_T              *ptr_dev_id)
{
    pci_read_config_word(_ptr_ext_pci_dev, PCI_DEVICE_ID, ptr_dev_id);
    *ptr_dev_id &= 0xFF00;
    return (CLX_E_OK);
}

static int
_netif_knl_dev_open(
    struct inode            *inode,
    struct file             *file)
{
    UI32_T              unit = 0;
    CLX_ERROR_NO_T      rc;

    _netif_knl_getDevId(&_netif_knl_cb.dev_id);

    rc = _netif_knl_initDevOps(_netif_knl_cb.dev_id,
                               &_netif_knl_cb.ops);
    if (CLX_E_OK == rc)
    {
        if (_netif_knl_cb.ops.init != NULL)
        {
            _netif_knl_cb.ops.init(unit);
        }
    }

    return (0);
}

static int
_netif_knl_dev_close(
    struct inode            *inode,
    struct file             *file)
{
    return (0);
}

static ssize_t
_netif_knl_dev_tx(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    long ret = CLX_E_OTHERS;

    if (_netif_knl_cb.ops.tx != NULL)
    {
        ret = _netif_knl_cb.ops.tx(file, buf, count, pos);
    }

    return (ret);
}

static ssize_t
_netif_knl_dev_rx(
    struct file             *file,
    char __user             *buf,
    size_t                  count,
    loff_t                  *pos)
{
    return (0);
}

static long
_netif_knl_dev_ioctl(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg)
{
    long ret = CLX_E_OTHERS;

    if (_netif_knl_cb.ops.ioctl != NULL)
    {
        ret = _netif_knl_cb.ops.ioctl(filp, cmd, arg);
    }

    return (ret);
}

#ifdef CONFIG_COMPAT
static long
_netif_knl_dev_compat_ioctl(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg)
{
    return _netif_knl_dev_ioctl(filp, cmd, (unsigned long)compat_ptr(arg));
}
#endif

static struct file_operations _netif_knl_dev_ops =
{
    .owner          = THIS_MODULE,
    .open           = _netif_knl_dev_open,
    .release        = _netif_knl_dev_close,
    .write          = _netif_knl_dev_tx,
    .read           = _netif_knl_dev_rx,
    .unlocked_ioctl = _netif_knl_dev_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = _netif_knl_dev_compat_ioctl,
#endif
};

static struct miscdevice _netif_knl_dev =
{
    .minor    = NETIF_KNL_DRIVER_MINOR_NUM,
    .name     = NETIF_KNL_DRIVER_NAME,
    .fops     = &_netif_knl_dev_ops,
};

extern int char_proc_init(void);
extern  void cleanup_procfs(void);
static int __init
netif_knl_init(void)
{
    misc_register(&_netif_knl_dev);

    osal_memset(&_netif_knl_cb, 0x0, sizeof(NETIF_KNL_CB_T));
    if (char_proc_init() != 0) {
        HAL_KNL_DBG(HAL_KNL_ERR, "Create procfs failed\n");
        misc_deregister(&_netif_knl_dev);
        return -ENOMEM;
    }

    return (0);
}

static void __exit
netif_knl_exit(void)
{
    UI32_T      unit = 0;

    if (_netif_knl_cb.ops.exit != NULL)
    {
        _netif_knl_cb.ops.exit(unit);
    }
    
    cleanup_procfs();
    misc_deregister(&_netif_knl_dev);
}

module_init(netif_knl_init);
module_exit(netif_knl_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Clounix");
MODULE_DESCRIPTION(NETIF_KNL_MODULE_DESC);
