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

/* FILE NAME:  hal_nb_pkt_knl.c
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
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
#include <linux/types.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/spinlock_types.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/pci.h>
#include <linux/module.h>
#include <linux/if.h>
#include <uapi/linux/ethtool.h>
#include <linux/ethtool.h>

/* netif */
#include <netif_osal.h>
#include <netif_perf.h>
#include <netif_nl.h>

#include <hal_nb_pkt_knl.h>

/* clx_sdk */
#include <osal/osal_mdc.h>
#include <hal/common/hal_dflt.h>



/*****************************************************************************
 * GLOBAL VARIABLE DECLARATIONS
 *****************************************************************************
 */
static HAL_NB_PKT_NETIF_PORT_DB_T                       _hal_nb_pkt_port_db[HAL_NB_PKT_MAX_PORT_NUM];
/*---------------------------------------------------------------------------*/
#define HAL_NB_PKT_GET_PORT_DB(port)                   (&_hal_nb_pkt_port_db[port])
#define HAL_NB_PKT_GET_PORT_PROFILE_LIST(port)         (_hal_nb_pkt_port_db[port].ptr_profile_list)
#define HAL_NB_PKT_GET_PORT_NETDEV(port)               (_hal_nb_pkt_port_db[port].ptr_net_dev)


struct net_device_priv
{
    struct net_device               *ptr_net_dev;
    struct net_device_stats         stats;
    UI32_T                          unit;
    UI32_T                          id;
    UI32_T                          port;
    UI16_T                          vlan;
    UI32_T                          speed;
};


/* ----------------------------------------------------------------------------------- Init/Deinit */
CLX_ERROR_NO_T
hal_nb_pkt_init(
    const UI32_T            unit)
{
    /* Init Thread */
    osal_init();
    /* Reset all database*/
    osal_memset(_hal_nb_pkt_port_db, 0x0,
                (HAL_NB_PKT_MAX_PORT_NUM * sizeof(HAL_NB_PKT_NETIF_PORT_DB_T)));
                
#if defined(NETIF_EN_NETLINK)
    netif_nl_init();
#endif

    return (0);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_stopAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_NB_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_NB_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            netif_tx_disable(ptr_net_dev);
        }
    }

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_nb_pkt_exit(
    const UI32_T            unit)
{
    /* 1st. Stop all netdev (if any) to prevent kernel from Tx new packets */
    _hal_nb_pkt_stopAllIntf(unit);



    osal_deinit();

    return (0);
}

ssize_t
hal_nb_pkt_dev_tx(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    return 0;
}


static void
_hal_nb_pkt_lockRxChannelAll(
    const UI32_T                        unit)
{

}

static void
_hal_nb_pkt_unlockRxChannelAll(
    const UI32_T                        unit)
{

}
/* ----------------------------------------------------------------------------------- Init: net_dev_ops */
static int
_hal_nb_pkt_net_dev_init(
    struct net_device           *ptr_net_dev)
{
    return 0;
}

static int
_hal_nb_pkt_net_dev_open(
    struct net_device           *ptr_net_dev)
{
    netif_start_queue(ptr_net_dev);

    return 0;
}

static int
_hal_nb_pkt_net_dev_stop(
    struct net_device           *ptr_net_dev)
{
    netif_stop_queue(ptr_net_dev);
    return 0;
}

static int
_hal_nb_pkt_net_dev_ioctl(
    struct net_device           *ptr_net_dev,
    struct ifreq                *ptr_ifreq,
    int                         cmd)
{
    return 0;
}

static netdev_tx_t
_hal_nb_pkt_net_dev_tx(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev)
{
    return NETDEV_TX_OK;
}
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
static void
_hal_nb_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev,
    unsigned int txqueue)
#else
static void
_hal_nb_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev)
#endif
{
    netif_stop_queue(ptr_net_dev);
    osal_sleepThread(1000);
    netif_wake_queue(ptr_net_dev);
}

static struct net_device_stats *
_hal_nb_pkt_net_dev_get_stats(
    struct net_device           *ptr_net_dev)
{
    struct net_device_priv      *ptr_priv = netdev_priv(ptr_net_dev);

    return (&ptr_priv->stats);
}

