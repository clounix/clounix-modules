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
#include <linux/if_ether.h>
#include <linux/if_vlan.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>

/* netif */
#include <netif/netif_osal.h>
#include <netif/netif_perf.h>
#include <netif/netif_nl.h>

#include <netif/netif_common.h>
#include <netif/netif_lightning_pkt.h>
#include <netif/netif_dawn_pkt.h>
#include <netif/netif_nb_pkt.h>

/* clx_sdk */
#include <osal/osal_mdc.h>
#include <hal/common/hal_dflt.h>


static HAL_PKT_NETIF_PROFILE_T              *_ptr_hal_pkt_profile_entry[HAL_PKT_NET_PROFILE_NUM_MAX] = {0};
HAL_PKT_NETIF_PORT_DB_T                     _hal_pkt_port_db[HAL_PKT_MAX_PORT_NUM];
HAL_PKT_DRV_CB_T                            _hal_pkt_drv_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];

/*---------------------------------------------------------------------------*/
#define HAL_PKT_GET_DRV_CB_PTR(unit)                (&_hal_pkt_drv_cb[unit])
#define HAL_PKT_GET_PORT_DB(port)                   (&_hal_pkt_port_db[port])
#define HAL_PKT_GET_PORT_PROFILE_LIST(port)         (_hal_pkt_port_db[port].ptr_profile_list)
#define HAL_PKT_GET_PORT_NETDEV(port)               _hal_pkt_port_db[port].ptr_net_dev


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


/*  Init: net_dev_ops */
static int
_hal_pkt_net_dev_init(
    struct net_device           *ptr_net_dev)
{
    return 0;
}

static int
_hal_pkt_net_dev_open(
    struct net_device           *ptr_net_dev)
{
    netif_start_queue(ptr_net_dev);

    return 0;
}

static int
_hal_pkt_net_dev_stop(
    struct net_device           *ptr_net_dev)
{
    netif_stop_queue(ptr_net_dev);
    return 0;
}

static int
_hal_pkt_net_dev_ioctl(
    struct net_device           *ptr_net_dev,
    struct ifreq                *ptr_ifreq,
    int                         cmd)
{
    return 0;
}

static netdev_tx_t
_hal_pkt_net_dev_tx(
    struct sk_buff              *ptr_skb,
    struct net_device           *ptr_net_dev)
{    
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(0);
    return ptr_cb->net_dev_tx(ptr_skb,ptr_net_dev);
}

ssize_t
hal_pkt_dev_tx(
    struct file             *file,
    const char __user       *buf,
    size_t                  count,
    loff_t                  *pos)
{
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(0);
    return ptr_cb->pkt_dev_tx(file,buf,count,pos);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0)
static void
_hal_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev,
    unsigned int txqueue)
#else
static void
_hal_pkt_net_dev_tx_timeout(
    struct net_device           *ptr_net_dev)
#endif
{
    netif_stop_queue(ptr_net_dev);
    osal_sleepThread(1000);
    netif_wake_queue(ptr_net_dev);
}

static struct net_device_stats *
_hal_pkt_net_dev_get_stats(
    struct net_device           *ptr_net_dev)
{
    struct net_device_priv      *ptr_priv = netdev_priv(ptr_net_dev);

    return (&ptr_priv->stats);
}

