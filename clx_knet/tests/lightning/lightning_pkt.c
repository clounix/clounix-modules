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
#include <hal_lightning_pkt_knl.h>
#include <clx_types.h>
#include <osal/osal_mdc.h>
#include <osal/osal.h>
static int                          _hal_lightning_pkt_dev_fd = 0;
static int                          _hal_clx_dev_fd = 0;
static OSAL_MDC_CB_T                _osal_mdc_cb;
static HAL_LIGHTNING_PKT_DRV_CB_T         _hal_lightning_pkt_drv_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_LIGHTNING_PKT_TX_CB_T          _hal_lightning_pkt_tx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];
static HAL_LIGHTNING_PKT_RX_CB_T          _hal_lightning_pkt_rx_cb[CLX_CFG_MAXIMUM_CHIPS_PER_SYSTEM];

#define HAL_LIGHTNING_PKT_GET_DRV_CB_PTR(unit)                (&_hal_lightning_pkt_drv_cb[unit])
/*---------------------------------------------------------------------------*/
#define HAL_LIGHTNING_PKT_GET_TX_CB_PTR(unit)                 (&_hal_lightning_pkt_tx_cb[unit])
#define HAL_LIGHTNING_PKT_GET_TX_PDMA_PTR(unit, channel)      (&_hal_lightning_pkt_tx_cb[unit].pdma[channel])
#define HAL_LIGHTNING_PKT_GET_TX_GPD_PTR(unit, channel, gpd)  (&_hal_lightning_pkt_tx_cb[unit].pdma[channel].ptr_gpd_align_start_addr[gpd])
/*---------------------------------------------------------------------------*/
#define HAL_LIGHTNING_PKT_GET_RX_CB_PTR(unit)                 (&_hal_lightning_pkt_rx_cb[unit])
#define HAL_LIGHTNING_PKT_GET_RX_PDMA_PTR(unit, channel)      (&_hal_lightning_pkt_rx_cb[unit].pdma[channel])
#define HAL_LIGHTNING_PKT_GET_RX_GPD_PTR(unit, channel, gpd)  (&_hal_lightning_pkt_rx_cb[unit].pdma[channel].ptr_gpd_align_start_addr[gpd])

#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN)
#define HAL_LIGHTNING_PKT_CAST_PTR_TO_CLX_ADDR_T(__ptr__)     (((CLX_ADDR_T)((CLX_HUGE_T)__ptr__)) & 0xFFFFFFFF)
#else
#define HAL_LIGHTNING_PKT_CAST_PTR_TO_CLX_ADDR_T(__ptr__)     (((CLX_ADDR_T)((CLX_HUGE_T)__ptr__)) & 0xFFFFFFFFFFFFFFFF)
#endif


#if defined(CLX_EN_HOST_32_BIT_BIG_ENDIAN) || defined(CLX_EN_HOST_32_BIT_LITTLE_ENDIAN)
#define HAL_LIGHTNING_PKT_PTR_64_HI(__ptr__)                  (0)
#define HAL_LIGHTNING_PKT_PTR_64_LOW(__ptr__)                 ((UI32_T)__ptr__)
#define HAL_LIGHTNING_PKT_PTR_32_TO_64(__hi32__, __low32__)   ((void *)__low32__)
#else
#define HAL_LIGHTNING_PKT_PTR_64_HI(__ptr__)                  (((unsigned long long int)__ptr__) >> 32)
#define HAL_LIGHTNING_PKT_PTR_64_LOW(__ptr__)                 (((unsigned long long int)__ptr__) & 0xFFFFFFFF)
#define HAL_LIGHTNING_PKT_PTR_32_TO_64(__hi32__, __low32__)   ((void *)(((unsigned long long int)(__low32__)) |  \
                                                                  ((unsigned long long int)(__hi32__) << 32)))
#endif

static CLX_ERROR_NO_T
_hal_lightning_pkt_ioctl(
    const UI32_T                    unit,
    const HAL_LIGHTNING_PKT_IOCTL_TYPE_T  type,
    const void                      *ptr_arg)
{
    int                             ret = 0;
    CLX_ERROR_NO_T                  rc = CLX_E_OK;
    HAL_LIGHTNING_PKT_IOCTL_CMD_T         cmd = {0};

    cmd.field.unit = unit;
    cmd.field.type = type;
    cmd.field.rsvd = 0x0;

    ret = ioctl(_hal_lightning_pkt_dev_fd, (long unsigned int)cmd.value, ptr_arg);
    if (0 != ret)
    {
        printf("u=%u, ioctl failed, type=%d, errno=%d.\n", unit, type, errno);
        rc = CLX_E_OTHERS;
    }

    return (rc);
}

CLX_ERROR_NO_T
hal_lightning_pkt_initPktDrv(
    const UI32_T            unit)
{
    HAL_LIGHTNING_PKT_TX_CB_T     *ptr_tx_cb = HAL_LIGHTNING_PKT_GET_TX_CB_PTR(unit);
    HAL_LIGHTNING_PKT_RX_CB_T     *ptr_rx_cb = HAL_LIGHTNING_PKT_GET_RX_CB_PTR(unit);

    if (0 != mknod(HAL_LIGHTNING_PKT_DRIVER_PATH, S_IFCHR,
                   makedev(HAL_LIGHTNING_PKT_DRIVER_MAJOR_NUM, HAL_LIGHTNING_PKT_DRIVER_MINOR_NUM)))
    {
        if (EEXIST != errno){
            return (CLX_E_OTHERS);
        }
    }

    _hal_lightning_pkt_dev_fd = open(HAL_LIGHTNING_PKT_DRIVER_PATH, O_RDWR | O_SYNC);

    if (_hal_lightning_pkt_dev_fd > 0)
    {
        _hal_lightning_pkt_ioctl(unit, HAL_LIGHTNING_PKT_IOCTL_TYPE_INIT_DRV, NULL);

        /* Clear Tx debug counters */
        memset(&ptr_tx_cb->cnt, 0x0, sizeof(HAL_LIGHTNING_PKT_TX_CNT_T));

        /* Clear Rx debug counters */
        memset(&ptr_rx_cb->cnt, 0x0, sizeof(HAL_LIGHTNING_PKT_RX_CNT_T));

        /* Sync semaphore to signal rxTask */
       // osal_createSemaphore("RX_SYNC", CLX_SEMAPHORE_SYNC, &ptr_rx_cb->sync_sema);
       // osal_createSemaphore("RX_DEINIT", CLX_SEMAPHORE_SYNC, &ptr_rx_cb->deinit_sema);
    }

    /* Register callback to ifmon for acquiring link status */
   // clx_ifmon_register(unit, (CLX_IFMON_NOTIFY_FUNC_T)hal_lightning_pkt_portStateChangeCallback, NULL);

    /* Check if dma enhancement is enabled */
    //_hal_lightning_pkt_initDmaConfig(unit);

    return (CLX_E_OK);
}

CLX_ERROR_NO_T
hal_lightning_pkt_deinitPktDrv(
    const UI32_T            unit)
{
    if (_hal_lightning_pkt_dev_fd > 0){
        _hal_lightning_pkt_ioctl(unit, HAL_LIGHTNING_PKT_IOCTL_TYPE_DEINIT_DRV, NULL);
    }

    close(_hal_lightning_pkt_dev_fd);

    if (0 != remove(HAL_LIGHTNING_PKT_DRIVER_PATH)){
        return (CLX_E_OTHERS);
    }

    return (CLX_E_OK);
}