static int
_hal_nb_pkt_net_dev_set_mtu(
    struct net_device           *ptr_net_dev,
    int                         new_mtu)
{
    if (new_mtu < 64 || new_mtu > 9216)
    {
        return -EINVAL;
    }
    ptr_net_dev->mtu = new_mtu; /* This mtu need to be synced to chip's */
    return 0;
}

static int
_hal_nb_pkt_net_dev_set_mac(
    struct net_device           *ptr_net_dev,
    void                        *ptr_mac_addr)
{
    struct sockaddr             *ptr_addr = ptr_mac_addr;

    memcpy(ptr_net_dev->dev_addr, ptr_addr->sa_data, ptr_net_dev->addr_len);
    return 0;
}

static void
_hal_nb_pkt_net_dev_set_rx_mode(
    struct net_device           *ptr_dev)
{
    if (ptr_dev->flags & IFF_PROMISC)
    {
    }
    else
    {
        if (ptr_dev->flags & IFF_ALLMULTI)
        {
        }
        else
        {
            if (netdev_mc_empty(ptr_dev))
            {
                return;
            }
        }
    }
}

static struct net_device_ops    _hal_nb_pkt_net_dev_ops =
{
    .ndo_init            = _hal_nb_pkt_net_dev_init,
    .ndo_open            = _hal_nb_pkt_net_dev_open,
    .ndo_stop            = _hal_nb_pkt_net_dev_stop,
    .ndo_do_ioctl        = _hal_nb_pkt_net_dev_ioctl,
    .ndo_start_xmit      = _hal_nb_pkt_net_dev_tx,
    .ndo_tx_timeout      = _hal_nb_pkt_net_dev_tx_timeout,
    .ndo_get_stats       = _hal_nb_pkt_net_dev_get_stats,
    .ndo_change_mtu      = _hal_nb_pkt_net_dev_set_mtu,
    .ndo_set_mac_address = _hal_nb_pkt_net_dev_set_mac,
    .ndo_set_rx_mode     = _hal_nb_pkt_net_dev_set_rx_mode,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
static int
_hal_nb_pkt_net_dev_ethtool_get(
    struct net_device           *ptr_dev,
    struct ethtool_cmd          *ptr_cmd)
{
    struct net_device_priv          *ptr_priv;

    ptr_cmd->supported   = SUPPORTED_1000baseT_Full | SUPPORTED_FIBRE;
    ptr_cmd->port        = PORT_FIBRE;
    ptr_cmd->duplex      = DUPLEX_FULL;

    ptr_priv = netdev_priv(ptr_dev);
    ethtool_cmd_speed_set(ptr_cmd, ptr_priv->speed);

    return 0;
}
#endif

static struct ethtool_ops       _hal_nb_pkt_net_dev_ethtool_ops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
    .get_settings        = _hal_nb_pkt_net_dev_ethtool_get,
#endif
    .get_link            = ethtool_op_get_link,
};

static void
_hal_nb_pkt_setup(
    struct net_device       *ptr_net_dev)
{
    struct net_device_priv  *ptr_priv = netdev_priv(ptr_net_dev);

    /* setup net device */
    ether_setup(ptr_net_dev);
    ptr_net_dev->netdev_ops     = &_hal_nb_pkt_net_dev_ops;
    ptr_net_dev->ethtool_ops    = &_hal_nb_pkt_net_dev_ethtool_ops;
    ptr_net_dev->watchdog_timeo = HAL_NB_PKT_TX_TIMEOUT;
    ptr_net_dev->mtu            = HAL_NB_PKT_MAX_ETH_FRAME_SIZE; /* This mtu need to be synced to chip's */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,19,0)
	ptr_net_dev->min_mtu        = 64;
	ptr_net_dev->max_mtu        = 65535;    
#endif
    random_ether_addr(ptr_net_dev->dev_addr); /* Please use the mac-addr of interface. */

    /* setup private data */
    ptr_priv->ptr_net_dev       = ptr_net_dev;
    memset(&ptr_priv->stats, 0, sizeof(struct net_device_stats));
}