static int
_hal_pkt_net_dev_set_mtu(
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
_hal_pkt_net_dev_set_mac(
    struct net_device           *ptr_net_dev,
    void                        *ptr_mac_addr)
{
    struct sockaddr             *ptr_addr = ptr_mac_addr;

    memcpy(ptr_net_dev->dev_addr, ptr_addr->sa_data, ptr_net_dev->addr_len);
    return 0;
}

static void
_hal_pkt_net_dev_set_rx_mode(
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

static struct net_device_ops    _hal_pkt_net_dev_ops =
{
    .ndo_init            = _hal_pkt_net_dev_init,
    .ndo_open            = _hal_pkt_net_dev_open,
    .ndo_stop            = _hal_pkt_net_dev_stop,
    .ndo_do_ioctl        = _hal_pkt_net_dev_ioctl,
    .ndo_start_xmit      = _hal_pkt_net_dev_tx,
    .ndo_tx_timeout      = _hal_pkt_net_dev_tx_timeout,
    .ndo_get_stats       = _hal_pkt_net_dev_get_stats,
    .ndo_change_mtu      = _hal_pkt_net_dev_set_mtu,
    .ndo_set_mac_address = _hal_pkt_net_dev_set_mac,
    .ndo_set_rx_mode     = _hal_pkt_net_dev_set_rx_mode,
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
static int
_hal_pkt_net_dev_ethtool_get(
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

static struct ethtool_ops       _hal_pkt_net_dev_ethtool_ops =
{
#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)    
    .get_settings        = _hal_pkt_net_dev_ethtool_get,
#endif
    .get_link            = ethtool_op_get_link,
};

static void
_hal_pkt_setup(
    struct net_device       *ptr_net_dev)
{
    struct net_device_priv  *ptr_priv = netdev_priv(ptr_net_dev);

    /* setup net device */
    ether_setup(ptr_net_dev);
    ptr_net_dev->netdev_ops     = &_hal_pkt_net_dev_ops;
    ptr_net_dev->ethtool_ops    = &_hal_pkt_net_dev_ethtool_ops;
    ptr_net_dev->watchdog_timeo = HAL_PKT_TX_TIMEOUT;
    ptr_net_dev->mtu            = HAL_PKT_MAX_ETH_FRAME_SIZE; /* This mtu need to be synced to chip's */
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
_hal_pkt_createIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = (HAL_PKT_IOCTL_NETIF_COOKIE_T*)ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    HAL_PKT_NETIF_INTF_T            net_intf = ptr_cookie->net_intf;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device                   *ptr_net_dev = NULL;
    struct net_device_priv              *ptr_priv = NULL;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    ptr_cb->lock_all_rx_channel(unit);

     DIAG_PRINT( HAL_DBG_INTF, "u=%u, create intf name=%s, phy port=%d\n",
                    unit, net_intf.name, net_intf.port);

    /* To check if the interface with the same name exists in kernel */
    ptr_net_dev = dev_get_by_name(&init_net, net_intf.name);
    if (NULL != ptr_net_dev)
    {
         DIAG_PRINT(( HAL_DBG_ERR |  HAL_DBG_INTF),
                        "u=%u, create intf failed, exist same name=%s\n",
                        unit, net_intf.name);

        dev_put(ptr_net_dev);

#if defined(HAL_PKT_FORCR_REMOVE_DUPLICATE_NETDEV)
        ptr_net_dev->operstate = IF_OPER_DOWN;
        netif_carrier_off(ptr_net_dev);
        netif_tx_disable(ptr_net_dev);
        unregister_netdev(ptr_net_dev);
        free_netdev(ptr_net_dev);
#endif
        ptr_cb->unlock_all_rx_channel(unit);
        return (CLX_E_ENTRY_EXISTS);
    }

    /* Bind the net dev and intf meta data to internel port-based array */
    ptr_port_db = HAL_PKT_GET_PORT_DB(net_intf.port);
    if (ptr_port_db->ptr_net_dev == NULL)
    {

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 17, 0)
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, NET_NAME_UNKNOWN, _hal_pkt_setup);
#else
        ptr_net_dev = alloc_netdev(sizeof(struct net_device_priv),
                                   net_intf.name, _hal_pkt_setup);
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
        osal_memcpy(&ptr_port_db->meta, &net_intf, sizeof(HAL_PKT_NETIF_INTF_T));

        ptr_port_db->ptr_net_dev = ptr_net_dev;

        /* Copy the intf-id to user space */
        ptr_cookie->net_intf = net_intf;
    }
    else
    {
         DIAG_PRINT(( HAL_DBG_INTF |  HAL_DBG_ERR),
                        "u=%u, create intf failed, exist on phy port=%d\n",
                        unit, net_intf.port);
        /* The user needs to delete the existing intf binding to the same port */
        rc = CLX_E_ENTRY_EXISTS;
    }

    ptr_cookie->rc = rc; 
    ptr_cb->unlock_all_rx_channel(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_destroyIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    HAL_PKT_NETIF_INTF_T            net_intf = ptr_cookie->net_intf;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    /* Lock all Rx tasks to avoid any access to the intf during packet processing */
    /* Only Rx tasks are locked since Tx action is performed under a spinlock protection */
    ptr_cb->lock_all_rx_channel(unit);

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                 DIAG_PRINT( HAL_DBG_INTF,
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
                /* _hal_pkt_destroyProfList(ptr_port_db->ptr_profile_list); */

                osal_memset(ptr_port_db, 0x0, sizeof(HAL_PKT_NETIF_PORT_DB_T));
                rc = CLX_E_OK;
                break;
            }
        }
    }

    ptr_cookie->rc = rc;

    ptr_cb->unlock_all_rx_channel(unit);

    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_pkt_traverseProfList(
    UI32_T                          intf_id,
    HAL_PKT_PROFILE_NODE_T      *ptr_prof_list)
{
    HAL_PKT_PROFILE_NODE_T      *ptr_curr_node;

    ptr_curr_node = ptr_prof_list;

     DIAG_PRINT( HAL_DBG_INTF, "intf id=%d, prof list=", intf_id);
    while(NULL != ptr_curr_node)
    {
         DIAG_PRINT( HAL_DBG_INTF, "%s (%d) => ",
                        ptr_curr_node->ptr_profile->name,
                        ptr_curr_node->ptr_profile->priority);
        ptr_curr_node = ptr_curr_node->ptr_next_node;
    }
     DIAG_PRINT( HAL_DBG_INTF, "null\n");
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_getIntf(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_NETIF_INTF_T            net_intf = ptr_cookie->net_intf;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            if (ptr_port_db->meta.id == net_intf.id)
            {
                 DIAG_PRINT( HAL_DBG_INTF, "u=%u, find intf id=%d\n", unit, net_intf.id);
                _hal_pkt_traverseProfList(net_intf.id, ptr_port_db->ptr_profile_list);
                ptr_cookie->net_intf = ptr_port_db->meta;
                rc = CLX_E_OK;
                break;
            }
        }
    }

    ptr_cookie->rc = rc;

    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_pkt_addProfToList(
    HAL_PKT_NETIF_PROFILE_T         *ptr_new_profile,
    HAL_PKT_PROFILE_NODE_T          **pptr_profile_list)
{
    HAL_PKT_PROFILE_NODE_T      *ptr_new_prof_node;
    HAL_PKT_PROFILE_NODE_T      *ptr_curr_node, *ptr_prev_node;

    ptr_new_prof_node = osal_alloc(sizeof(HAL_PKT_PROFILE_NODE_T));
    ptr_new_prof_node->ptr_profile = ptr_new_profile;

    /* Create the 1st node in the interface profile list */
    if (NULL == *pptr_profile_list)
    {
        DIAG_PRINT(HAL_DBG_PROFILE,
                        "prof list empty\n");
        *pptr_profile_list = ptr_new_prof_node;
        ptr_new_prof_node->ptr_next_node = NULL;
    }
    else
    {
        ptr_prev_node = *pptr_profile_list;
        ptr_curr_node = *pptr_profile_list;

        while (ptr_curr_node != NULL)
        {
            if (ptr_curr_node->ptr_profile->priority <= ptr_new_profile->priority)
            {
                DIAG_PRINT(HAL_DBG_PROFILE,
                                "find prof id=%d (%s) higher priority=%d, search next\n",
                                ptr_curr_node->ptr_profile->id,
                                ptr_curr_node->ptr_profile->name,
                                ptr_curr_node->ptr_profile->priority);
                /* Search the next node */
                ptr_prev_node = ptr_curr_node;
                ptr_curr_node = ptr_curr_node->ptr_next_node;
            }
            else
            {
                /* Insert intermediate node */
                ptr_new_prof_node->ptr_next_node = ptr_curr_node;
                DIAG_PRINT(HAL_DBG_PROFILE,
                                "insert prof id=%d (%s) before prof id=%d (%s) (priority=%d >= %d)\n",
                                ptr_new_prof_node->ptr_profile->id,
                                ptr_new_prof_node->ptr_profile->name,
                                ptr_curr_node->ptr_profile->id,
                                ptr_curr_node->ptr_profile->name,
                                ptr_new_prof_node->ptr_profile->priority,
                                ptr_curr_node->ptr_profile->priority);

                if (ptr_prev_node == ptr_curr_node)
                {
                    /* There is no previous node: change the root */
                    *pptr_profile_list = ptr_new_prof_node;
                    DIAG_PRINT(HAL_DBG_PROFILE,
                                    "insert prof id=%d (%s) to head (priority=%d)\n",
                                    ptr_new_prof_node->ptr_profile->id,
                                    ptr_new_prof_node->ptr_profile->name,
                                    ptr_new_prof_node->ptr_profile->priority);
                }
                else
                {
                    ptr_prev_node->ptr_next_node = ptr_new_prof_node;
                    DIAG_PRINT(HAL_DBG_PROFILE,
                                    "insert prof id=%d (%s) after prof id=%d (%s) (priority=%d <= %d)\n",
                                    ptr_new_prof_node->ptr_profile->id,
                                    ptr_new_prof_node->ptr_profile->name,
                                    ptr_prev_node->ptr_profile->id,
                                    ptr_prev_node->ptr_profile->name,
                                    ptr_new_prof_node->ptr_profile->priority,
                                    ptr_prev_node->ptr_profile->priority);
                }

                return (CLX_E_OK);
            }
        }

        /* Insert node to the tail of list */
        ptr_prev_node->ptr_next_node = ptr_new_prof_node;
        ptr_new_prof_node->ptr_next_node = NULL;
        DIAG_PRINT(HAL_DBG_PROFILE,
                        "insert prof id=%d (%s) to tail, after prof id=%d (%s) (priority=%d <= %d)\n",
                        ptr_new_prof_node->ptr_profile->id,
                        ptr_new_prof_node->ptr_profile->name,
                        ptr_prev_node->ptr_profile->id,
                        ptr_prev_node->ptr_profile->name,
                        ptr_new_prof_node->ptr_profile->priority,
                        ptr_prev_node->ptr_profile->priority);
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_addProfToAllIntf(
    HAL_PKT_NETIF_PROFILE_T         *ptr_new_profile)
{
    UI32_T                              port;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;

    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        /* Shall we check if the interface is ever created on the port?? */
        /* if (NULL != ptr_port_db->ptr_net_dev) */
        if (1)
        {
            _hal_pkt_addProfToList(ptr_new_profile, &ptr_port_db->ptr_profile_list);
        }
    }

    return (CLX_E_OK);
}

static HAL_PKT_NETIF_PROFILE_T *
_hal_pkt_delProfFromListById(
    const UI32_T                            id,
    HAL_PKT_PROFILE_NODE_T              **pptr_profile_list)
{
    HAL_PKT_PROFILE_NODE_T      *ptr_temp_node;
    HAL_PKT_PROFILE_NODE_T      *ptr_curr_node, *ptr_prev_node;
    HAL_PKT_NETIF_PROFILE_T     *ptr_profile = NULL;;

    if (NULL != *pptr_profile_list)
    {
        /* Check the 1st node */
        if (id == (*pptr_profile_list)->ptr_profile->id)
        {
            ptr_profile = (*pptr_profile_list)->ptr_profile;
            ptr_temp_node = (*pptr_profile_list);
            (*pptr_profile_list) = ptr_temp_node->ptr_next_node;

            if (NULL != ptr_temp_node->ptr_next_node)
            {
                DIAG_PRINT(HAL_DBG_PROFILE,
                                "choose prof id=%d (%s) as new head\n",
                                ptr_temp_node->ptr_next_node->ptr_profile->id,
                                ptr_temp_node->ptr_next_node->ptr_profile->name);
            }
            else
            {
                DIAG_PRINT(HAL_DBG_PROFILE,
                                "prof list is empty\n");
            }


            osal_free(ptr_temp_node);
        }
        else
        {
            ptr_prev_node = *pptr_profile_list;
            ptr_curr_node = ptr_prev_node->ptr_next_node;

            while (NULL != ptr_curr_node)
            {
                if (id != ptr_curr_node->ptr_profile->id)
                {
                    ptr_prev_node = ptr_curr_node;
                    ptr_curr_node = ptr_curr_node->ptr_next_node;
                }
                else
                {
                    DIAG_PRINT(HAL_DBG_PROFILE,
                                    "find prof id=%d, free done\n", id);

                    ptr_profile = ptr_curr_node->ptr_profile;
                    ptr_prev_node->ptr_next_node = ptr_curr_node->ptr_next_node;
                    osal_free(ptr_curr_node);
                    break;
                }
            }
        }
    }

    if (NULL == ptr_profile)
    {
        DIAG_PRINT((HAL_DBG_PROFILE | HAL_DBG_ERR),
                        "find prof failed, id=%d\n", id);
    }

    return (ptr_profile);
}


static CLX_ERROR_NO_T
_hal_pkt_delProfFromAllIntfById(
    const UI32_T                        id)
{
    UI32_T                              port;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;

    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        /* Shall we check if the interface is ever created on the port?? */
        /* if (NULL != ptr_port_db->ptr_net_dev) */
        if (1)
        {
            _hal_pkt_delProfFromListById(id, &ptr_port_db->ptr_profile_list);
        }
    }
    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_allocProfEntry(
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile)
{
    UI32_T          idx;

    for (idx=0; idx<HAL_PKT_NET_PROFILE_NUM_MAX; idx++)
    {
        if (NULL == _ptr_hal_pkt_profile_entry[idx])
        {
            DIAG_PRINT(HAL_DBG_PROFILE,
                            "alloc prof entry failed, id=%d\n", idx);
            _ptr_hal_pkt_profile_entry[idx] = ptr_profile;
            ptr_profile->id = idx;
            return (CLX_E_OK);
        }
    }
    return (CLX_E_TABLE_FULL);
}

static HAL_PKT_NETIF_PROFILE_T  *
_hal_pkt_freeProfEntry(
    const UI32_T                 id)
{
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile = NULL;

    if (id < HAL_PKT_NET_PROFILE_NUM_MAX)
    {
        ptr_profile = _ptr_hal_pkt_profile_entry[id];
        _ptr_hal_pkt_profile_entry[id] = NULL;
    }

    return (ptr_profile);
}

static CLX_ERROR_NO_T
_hal_pkt_destroyAllIntf(
    const UI32_T                        unit)
{
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_net_dev)       /* valid intf */
        {
            DIAG_PRINT(HAL_DBG_INTF,
                            "u=%u, find intf %s (id=%d) on phy port=%d, destroy done\n",
                            unit,
                            ptr_port_db->meta.name,
                            ptr_port_db->meta.port,
                            ptr_port_db->meta.port);

            netif_tx_disable(ptr_port_db->ptr_net_dev);
            unregister_netdev(ptr_port_db->ptr_net_dev);
            free_netdev(ptr_port_db->ptr_net_dev);

            /* Don't need to remove profiles on this port.
             * In fact, the profile is binding to "port" not "intf".
             */
            /* _hal_pkt_destroyProfList(ptr_port_db->ptr_profile_list); */

            osal_memset(ptr_port_db, 0x0, sizeof(HAL_PKT_NETIF_PORT_DB_T));
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_delProfListOnAllIntf(
    const UI32_T                        unit)
{
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    UI32_T                              port = 0;
    HAL_PKT_PROFILE_NODE_T          *ptr_curr_node, *ptr_next_node;

    /* Unregister net devices by id, although the "id" is now relavent to "port" we still perform a search */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
        if (NULL != ptr_port_db->ptr_profile_list)       /* valid intf */
        {
            ptr_curr_node = ptr_port_db->ptr_profile_list;
            while (NULL != ptr_curr_node)
            {
                DIAG_PRINT(HAL_DBG_PROFILE,
                                "u=%u, del prof id=%d on phy port=%d\n",
                                unit, ptr_curr_node->ptr_profile->id, port);

                ptr_next_node = ptr_curr_node->ptr_next_node;
                osal_free(ptr_curr_node);
                ptr_curr_node = ptr_next_node;
            }
        }
    }

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_destroyAllProfile(
    const UI32_T                        unit)
{
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile;
    UI32_T                              prof_id;

    _hal_pkt_delProfListOnAllIntf(unit);

    for (prof_id=0; prof_id<CLX_NETIF_PROFILE_NUM_MAX; prof_id++)
    {
        ptr_profile = _hal_pkt_freeProfEntry(prof_id);
        if (NULL != ptr_profile)
        {
            DIAG_PRINT(HAL_DBG_PROFILE,
                            "u=%u, destroy prof id=%d, name=%s, priority=%d, flag=0x%x\n",
                            unit,
                            ptr_profile->id,
                            ptr_profile->name,
                            ptr_profile->priority,
                            ptr_profile->flags);
            osal_free(ptr_profile);
        }
    }

    return (CLX_E_OK);
}

static HAL_PKT_NETIF_PROFILE_T  *
_hal_pkt_getProfEntry(
    const UI32_T                 id)
{
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile = NULL;

    if (id < HAL_PKT_NET_PROFILE_NUM_MAX)
    {
        if (NULL != _ptr_hal_pkt_profile_entry[id])
        {
            ptr_profile = _ptr_hal_pkt_profile_entry[id];
        }
    }

    return (ptr_profile);
}

static CLX_ERROR_NO_T
_hal_pkt_createProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    CLX_ERROR_NO_T                      rc;

    /* Lock all Rx tasks to avoid profiles being refered during packet processing */
    /* Need to lock all Rx tasks since packets from all Rx channels do profile lookup */
    ptr_cb->lock_all_rx_channel(unit);

    ptr_profile = osal_alloc(sizeof(HAL_PKT_NETIF_PROFILE_T));
    *ptr_profile = ptr_cookie->net_profile;

     DIAG_PRINT( HAL_DBG_PROFILE,
                    "u=%u, create prof name=%s, priority=%d, flag=0x%x\n",
                    unit,
                    ptr_profile->name,
                    ptr_profile->priority,
                    ptr_profile->flags);

    /* Save the profile to the profile array and assign the index to ptr_profile->id */
    rc = _hal_pkt_allocProfEntry(ptr_profile);
    if (CLX_E_OK == rc)
    {
        /* Insert the profile to the corresponding (port) interface */
        if ((ptr_profile->flags & HAL_PKT_NETIF_PROFILE_FLAGS_PORT) != 0)
        {
             DIAG_PRINT( HAL_DBG_PROFILE,
                            "u=%u, bind prof to phy port=%d\n", unit, ptr_profile->port);
            ptr_port_db = HAL_PKT_GET_PORT_DB(ptr_profile->port);
            _hal_pkt_addProfToList(ptr_profile, &ptr_port_db->ptr_profile_list);
        }
        else
        {
             DIAG_PRINT( HAL_DBG_PROFILE,
                            "u=%u, bind prof to all intf\n", unit);
            _hal_pkt_addProfToAllIntf(ptr_profile);
        }

        /* Copy the ptr_profile->id to user space */
        ptr_cookie->net_profile = *ptr_profile;
    }
    else
    {
         DIAG_PRINT(( HAL_DBG_PROFILE |  HAL_DBG_ERR),
                        "u=%u, alloc prof entry failed, tbl full\n", unit);
        osal_free(ptr_profile);
    }

    ptr_cookie->rc = rc;

    ptr_cb->unlock_all_rx_channel(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_destroyProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    HAL_PKT_NETIF_PROFILE_T         profile = ptr_cookie->net_profile;
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    /* Lock all Rx tasks to avoid profiles being refered during packet processing */
    /* Need to lock all Rx tasks since packets from all Rx channels do profile lookup */
    ptr_cb->lock_all_rx_channel(unit);

    /* Remove the profile from corresponding interface (port) */
    _hal_pkt_delProfFromAllIntfById(profile.id);

    ptr_profile = _hal_pkt_freeProfEntry(profile.id);
    if (NULL != ptr_profile)
    {
         DIAG_PRINT( HAL_DBG_PROFILE,
                        "u=%u, destroy prof id=%d, name=%s, priority=%d, flag=0x%x\n",
                        unit,
                        ptr_profile->id,
                        ptr_profile->name,
                        ptr_profile->priority,
                        ptr_profile->flags);
        osal_free(ptr_profile);
    }

    ptr_cookie->rc = rc;

    ptr_cb->unlock_all_rx_channel(unit);

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_getProfile(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_NETIF_PROFILE_T         profile = ptr_cookie->net_profile;
    HAL_PKT_NETIF_PROFILE_T         *ptr_profile;
    CLX_ERROR_NO_T                      rc = CLX_E_OK;

    ptr_profile = _hal_pkt_getProfEntry(profile.id);
    if (NULL != ptr_profile)
    {
        ptr_cookie->net_profile = *ptr_profile;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    ptr_cookie->rc = rc;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_getIntfCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_NETIF_INTF_T            net_intf = ptr_cookie->net_intf;
    HAL_PKT_NETIF_INTF_CNT_T        intf_cnt = {0};
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
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

    ptr_cookie->cnt = intf_cnt;
    ptr_cookie->rc = rc;

    return (CLX_E_OK);
}

static CLX_ERROR_NO_T
_hal_pkt_clearIntfCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_NETIF_COOKIE_T    *ptr_cookie = ptr_data;
    HAL_PKT_NETIF_INTF_T            net_intf = ptr_cookie->net_intf;
    HAL_PKT_NETIF_PORT_DB_T         *ptr_port_db;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port = 0;
    CLX_ERROR_NO_T                      rc = CLX_E_ENTRY_NOT_FOUND;

    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_port_db = HAL_PKT_GET_PORT_DB(port);
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

    ptr_cookie->rc = rc;

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_getTxKnlCnt
 * PURPOSE:
 *      To get the PDMA TX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the TX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_getTxKnlCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_getRxKnlCnt
 * PURPOSE:
 *      To get the PDMA RX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully get the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_getRxKnlCnt(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_clearTxKnlCnt
 * PURPOSE:
 *      To clear the PDMA TX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the TX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully clear the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_clearTxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_clearRxKnlCnt
 * PURPOSE:
 *      To clear the PDMA RX counters of the target channel.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully clear the counters.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_clearRxKnlCnt(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_setPortAttr
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
hal_pkt_setPortAttr(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_IOCTL_PORT_COOKIE_T     *ptr_cookie = ptr_data;
#define HAL_PKT_PORT_STATUS_UP          (1)
#define HAL_PKT_PORT_STATUS_DOWN        (0)
    struct net_device                   *ptr_net_dev;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port;
    UI32_T                              status;
    CLX_PORT_SPEED_T                    speed;

    port = ptr_cookie->port;
    status = ptr_cookie->status;
    speed = ptr_cookie->speed;

    ptr_net_dev = HAL_PKT_GET_PORT_NETDEV(port);
    if ((NULL != ptr_net_dev) && (port<HAL_PKT_MAX_PORT_NUM))
    {
        if (HAL_PKT_PORT_STATUS_UP == status)
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
                ptr_priv->speed = 25000;
                break;
            case CLX_PORT_SPEED_40G:
                ptr_priv->speed = 40000;
                break;
            case CLX_PORT_SPEED_50G:
                ptr_priv->speed = 50000;
                break;
            case CLX_PORT_SPEED_100G:
                ptr_priv->speed = 100000;
                break;
            case CLX_PORT_SPEED_200G:
                ptr_priv->speed = 200000;
                break;
            case CLX_PORT_SPEED_400G:
                ptr_priv->speed = 400000;
                break;
            default:
                break;
        }
    }
    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_getPortAttr
 * PURPOSE:
 *      To get the port attributes such as status or speeds.
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
hal_pkt_getPortAttr(
    const UI32_T                              unit,
    void                                      *ptr_data)
{
    HAL_PKT_IOCTL_PORT_COOKIE_T     *ptr_cookie = ptr_data;
    struct net_device                   *ptr_net_dev;
    struct net_device_priv              *ptr_priv;
    UI32_T                              port;
    UI32_T                              status;
    CLX_PORT_SPEED_T                    speed;

    port = ptr_cookie->port;

    ptr_net_dev = HAL_PKT_GET_PORT_NETDEV(port);
    if ((NULL == ptr_net_dev) || (port >= HAL_PKT_MAX_PORT_NUM))
    {
         DIAG_PRINT( HAL_DBG_ERR,
            "%s(%d): Failed to get netdev, port %d\n",
                __FUNCTION__, __LINE__, port);
        return -1;
    }
    status = netif_carrier_ok(ptr_net_dev);

    ptr_priv = netdev_priv(ptr_net_dev);
    switch(ptr_priv->speed)
    {
        case SPEED_1000:
            speed = CLX_PORT_SPEED_1G;
            break;
        case SPEED_10000:
            speed = CLX_PORT_SPEED_10G;
            break;
        case 25000:
            speed = CLX_PORT_SPEED_25G;
            break;
        case 40000:
            speed = CLX_PORT_SPEED_40G;
            break;
        case 50000:
            speed = CLX_PORT_SPEED_50G;
            break;
        case 100000:
            speed = CLX_PORT_SPEED_100G;
            break;
        case 200000:
            speed = CLX_PORT_SPEED_200G;
            break;
        case 400000:
            speed = CLX_PORT_SPEED_400G;
            break;
        default:
             DIAG_PRINT( HAL_DBG_ERR,
                               "%s(%d): Unknown speed %d, port %d\n",
                               __FUNCTION__, __LINE__, ptr_priv->speed, port);
            speed = CLX_PORT_SPEED_400G;
            break;
    }
    ptr_cookie->status = status;
    ptr_cookie->speed = speed;
    return (CLX_E_OK);
}


static CLX_ERROR_NO_T
_hal_pkt_setIntfProperty(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;

    intf_id = ptr_cookie->intf_id;
    property = ptr_cookie->property;
    param0 = ptr_cookie->param0;
    param1 = ptr_cookie->param1;

    ptr_cb->lock_all_rx_channel(unit);

    rc = netif_nl_setIntfProperty(unit, intf_id, property, param0, param1);

    ptr_cb->unlock_all_rx_channel(unit);

    ptr_cookie->rc = rc;

    return (rc);
}

static CLX_ERROR_NO_T
_hal_pkt_getIntfProperty(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;
    UI32_T                              intf_id;
    NETIF_NL_INTF_PROPERTY_T            property;
    UI32_T                              param0;
    UI32_T                              param1;
    CLX_ERROR_NO_T                      rc;

    intf_id = ptr_cookie->intf_id;
    property = ptr_cookie->property;
    param0 = ptr_cookie->param0;

    rc = netif_nl_getIntfProperty(unit, intf_id, property, &param0, &param1);

    ptr_cookie->param0 = param0;
    ptr_cookie->param1 = param1;
    ptr_cookie->rc = rc;
    return (rc);
}

static CLX_ERROR_NO_T
_hal_pkt_createNetlink(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    NETIF_NL_NETLINK_T                  netlink;
    UI32_T                              netlink_id;
    CLX_ERROR_NO_T                      rc;

    memcpy(&netlink, &ptr_cookie->netlink, sizeof(NETIF_NL_NETLINK_T));

    ptr_cb->lock_all_rx_channel(unit);

    rc = netif_nl_createNetlink(unit, &netlink, &netlink_id);

    ptr_cb->unlock_all_rx_channel(unit);

    ptr_cookie->netlink.id = netlink_id;
    ptr_cookie->rc = rc;
    return (rc);
}

static CLX_ERROR_NO_T
_hal_pkt_destroyNetlink(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_PKT_NL_IOCTL_COOKIE_T       *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    UI32_T                          netlink_id;
    CLX_ERROR_NO_T                  rc;

    netlink_id = ptr_cookie->netlink.id;

    ptr_cb->lock_all_rx_channel(unit);

    rc = netif_nl_destroyNetlink(unit, netlink_id);

    ptr_cb->unlock_all_rx_channel(unit);

    ptr_cookie->rc = rc;

    return (rc);
}

static CLX_ERROR_NO_T
_hal_pkt_getNetlink(
    const UI32_T                        unit,
    void                                *ptr_data)
{
    HAL_PKT_NL_IOCTL_COOKIE_T *ptr_cookie = ptr_data;
    UI32_T                              id;
    NETIF_NL_NETLINK_T                  netlink;
    CLX_ERROR_NO_T                      rc;

    id = ptr_cookie->netlink.id;

    rc = netif_nl_getNetlink(unit, id, &netlink);
    if (CLX_E_OK == rc)
    {
        memcpy(&ptr_cookie->netlink, &netlink, sizeof(NETIF_NL_NETLINK_T));
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    ptr_cookie->rc = rc;

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_pkt_resumeAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            if (netif_queue_stopped(ptr_net_dev))
            {
                netif_wake_queue(ptr_net_dev);
            }
        }
    }

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_pkt_suspendAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            netif_stop_queue(ptr_net_dev);
        }
    }

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_pkt_stopAllIntf(
    const UI32_T                        unit)
{
    struct net_device                   *ptr_net_dev = NULL;
    UI32_T                              port;

    /* Unregister net devices by id */
    for (port = 0; port < HAL_PKT_MAX_PORT_NUM; port++)
    {
        ptr_net_dev = HAL_PKT_GET_PORT_NETDEV(port);
        if (NULL != ptr_net_dev)
        {
            netif_tx_disable(ptr_net_dev);
        }
    }

    return (CLX_E_OK);
}

/* FUNCTION NAME: hal_pkt_initPktDrv
 * PURPOSE:
 *      To invoke the functions to initialize the control block for each
 *      PDMA subsystem.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully initialize the control blocks.
 *      CLX_E_OTHERS    --  Initialize the control blocks failed.
 * NOTES:
 *      None
 */

CLX_ERROR_NO_T
hal_pkt_initPktDrv(
    const UI32_T            unit,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T        *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);

    if(ptr_cb->init_stage != HAL_PKT_INIT_START)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, pkt drv init failed. init_stage=%d\n", unit, ptr_cb->init_stage);
        return rc;
    }
    rc = ptr_cb->pkt_init_drv(unit);

    if(rc == CLX_E_OK)
    {  
        ptr_cb->init_stage = HAL_PKT_INIT_DRV;
        DIAG_PRINT(HAL_DBG_COMMON,
                        "u=%u, pkt drv init done, next_stage=%d\n", unit, ptr_cb->init_stage);

        rc = ptr_cb->pkt_init_irq(unit);
    }
    return (rc);
}

/* FUNCTION NAME: hal_pkt_initTask
 * PURPOSE:
 *      To initialize the Task for packet module.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully dinitialize the control block.
 *      CLX_E_OTHERS    --  Initialize the control block failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_initTask(
    const UI32_T            unit,
    void                    *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T        *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);

    if(ptr_cb->init_stage != HAL_PKT_INIT_DRV)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, pkt task init failed. init_stage=%d\n", unit, ptr_cb->init_stage);
        return rc;
    }

    rc = ptr_cb->pkt_init_task(unit);

    if(rc == CLX_E_OK)
    {
        ptr_cb->init_stage = HAL_PKT_INIT_TASK;
        DIAG_PRINT(HAL_DBG_COMMON,
                        "u=%u, pkt task init done, next_stage=%d\n", unit, ptr_cb->init_stage);
    }

    /* For some specail case in warmboot, the netifs are not destroyed during sdk deinit
     * but stopped, here we need to resume them with the original carrier status
     */
    hal_pkt_resumeAllIntf(unit);

    return (rc);
}

/* FUNCTION NAME: hal_pkt_deinitPktDrv
 * PURPOSE:
 *      To invoke the functions to de-initialize the control block for each
 *      PDMA subsystem.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully de-initialize the control blocks.
 *      CLX_E_OTHERS    --  De-initialize the control blocks failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_deinitPktDrv(
    const UI32_T            unit,
    void                    *ptr_data)
{
    HAL_PKT_DRV_CB_T    *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    if(ptr_cb->init_stage != HAL_PKT_INIT_DRV)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, pkt drv deinit failed, init_stage=%d\n", unit, ptr_cb->init_stage);
        return rc;
    }

    ptr_cb->pkt_deinit_drv(unit);

    ptr_cb->init_stage = HAL_PKT_INIT_START;

    DIAG_PRINT(HAL_DBG_COMMON,
                    "u=%u, pkt drv deinit done, init_stage=0x%x\n",
                    unit, ptr_cb->init_stage);
    return (rc);
}
CLX_ERROR_NO_T hal_pkt_rx_stop(
    const UI32_T        unit)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T    *ptr_cb    = HAL_PKT_GET_DRV_CB_PTR(unit);
    if(ptr_cb->init_stage != HAL_PKT_INIT_RX_START)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                    "u=%u, rx stop failed, not started. init_stage=%d\n", unit, ptr_cb->init_stage);
        return rc;
    }
    rc = ptr_cb->pkt_rx_stop(unit);

    ptr_cb->init_stage = HAL_PKT_INIT_TASK;
    DIAG_PRINT(HAL_DBG_RX,
                "u=%u, rx stop done, init_stage=0x%x\n", unit, ptr_cb->init_stage);
                
    return rc;
}
/* FUNCTION NAME: hal_pkt_deinitTask
 * PURPOSE:
 *      To de-initialize the Task for packet module.
 * INPUT:
 *      unit            --  The unit ID
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        --  Successfully dinitialize the control block.
 *      CLX_E_OTHERS    --  Initialize the control block failed.
 * NOTES:
 *      None
 */
CLX_ERROR_NO_T
hal_pkt_deinitTask(
    const UI32_T            unit,
    void                    *ptr_data)
{
    HAL_PKT_DRV_CB_T        *ptr_cb    = HAL_PKT_GET_DRV_CB_PTR(unit);

    /* In case that some undestroyed net intf keep Tx after task deinit */
    hal_pkt_stopAllIntf(unit);

    /* Need to stop Rx before de-init Task */
    if(ptr_cb->init_stage == HAL_PKT_INIT_RX_START)
    {
        hal_pkt_rx_stop(unit);
    }
    
    if(ptr_cb->init_stage != HAL_PKT_INIT_TASK)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, pkt task deinit failed, init_stage=%d\n", unit, ptr_cb->init_stage);
        return CLX_E_OK;
    }
    
    ptr_cb->pkt_deinit_task(unit);

    /* Set the flag to record init state */
    ptr_cb->init_stage = HAL_PKT_INIT_DRV;

    DIAG_PRINT(HAL_DBG_RX,
                    "u=%u, pkt task deinit done, init_stage=0x%x\n",
                    unit, ptr_cb->init_stage);

    return (CLX_E_OK);
}
CLX_ERROR_NO_T
hal_pkt_setRxKnlConfig(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_PKT_IOCTL_RX_COOKIE_T       *ptr_cookie = (HAL_PKT_IOCTL_RX_COOKIE_T*)ptr_data;
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);
    HAL_PKT_IOCTL_RX_TYPE_T         rx_type = ptr_cookie->rx_type;

     DIAG_PRINT(HAL_DBG_DEBUG,
        "rx_type:%x.ptr_cookie->buf_len:%x,unit:%x,channel:%x,ioctl_gpd_addr:%llx\n",
        rx_type,ptr_cookie->buf_len,ptr_cookie->unit,ptr_cookie->channel,ptr_cookie->ioctl_gpd_addr);
        
    if (HAL_PKT_IOCTL_RX_TYPE_DEINIT == rx_type)
    {
        rc = hal_pkt_rx_stop(unit);
    }
    else if (HAL_PKT_IOCTL_RX_TYPE_INIT == rx_type)
    {
        /* To prevent buffer size from being on-the-fly changed */
        if(ptr_cb->init_stage != HAL_PKT_INIT_TASK)
        {
            DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, rx start failed. init_stage=%d\n", unit, ptr_cb->init_stage);
            return rc;
        }

        ptr_cb->buf_len = ptr_cookie->buf_len;
        rc = ptr_cb->pkt_rx_start(unit);


        ptr_cb->init_stage = HAL_PKT_INIT_RX_START;
        DIAG_PRINT(HAL_DBG_RX,
                    "u=%u, rx start done, init_stage=%d\n", unit, ptr_cb->init_stage);
    }

    return (rc);
}
/* FUNCTION NAME: hal_pkt_getRxKnlConfig
 * PURPOSE:
 *      To get the Rx subsystem configuration.
 * INPUT:
 *      unit            -- The unit ID
 *      ptr_cookie      -- Pointer of the RX cookie
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK        -- Successfully configure the RX parameters.
 *      CLX_E_OTHERS    -- Configure the parameter failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
hal_pkt_getRxKnlConfig(
    const UI32_T                    unit,
    void                            *ptr_data)
{
    HAL_PKT_IOCTL_RX_COOKIE_T       *ptr_cookie = ptr_data;
    HAL_PKT_DRV_CB_T                *ptr_cb = HAL_PKT_GET_DRV_CB_PTR(unit);

    ptr_cookie->buf_len = ptr_cb->buf_len;

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_register_netif_common_ioctl(void)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    /* network interface */
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_INTF,
        _hal_pkt_createIntf);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_INTF,
        _hal_pkt_destroyIntf);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF,
        _hal_pkt_getIntf);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CREATE_PROFILE,
        _hal_pkt_createProfile);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_DESTROY_PROFILE,
        _hal_pkt_destroyProfile);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_PROFILE,
        _hal_pkt_getProfile);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_INTF_CNT,
        _hal_pkt_getIntfCnt);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_INTF_CNT,
        _hal_pkt_clearIntfCnt);


    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_SET_RX_CFG,
        hal_pkt_setRxKnlConfig);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CFG,
        hal_pkt_getRxKnlConfig);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_TASK,
        hal_pkt_deinitTask);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_DEINIT_DRV,
        hal_pkt_deinitPktDrv);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_INIT_TASK,
        hal_pkt_initTask);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_INIT_DRV,
        hal_pkt_initPktDrv);
    // /* counter */
    // _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_TX_CNT,
    //     hal_pkt_getTxKnlCnt);
    // _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_RX_CNT,
    //     hal_pkt_getRxKnlCnt);
    // _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_TX_CNT,
    //     hal_pkt_clearTxKnlCnt);
    // _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_CLEAR_RX_CNT,
    //     hal_pkt_clearRxKnlCnt);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_SET_PORT_ATTR,
        hal_pkt_setPortAttr);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_GET_PORT_ATTR,
        hal_pkt_getPortAttr);

    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_NL_SET_INTF_PROPERTY,
        _hal_pkt_setIntfProperty);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_INTF_PROPERTY,
        _hal_pkt_getIntfProperty);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_NL_CREATE_NETLINK,
        _hal_pkt_createNetlink);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_NL_DESTROY_NETLINK,
        _hal_pkt_destroyNetlink);
    _osal_mdc_registerIoctlCallback(OSAL_MDC_IOCTL_TYPE_NETIF_NL_GET_NETLINK,
        _hal_pkt_getNetlink);

    return rc;
}