static CLX_ERROR_NO_T
_hal_nb_pkt_createIntf(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie)
{
    HAL_NB_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_NB_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device                   *ptr_net_dev = NULL;
    struct net_device_priv              *ptr_priv = NULL;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    _hal_nb_pkt_lockRxChannelAll(unit);

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

    DIAG_PRINT(HAL_DBG_INTF, "u=%u, create intf name=%s, phy port=%d\n",
                    unit, net_intf.name, net_intf.port);

    /* To check if the interface with the same name exists in kernel */
    ptr_net_dev = dev_get_by_name(&init_net, net_intf.name);
    if (NULL != ptr_net_dev)
    {
        DIAG_PRINT((HAL_DBG_ERR | HAL_DBG_INTF),
                        "u=%u, create intf failed, exist same name=%s\n",
                        unit, net_intf.name);

        dev_put(ptr_net_dev);

#if defined(HAL_NB_PKT_FORCR_REMOVE_DUPLICATE_NETDEV)
        ptr_net_dev->operstate = IF_OPER_DOWN;
        netif_carrier_off(ptr_net_dev);
        netif_tx_disable(ptr_net_dev);
        unregister_netdev(ptr_net_dev);
        free_netdev(ptr_net_dev);
#endif
        _hal_nb_pkt_unlockRxChannelAll(unit);
        return (CLX_E_ENTRY_EXISTS);
    }

    /* Bind the net dev and intf meta data to internel port-based array */
    ptr_port_db = HAL_NB_PKT_GET_PORT_DB(net_intf.port);
    if (ptr_port_db->ptr_net_dev == NULL)
    {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, NET_NAME_UNKNOWN, _hal_nb_pkt_setup);
#else
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, _hal_nb_pkt_setup);
#endif
        memcpy(ptr_net_dev->dev_addr, net_intf.mac, ptr_net_dev->addr_len);

        ptr_priv = netdev_priv(ptr_net_dev);

        /* Port info will be used when packet sent from this netdev */
        ptr_priv->port = net_intf.port;
        ptr_priv->id   = net_intf.port;
        ptr_priv->unit = unit;

        register_netdev(ptr_net_dev);

        netif_carrier_off(ptr_net_dev);

        net_intf.id = net_intf.port;    /* Currently, id is 1-to-1 mapped to port */
        osal_memcpy(&ptr_port_db->meta, &net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

        ptr_port_db->ptr_net_dev = ptr_net_dev;

        /* Copy the intf-id to user space */
        osal_io_copyToUser(&ptr_cookie->net_intf, &net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));
    }
    else
    {
        DIAG_PRINT((HAL_DBG_INTF | HAL_DBG_ERR),
                        "u=%u, create intf failed, exist on phy port=%d\n",
                        unit, net_intf.port);
        /* The user needs to delete the existing intf binding to the same port */
        rc = CLX_E_ENTRY_EXISTS;
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_nb_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_nb_pkt_destroyIntf(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie)
{
    HAL_NB_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_NB_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    _hal_nb_pkt_lockRxChannelAll(unit);

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_NB_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_NB_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                DIAG_PRINT(HAL_DBG_INTF,
                                "u=%u, find intf %s (id=%d) on phy port=%d, destroy done\n",
                                unit,
                                ptr_port_db->meta.name,
                                ptr_port_db->meta.id,
                                ptr_port_db->meta.port);

                netif_carrier_off(ptr_port_db->ptr_net_dev);
                netif_tx_disable(ptr_port_db->ptr_net_dev);
                unregister_netdev(ptr_port_db->ptr_net_dev);
                free_netdev(ptr_port_db->ptr_net_dev);

                /* Don't need to remove profiles on this port.
                 * In fact, the profile is binding to "port" not "intf".
                 */
                /* _hal_nb_pkt_destroyProfList(ptr_port_db->ptr_profile_list); */

                osal_memset(ptr_port_db, 0x0, sizeof(HAL_NB_PKT_NETIF_PORT_DB_T));
                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    _hal_nb_pkt_unlockRxChannelAll(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_traverseProfList(
    UI32_T                          intf_id,
    HAL_NB_PKT_PROFILE_NODE_T      *ptr_prof_list)
{
    HAL_NB_PKT_PROFILE_NODE_T      *ptr_curr_node;

    ptr_curr_node = ptr_prof_list;

    DIAG_PRINT(HAL_DBG_INTF, "intf id=%d, prof list=", intf_id);
    while(NULL != ptr_curr_node)
    {
        DIAG_PRINT(HAL_DBG_INTF, "%s (%d) => ",
                        ptr_curr_node->ptr_profile->name,
                        ptr_curr_node->ptr_profile->priority);
        ptr_curr_node = ptr_curr_node->ptr_next_node;
    }
    DIAG_PRINT(HAL_DBG_INTF, "null\n");
    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_nb_pkt_getIntf(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie)
{
    HAL_NB_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_NB_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_NB_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_NB_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                DIAG_PRINT(HAL_DBG_INTF, "u=%u, find intf id=%d\n", unit, net_intf.id);
                _hal_nb_pkt_traverseProfList(net_intf.id, ptr_port_db->ptr_profile_list);
                osal_io_copyToUser(&ptr_cookie->net_intf, &ptr_port_db->meta, sizeof(HAL_NB_PKT_NETIF_INTF_T));
                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_nb_pkt_setPortAttr
 * PURPOSE:
 *      To set the port attributes such as status or speeds.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the Port cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully set the attributes.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_nb_pkt_setPortAttr(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_PORT_COOKIE_T     *ptr_cookie)
{
    struct net_device                   *ptr_net_dev;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port;
    UI32_T                              status;
    CLX_PORT_SPEED_T                    speed;

    osal_io_copyFromUser(&port,   &ptr_cookie->port, sizeof(UI32_T));
    osal_io_copyFromUser(&status, &ptr_cookie->status, sizeof(UI32_T));
    osal_io_copyFromUser(&speed,  &ptr_cookie->speed, sizeof(CLX_PORT_SPEED_T));

    ptr_net_dev = HAL_NB_PKT_GET_PORT_NETDEV(port);
    if ((NULL != ptr_net_dev) && (port<HAL_NB_PKT_MAX_PORT_NUM))
    {
        if (HAL_NB_PKT_PORT_STATUS_UP == status)
        {
            netif_carrier_on(ptr_net_dev);
        }
        else
        {
            netif_carrier_off(ptr_net_dev);
        }

        /* Link speed config */
        ptr_priv = netdev_priv(ptr_net_dev);
        switch(speed)
        {
            case CLX_PORT_SPEED_1G:
                ptr_priv->speed = SPEED_1000;
                break;
            case CLX_PORT_SPEED_10G:
                ptr_priv->speed = SPEED_10000;
                break;
            case CLX_PORT_SPEED_25G:
                ptr_priv->speed = SPEED_25000;
                break;
            case CLX_PORT_SPEED_40G:
                ptr_priv->speed = SPEED_40000;
                break;
            case CLX_PORT_SPEED_50G:
                ptr_priv->speed = SPEED_50000;
                break;
            case CLX_PORT_SPEED_100G:
                ptr_priv->speed = SPEED_100000;
                break;
            case CLX_PORT_SPEED_200G:
                ptr_priv->speed = SPEED_200000;
                break;
            case CLX_PORT_SPEED_400G:
                ptr_priv->speed = SPEED_400000;
                break;
            default:
                break;
        }
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_getIntfCnt(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie)
{
    HAL_NB_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_NB_PKT_NETIF_INTF_CNT_T        intf_cnt = {0};
    HAL_NB_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_NB_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_NB_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                ptr_priv = netdev_priv(ptr_port_db->ptr_net_dev);
                intf_cnt.rx_pkt   = ptr_priv->stats.rx_packets;
                intf_cnt.tx_pkt   = ptr_priv->stats.tx_packets;
                intf_cnt.tx_error = ptr_priv->stats.tx_errors;
                intf_cnt.tx_queue_full = ptr_priv->stats.tx_fifo_errors;

                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->cnt, &intf_cnt, sizeof(HAL_NB_PKT_NETIF_INTF_CNT_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_clearIntfCnt(
    const UI32_T                        unit,
    HAL_NB_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie)
{
    HAL_NB_PKT_NETIF_INTF_T            net_intf = {0};
    HAL_NB_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    osal_io_copyFromUser(&net_intf, &ptr_cookie->net_intf, sizeof(HAL_NB_PKT_NETIF_INTF_T));

    for (port = 0; port < HAL_NB_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_NB_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                ptr_priv = netdev_priv(ptr_port_db->ptr_net_dev);
                ptr_priv->stats.rx_packets = 0;
                ptr_priv->stats.tx_packets = 0;
                ptr_priv->stats.tx_errors  = 0;
                ptr_priv->stats.tx_fifo_errors = 0;

                rc = CLX_E_OK;
                break;
            }
        }
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

#if defined(NETIF_EN_NETLINK)

static CLX_ERROR_NO_T
_hal_nb_pkt_setIntfProperty(
    const UI32_T                        unit,
    HAL_NB_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie)
{
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;

    osal_io_copyFromUser(&intf_id,  &ptr_cookie->intf_id,  sizeof(UI32_T));
    osal_io_copyFromUser(&property, &ptr_cookie->property, sizeof(NETIF_NL_INTF_PROPERTY_T));
    osal_io_copyFromUser(&param0,   &ptr_cookie->param0,   sizeof(UI32_T));
    osal_io_copyFromUser(&param1,   &ptr_cookie->param1,   sizeof(UI32_T));

    _hal_nb_pkt_lockRxChannelAll(unit);

    rc = netif_nl_setIntfProperty(unit, intf_id, property, param0, param1);

    _hal_nb_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_getIntfProperty(
    const UI32_T                        unit,
    HAL_NB_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie)
{
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;

    osal_io_copyFromUser(&intf_id,  &ptr_cookie->intf_id,  sizeof(UI32_T));
    osal_io_copyFromUser(&property, &ptr_cookie->property, sizeof(NETIF_NL_INTF_PROPERTY_T));
    osal_io_copyFromUser(&param0,   &ptr_cookie->param0,   sizeof(UI32_T));

    rc = netif_nl_getIntfProperty(unit, intf_id, property, &param0, &param1);

    osal_io_copyToUser(&ptr_cookie->param0, &param0, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->param1, &param1, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_createNetlink(
    const UI32_T                        unit,
    HAL_NB_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie)
{
    NETIF_NL_NETLINK_T                  netlink;
    UI32_T                              netlink_id;
    CLX_ERROR_NO_T                      rc;

    osal_io_copyFromUser(&netlink, &ptr_cookie->netlink, sizeof(NETIF_NL_NETLINK_T));

    _hal_nb_pkt_lockRxChannelAll(unit);

    rc = netif_nl_createNetlink(unit, &netlink, &netlink_id);

    _hal_nb_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->netlink.id, &netlink_id, sizeof(UI32_T));
    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_destroyNetlink(
    const UI32_T                        unit,
    HAL_NB_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie)
{
    UI32_T                              netlink_id;
    CLX_ERROR_NO_T                      rc;

    osal_io_copyFromUser(&netlink_id, &ptr_cookie->netlink.id, sizeof(UI32_T));

    _hal_nb_pkt_lockRxChannelAll(unit);

    rc = netif_nl_destroyNetlink(unit, netlink_id);

    _hal_nb_pkt_unlockRxChannelAll(unit);

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (rc);
}

static CLX_ERROR_NO_T
_hal_nb_pkt_getNetlink(
    const UI32_T                        unit,
    HAL_NB_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie)
{
    UI32_T                              id;
    NETIF_NL_NETLINK_T                  netlink;
    CLX_ERROR_NO_T                      rc;

    osal_io_copyFromUser(&id, &ptr_cookie->netlink.id, sizeof(UI32_T));

    rc = netif_nl_getNetlink(unit, id, &netlink);
    if (CLX_E_OK == rc)
    {
        osal_io_copyToUser(&ptr_cookie->netlink, &netlink, sizeof(NETIF_NL_NETLINK_T));
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    osal_io_copyToUser(&ptr_cookie->rc, &rc, sizeof(CLX_ERROR_NO_T));

    return (CLX_E_OK);
}

#endif
long
hal_nb_pkt_dev_ioctl(
    struct file             *filp,
    unsigned int            cmd,
    unsigned long           arg)
{
    int                             ret = 0;

    /* cmd */
    HAL_NB_PKT_IOCTL_CMD_T         *ptr_cmd = (HAL_NB_PKT_IOCTL_CMD_T *)&cmd;
    unsigned int                    unit = ptr_cmd->field.unit;
    HAL_NB_PKT_IOCTL_TYPE_T        type = ptr_cmd->field.type;

    DIAG_PRINT(HAL_DBG_COMMON, "u=%u, ioctl type=%u, cmd=%u\n",
                    unit, type, cmd);

    switch (type)
    {
        /* network interface */
        case HAL_NB_PKT_IOCTL_TYPE_CREATE_INTF:
            ret = _hal_nb_pkt_createIntf(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_DESTROY_INTF:
            ret = _hal_nb_pkt_destroyIntf(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_GET_INTF:
            ret = _hal_nb_pkt_getIntf(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_GET_INTF_CNT:
            ret = _hal_nb_pkt_getIntfCnt(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_CLEAR_INTF_CNT:
            ret = _hal_nb_pkt_clearIntfCnt(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_SET_PORT_ATTR:
            ret = hal_nb_pkt_setPortAttr(unit, (HAL_NB_PKT_IOCTL_PORT_COOKIE_T *)arg);
            break;

        #if 0
        case HAL_NB_PKT_IOCTL_TYPE_CREATE_PROFILE:
            ret = _hal_nb_pkt_createProfile(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_DESTROY_PROFILE:
            ret = _hal_nb_pkt_destroyProfile(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_GET_PROFILE:
            ret = _hal_nb_pkt_getProfile(unit, (HAL_NB_PKT_IOCTL_NETIF_COOKIE_T *)arg);
            break;

        /* driver */
        case HAL_NB_PKT_IOCTL_TYPE_WAIT_RX_FREE:
            ret = _hal_nb_pkt_schedRxDeQueue(unit, (HAL_NB_PKT_IOCTL_RX_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_WAIT_TX_FREE:
            ret = _hal_nb_pkt_strictTxDeQueue(unit, (HAL_NB_PKT_IOCTL_TX_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_SET_RX_CFG:
            ret = hal_nb_pkt_setRxKnlConfig(unit, (HAL_NB_PKT_IOCTL_RX_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_GET_RX_CFG:
            ret = hal_nb_pkt_getRxKnlConfig(unit, (HAL_NB_PKT_IOCTL_RX_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_DEINIT_TASK:
            ret = hal_nb_pkt_deinitTask(unit);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_DEINIT_DRV:
            ret = hal_nb_pkt_deinitPktDrv(unit);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_INIT_TASK:
            ret = hal_nb_pkt_initTask(unit);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_INIT_DRV:
            ret = hal_nb_pkt_initPktDrv(unit);
            break;
        /* counter */
        case HAL_NB_PKT_IOCTL_TYPE_GET_TX_CNT:
            ret = hal_nb_pkt_getTxKnlCnt(unit, (HAL_NB_PKT_IOCTL_CH_CNT_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_GET_RX_CNT:
            ret = hal_nb_pkt_getRxKnlCnt(unit, (HAL_NB_PKT_IOCTL_CH_CNT_COOKIE_T *)arg);
            break;
        case HAL_NB_PKT_IOCTL_TYPE_CLEAR_TX_CNT:
            ret = hal_nb_pkt_clearTxKnlCnt(unit, (HAL_NB_PKT_IOCTL_TX_COOKIE_T *)arg);
            break;

        case HAL_NB_PKT_IOCTL_TYPE_CLEAR_RX_CNT:
            ret = hal_nb_pkt_clearRxKnlCnt(unit, (HAL_NB_PKT_IOCTL_RX_COOKIE_T *)arg);
            break;
        #endif

#if defined(NETIF_EN_NETLINK)
        case HAL_NB_PKT_IOCTL_TYPE_NL_SET_INTF_PROPERTY:
            ret = _hal_nb_pkt_setIntfProperty(unit, (HAL_NB_PKT_NL_IOCTL_COOKIE_T *)arg);
            break;
        case HAL_NB_PKT_IOCTL_TYPE_NL_GET_INTF_PROPERTY:
            ret = _hal_nb_pkt_getIntfProperty(unit, (HAL_NB_PKT_NL_IOCTL_COOKIE_T *)arg);
            break;
        case HAL_NB_PKT_IOCTL_TYPE_NL_CREATE_NETLINK:
            ret = _hal_nb_pkt_createNetlink(unit, (HAL_NB_PKT_NL_IOCTL_COOKIE_T *)arg);
            break;
        case HAL_NB_PKT_IOCTL_TYPE_NL_DESTROY_NETLINK:
            ret = _hal_nb_pkt_destroyNetlink(unit, (HAL_NB_PKT_NL_IOCTL_COOKIE_T *)arg);
            break;
        case HAL_NB_PKT_IOCTL_TYPE_NL_GET_NETLINK:
            ret = _hal_nb_pkt_getNetlink(unit, (HAL_NB_PKT_NL_IOCTL_COOKIE_T *)arg);
            break;
#endif

        default:
            ret = -1;
            break;
    }

    return (ret);
}