/* Init/Deinit */
CLX_ERROR_NO_T
hal_netif_pkt_init(
    const UI32_T            unit)
{
    HAL_PKT_DRV_CB_T        *ptr_cb    = HAL_PKT_GET_DRV_CB_PTR(unit);
    
    if(ptr_cb->init_stage != HAL_PKT_INIT_START)
    {
        DIAG_PRINT(HAL_DBG_ERR,
                        "u=%u, netif current init_stage=%d\n", unit, ptr_cb->init_stage);
        DIAG_PRINT( HAL_DBG_ERR,
           "BUG!!! u=%u, looks the users may kill SDK app without a de-init flow\n", unit);
        return CLX_E_OK;
    }
    hal_register_netif_common_ioctl();

    /* Init Thread */
    osal_init();

    /* Reset all database*/
    osal_memset(_hal_pkt_port_db, 0x0,
            HAL_PKT_MAX_PORT_NUM * sizeof(HAL_PKT_NETIF_PORT_DB_T));

    osal_memset(_hal_pkt_drv_cb, 0x0,
            CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM * sizeof(HAL_PKT_DRV_CB_T));

    if(CLX_DEVICE_LIGHTNING == clx_get_device_type(unit))
    {
        hal_lightning_register_drv_cb(unit);
    }
    else if(CLX_DEVICE_DAWN == clx_get_device_type(unit))
    {
        hal_dawn_register_drv_cb(unit);
    }
    else if(CLX_DEVICE_NB == clx_get_device_type(unit))
    {
        hal_nb_register_drv_cb(unit);
    }

    netif_nl_init();

    return (CLX_E_OK);
}


CLX_ERROR_NO_T
hal_netif_pkt_exit(
    const UI32_T            unit)
{
    /* 1st. Stop all netdev (if any) to prevent kernel from Tx new packets */
    hal_pkt_stopAllIntf(unit);

    /* 2nd. Stop Rx HW DMA and free all the DMA buffer hooked on the ring */
    hal_pkt_rx_stop(unit);

    /* 3rd. Need to wait Rx done task process all the availavle packets on GPD ring */

    /* 4th. Stop all the internal tasks (if any) */
    hal_pkt_deinitTask(unit,NULL);

    /* 5th. Deinit pkt driver for common database/interrupt source (if required) */
    hal_pkt_deinitPktDrv(unit,NULL);

    /* 6th. Clean up those intf/profiles not been destroyed */
    _hal_pkt_destroyAllProfile(unit);
    _hal_pkt_destroyAllIntf(unit);

    osal_deinit();

    return (CLX_E_OK);
}
