#include <linux/types.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/netdevice.h>
#include <asm/io.h>

#include <linux/version.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#include "clx_types.h"
#include "hal_dev.h"
#include "netif_osal.h"
#include "clx_error.h"
#include "hal_dflt.h"
#include "clx_nb.h"


char nb_driver_name[] = "nb_driver";

enum clx_chip_family {
    clx_dawn,
    clx_lightning,
    clx_nb,
    clx_cosim,
};

typedef struct {
    CLX_HUGE_T                      que_id;
    CLX_SEMAPHORE_ID_T              sema;
    UI32_T                          len;      /* Software CPU queue maximum length.        */
    UI32_T                          weight;   /* The weight for thread de-queue algorithm. */

} HAL_NB_PDMA_SW_QUEUE_T;

typedef struct  {
    UI32_T          channel;
    struct pci_dev  *pdev;
} HAL_NB_PDMA_TASK_COOKIE;

typedef struct {
    UI16_T                  device_id;
    UI16_T                  vendor_id;
    UI8_T                   revision_id;
} HAL_NB_PCI_DEV_ID;

typedef struct {
    CLX_ADDR_T              src_phy_addr[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    CLX_ADDR_T              dst_phy_addr[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    UI32_T                  channel_mode[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    UI32_T                  data_len[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    UI32_T                  channel_inited[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
} PDMA_CHANNEL_TEST_ATTR;//for test

struct clxdev_data{
    struct pci_dev          *pdev;
    struct mutex            lock;
    void __iomem            *mmio;
    CLX_THREAD_ID_T         *pdma_task[HAL_NB_PDMA_TX_CHANNEL_LAST]; 
    CLX_THREAD_ID_T         *pdma_dequeue_task; 

    HAL_NB_PDMA_DESC_T      *pdma_ring_base[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    HAL_NB_PDMA_DESC_T      *pdma_ring_base_align[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];
    UI32_T                  pdma_ring_size[HAL_NB_PDMA_GENERAL_CHANNEL_LAST];

    HAL_NB_PDMA_SW_QUEUE_T  sw_queue[HAL_NB_PDMA_TX_CHANNEL_LAST];
    UI32_T                  deque_idx;
    HAL_NB_PCI_DEV_ID       id;
    PDMA_CHANNEL_TEST_ATTR  channel_attr;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(5,10,0)
#if defined(OSAL_MDC_DMA_RESERVED_MEM_CACHEABLE)
    #define IOREMAP_API(a, b)       ioremap(a, b)
#else
    #define IOREMAP_API(a, b)       ioremap_nocache(a, b)
#endif
#else
    #define IOREMAP_API(a, b)       ioremap(a, b)
#endif

static uint loglevel = LOG_INFO | LOG_WARNING | LOG_ERR;

static CLX_ERROR_NO_T clxdev_get_pci_mmio_info(
    struct pci_dev      *pdev,
    UI32_T              **pptr_base_addr)
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    CLX_ADDR_T          phy_addr;
    UI32_T              reg_space_sz;

    phy_addr     = pci_resource_start(pdev, 0x0);
    reg_space_sz = pci_resource_len(pdev, 0x0);

    ret = pci_request_region(pdev, 0x0, nb_driver_name);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"request pci region failed\n"); 
        return ret;
    }

    *pptr_base_addr = (UI32_T*)IOREMAP_API(phy_addr, reg_space_sz);
    if (NULL == *pptr_base_addr) {
        clx_print(ERR,"ioremap faild\n");
        ret = CLX_E_OTHERS;
    }

    clx_print(DEBUG,"clx_dev mmio phy_addr:0x%llx,virt_addr:%p,size:0x%x\n",
                phy_addr,*pptr_base_addr,reg_space_sz);
    return ret;
}

CLX_ERROR_NO_T clxdev_read_pci_reg(struct pci_dev *pdev, const UI32_T offset,
                                    void *ptr_data,const UI32_T width) 
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    volatile UI32_T     *ptr_base_addr = privdata->mmio;
    UI32_T              idx;
    UI32_T              count = width / HAL_NB_PCI_DEV_BUS_WIDTH;
    
    CLX_CHECK_NULL_POINTER(ptr_data);
    CLX_CHECK_NULL_POINTER(ptr_base_addr);

    if((width % HAL_NB_PCI_DEV_BUS_WIDTH) || (count < 1)) {
        clx_print(ERR,"offset:0x%x,data width:%d\n",offset,width);
        return CLX_E_BAD_PARAMETER;
    }

    for (idx = 0; idx < count; idx++) {
        clx_print(DEBUG,"offset:0x%x,ptr_data:0x%x,width:%d\n",
                  offset + idx*4,*((UI32_T*)ptr_data + idx),width);
        *((UI32_T*)ptr_data + idx) = *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset + idx * 4));
    }
    return rc;
}

CLX_ERROR_NO_T clxdev_write_pci_reg(struct pci_dev *pdev, const UI32_T offset, 
                                    const void *ptr_data,const UI32_T width) 
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    volatile UI32_T     *ptr_base_addr = privdata->mmio;
    UI32_T              idx;
    UI32_T              count = width / HAL_NB_PCI_DEV_BUS_WIDTH;
    
    CLX_CHECK_NULL_POINTER(ptr_data);
    CLX_CHECK_NULL_POINTER(ptr_base_addr);

    if((width % HAL_NB_PCI_DEV_BUS_WIDTH) || (count < 1)) {
        clx_print(ERR,"offset:0x%x,data width:%d\n",offset,width);
        return CLX_E_BAD_PARAMETER;
    }

    for (idx = 0; idx < count; idx++) {
        clx_print(DEBUG,"offset:0x%x,ptr_data:0x%x,width:%d\n",
                  offset + idx*4,*((UI32_T*)ptr_data + idx),width);
        *((UI32_T *)((CLX_HUGE_T)ptr_base_addr + offset + idx * 4)) = *((UI32_T*)ptr_data + idx);
    }
    return rc;
}

static int clxdev_pdma_enable_channel(struct pci_dev* pdev,UI32_T channel)
{
    UI32_T              enable_channel = HAL_NB_PDMA_ENABLE_CHANNEL;
    
    return clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ENABLE_REG(channel)),
                        &enable_channel,sizeof(UI32_T));
}
static int clxdev_pdma_disable_channel(struct pci_dev* pdev,UI32_T channel)
{
    UI32_T              enable_channel = HAL_NB_PDMA_DISABLE_CHANNEL;
    
    return clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ENABLE_REG(channel)),
                        &enable_channel,sizeof(UI32_T));
}



static int clxdev_init_pdma_one_rx_desc(struct pci_dev* pdev,HAL_NB_PDMA_DESC_T volatile * descriptor) 
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    dma_addr_t          phy_addr = 0x0;
    void * desc_data = NULL;

    if(descriptor == NULL) {
        return CLX_E_NOT_INITED;
    }
    if((desc_data = kmalloc(descriptor->size,GFP_ATOMIC)) == NULL) {
        return CLX_E_NO_MEMORY;
    }
    descriptor->size = HAL_NB_PDMA_DESC_DATA_LEN/4*4;

    phy_addr = dma_map_single(&pdev->dev, desc_data, descriptor->size, DMA_FROM_DEVICE);
    if (dma_mapping_error(&pdev->dev, phy_addr)) {
        kfree(desc_data);
        return CLX_E_OTHERS;
    }

    descriptor->dinc = 1;
    //descriptor->sinc = 1;
    descriptor->d_addr = phy_addr;


    return ret;
}

static CLX_ERROR_NO_T hal_nb_pkt_fill_pph(const UI32_T port, HAL_NB_PP_HDR_T *ptr_pph)
{
    ptr_pph->dst_idx                                 = port;
    ptr_pph->skip_epp                                = 1;
    ptr_pph->skip_ipp                                = 1;

    /*
     * TODO:
     *      fill in other fileds.
     */
    return (CLX_E_OK);
}

static int clxdev_prepare_tx_pkt_desc(struct pci_dev* pdev,HAL_NB_PDMA_DESC_T volatile * descriptor,UI32_T port,UI32_T len) 
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    dma_addr_t          phy_addr = 0x0;
    void * desc_data = kmalloc(len + sizeof(HAL_NB_PP_HDR_T),GFP_ATOMIC);

    if(desc_data == NULL) {
        return CLX_E_NO_MEMORY;
    }

    hal_nb_pkt_fill_pph(port,(HAL_NB_PP_HDR_T*)desc_data);
    
    descriptor->size = len + sizeof(HAL_NB_PP_HDR_T);

    phy_addr = dma_map_single(&pdev->dev, desc_data, descriptor->size, DMA_TO_DEVICE);
    if (dma_mapping_error(&pdev->dev, phy_addr)) {
        kfree(desc_data);
        return CLX_E_OTHERS;
    }

    //descriptor->dinc = 1;
    descriptor->sinc = 1;
    descriptor->s_addr = phy_addr;


    return ret;
}
static int clxdev_free_desc(struct pci_dev* pdev,const dma_addr_t phy_addr)
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    void  *virt_addr;

    virt_addr = phys_to_virt(phy_addr);
    kfree(virt_addr);
    return ret;
}


static int clxdev_init_pdma_rx_desc(struct pci_dev* pdev)
{
    UI32_T channel;
    UI32_T desc_index;
    UI32_T              work_index = 0;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    HAL_NB_PDMA_DESC_T volatile * descriptor = NULL;

    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_RX_CHANNEL_LAST;channel++) {
        for(desc_index = 0; desc_index < privdata->pdma_ring_size[channel] - 1;desc_index++){
            descriptor = &privdata->pdma_ring_base_align[channel][desc_index];
            ret = clxdev_init_pdma_one_rx_desc(pdev,descriptor);
            if(ret != CLX_E_OK) {
                clx_print(ERR,"Alloc rx desc failed,rc=%d,channel=%d,desc_index=%d\n",
                          ret,channel,desc_index);
                return ret;
            }
        }
        /*work_index*/
        work_index = privdata->pdma_ring_size[channel];
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                            &work_index,sizeof(UI32_T));
        ret = clxdev_pdma_enable_channel(pdev,channel);
    }
    return ret;
}

static int clxdev_deinit_pdma_rx_desc(struct pci_dev* pdev)
{
    UI32_T channel;
    UI32_T desc_index;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    HAL_NB_PDMA_DESC_T volatile * descriptor = NULL;

    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_RX_CHANNEL_LAST;channel++) {
        ret = clxdev_pdma_disable_channel(pdev,channel);

        for(desc_index = 0; desc_index < privdata->pdma_ring_size[channel];desc_index++) {
            descriptor = &privdata->pdma_ring_base_align[channel][desc_index];
            if(descriptor->d_addr == 0x0) {/*make sure the last descriptor of the ring is empty */
                continue;
            }

            dma_unmap_single(&pdev->dev, descriptor->d_addr, descriptor->size, DMA_FROM_DEVICE);
            ret = clxdev_free_desc(pdev,descriptor->d_addr);
            if(ret != CLX_E_OK) {
                clx_print(ERR,"free rx desc failed,rc=%d,channel=%d,desc_index=%d\n",
                          ret,channel,desc_index);
                return ret;
            }
        }
    }
    return ret;
}

static int clxdev_init_pdma_tx_desc(struct pci_dev* pdev)
{
    UI32_T channel;
    UI32_T              work_index = 0;
    CLX_ERROR_NO_T      ret = CLX_E_OK;

    for(channel = HAL_NB_PDMA_TX_CHANNEL_0;channel < HAL_NB_PDMA_TX_CHANNEL_LAST;channel++) {
        /*work_index*/
        work_index = 0; //when pop_idx != work_idx,the PDMA machine works immediately.
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                            &work_index,sizeof(UI32_T));
        ret = clxdev_pdma_enable_channel(pdev,channel); 
    }
    return ret;
}

static int clxdev_deinit_pdma_tx_desc(struct pci_dev* pdev)
{
    UI32_T channel;
    CLX_ERROR_NO_T      ret = CLX_E_OK;

    for(channel = HAL_NB_PDMA_TX_CHANNEL_0;channel < HAL_NB_PDMA_TX_CHANNEL_LAST;channel++) {
        ret = clxdev_pdma_disable_channel(pdev,channel); 
    }
    return ret;
}

/*pkt RX/TX FIFO DMA channel*/
static CLX_ERROR_NO_T clxdev_init_pkt_pdma_ring(struct pci_dev* pdev)
{
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    CLX_ADDR_T          phy_addr = 0x0;
    UI32_T              byte_swap = 0;
    UI32_T              channel = 0;
    UI32_T              config_ring_size = 0;

    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_PKT_CHANNEL_NUM;channel++) {
        privdata->pdma_ring_size[channel] = HAL_DFLT_CFG_PKT_TX_GPD_NUM;
        privdata->pdma_ring_base[channel] = (HAL_NB_PDMA_DESC_T *)osal_dma_alloc(&pdev->dev,
                                  (privdata->pdma_ring_size[channel] + 1) * sizeof(HAL_NB_PDMA_DESC_T));
        if(privdata->pdma_ring_base[channel] == NULL) {
            ret = CLX_E_NO_MEMORY;
            return ret;
        }

        memset(privdata->pdma_ring_base[channel], 0,
                    (privdata->pdma_ring_size[channel]) * sizeof(HAL_NB_PDMA_DESC_T));
        privdata->pdma_ring_base_align[channel] = (HAL_NB_PDMA_DESC_T *)HAL_NB_PDMA_RING_BASE_ALIGN_ADDR(
                                            (CLX_HUGE_T)privdata->pdma_ring_base[channel],sizeof(HAL_NB_PDMA_DESC_T));

        /*ring base*/
        phy_addr = virt_to_phys(privdata->pdma_ring_base[channel]);
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_BASE_REG(channel)),
                            &phy_addr,sizeof(CLX_ADDR_T));
        /*ring size*/
        config_ring_size = privdata->pdma_ring_size[channel];
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_SIZE_REG(channel)),
                            &config_ring_size,sizeof(UI32_T));

        /*channel mode*/
        //  no need to config tx/rx channel mode

        /*byte swap*/
        byte_swap = HAL_NB_PDMA_BYTE_SWAP_DISABLE;
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_BYTE_ENDIAN_REG(channel)),
                            &byte_swap,sizeof(UI32_T));
        /*enable channel*/
        //clxdev_pdma_enable_channel(pdev,channel);
    }

    return ret ;
}
static CLX_ERROR_NO_T clxdev_deinit_pdma_ring(struct pci_dev* pdev)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    UI32_T              channel = 0;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_PKT_CHANNEL_NUM;channel++) {
        osal_dma_free(&pdev->dev,privdata->pdma_ring_base[channel]);
    }
    return rc;
}

static CLX_ERROR_NO_T _hal_nb_pdma_enQueue(HAL_NB_PDMA_SW_QUEUE_T  *ptr_que, void *ptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_enque(&ptr_que->que_id, ptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static CLX_ERROR_NO_T _hal_nb_pdma_deQueue(HAL_NB_PDMA_SW_QUEUE_T  *ptr_que,void **pptr_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    rc = osal_que_deque(&ptr_que->que_id, pptr_data);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static CLX_ERROR_NO_T _hal_nb_pdma_get_queue_count(HAL_NB_PDMA_SW_QUEUE_T  *ptr_que,UI32_T *ptr_count)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;

    osal_takeSemaphore(&ptr_que->sema, CLX_SEMAPHORE_WAIT_FOREVER);
    osal_que_getCount(&ptr_que->que_id, ptr_count);
    osal_giveSemaphore(&ptr_que->sema);

    return (rc);
}

static int clx_dev_init_pdma_queue(struct pci_dev* pdev)
{
    UI32_T queue = 0;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);

    for(queue = HAL_NB_PDMA_RX_CHANNEL_0; queue < HAL_NB_PDMA_PKT_CHANNEL_NUM; queue++) {
        privdata->sw_queue[queue].len    = HAL_DFLT_CFG_PKT_RX_QUEUE_LEN;
        privdata->sw_queue[queue].weight = HAL_DFLT_CFG_PKT_RX_QUEUE_WEIGHT;

        osal_createSemaphore("RX_QUE", CLX_SEMAPHORE_BINARY, &privdata->sw_queue[queue].sema);
        ret = osal_que_create(&privdata->sw_queue[queue].que_id, privdata->sw_queue[queue].len);
    }

    return ret;

}

static int clx_dev_deinit_pdma_queue(struct pci_dev* pdev)
{
    UI32_T queue = 0;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);

    for(queue = HAL_NB_PDMA_RX_CHANNEL_0; queue < HAL_NB_PDMA_PKT_CHANNEL_NUM; queue++) {
        osal_destroySemaphore(&privdata->sw_queue[queue].sema);
        osal_que_destroy(&privdata->sw_queue[queue].que_id);
    }

    return ret;
}

static int clxdev_init_pdma(struct pci_dev* pdev) 
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;

    ret = clxdev_init_pkt_pdma_ring(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init PDMA ring failed,rc=%d\n",ret);
        return ret;
    }

    ret = clxdev_init_pdma_rx_desc(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init PDMA ring rx descriptor failed,rc=%d\n",ret);
        goto err_deinit_ring; 
    }
    ret = clxdev_init_pdma_tx_desc(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init PDMA ring tx descriptor failed,rc=%d\n",ret);
        goto err_deinit_rx_desc;
    }

    ret = clx_dev_init_pdma_queue(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init PDMA queue failed,rc=%d\n",ret);
        goto err_deinit_tx_desc;
    }
    return CLX_E_OK;
    
err_deinit_tx_desc:
    clxdev_deinit_pdma_tx_desc(pdev);
err_deinit_rx_desc:
    clxdev_deinit_pdma_rx_desc(pdev);
err_deinit_ring:
    clxdev_deinit_pdma_ring(pdev);

    return ret;
}

static int clxdev_deinit_pdma(struct pci_dev* pdev) 
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;

    if(clx_dev_deinit_pdma_queue(pdev)) {
        clx_print(INFO,"clx_dev_deinit_pdma_queue success\n");
    }
    if (clxdev_deinit_pdma_tx_desc(pdev)) {
        clx_print(INFO,"clxdev_deinit_pdma_tx_desc success\n");
    }
    if (clxdev_deinit_pdma_rx_desc(pdev)) {
        clx_print(INFO,"clxdev_deinit_pdma_rx_desc success\n");
    }
    if (clxdev_deinit_pdma_ring(pdev)) {
        clx_print(INFO,"clxdev_deinit_pdma_ring success\n");
    }
    return ret;
}


static CLX_ERROR_NO_T clxdev_print_pkt_buf(const UI8_T *ptr_data, const UI32_T data_len)
{
/* e.g.
 *  0: ff ff ff ff ff ff 00 00 00 00 00 01 08 06 00 00
 * 16: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 32: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 48: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
 * 64: 09 00 44 d7
 */
    UI32_T idx = 0;

    for (idx = 0; idx < data_len; idx++) {
        /* Prefix */
        if (0 == idx % 16)  {
            clx_print(INFO,"\n");
            clx_print(INFO,"%3d: ", idx);
        }

        /* Data */
        clx_print(INFO,"%02x ", ptr_data[idx]);
    }

    /* Last data */
    clx_print(INFO,"\n");

    return (CLX_E_OK);
}


static int enable_rxdequeue = 0; //for debug
static int enable_rxenqueue = 0; //for debug
static void pdma_rx_dequeue_task(void *task_cookie)
{
    CLX_ERROR_NO_T  ret = CLX_E_OK;
    struct pci_dev  *pdev = ((HAL_NB_PDMA_TASK_COOKIE *)task_cookie)->pdev;
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    UI32_T idx = 0;
    UI32_T queue   = 0;
    UI32_T que_cnt = 0;
    HAL_NB_PDMA_DESC_T  *descriptor;
    void  *virt_addr;
    UI8_T   ptr_cookie[65536];
    UI32_T  total_len = 0;
    unsigned long   timeout  = 0;
    
    do {
        if(enable_rxdequeue == 0) {
            usleep_range(100000, 150000);
            continue;
        }
        for (idx = 0; idx < HAL_NB_PDMA_RX_CHANNEL_LAST; idx++)
        {
            /* to gurantee the opportunity where each queue can be handler */
            queue = ((privdata->deque_idx + idx) % HAL_NB_PDMA_RX_CHANNEL_LAST);
            _hal_nb_pdma_get_queue_count(&privdata->sw_queue[queue], &que_cnt);
            if (que_cnt > 0)
            {
                privdata->deque_idx = ((queue + 1) % HAL_NB_PDMA_RX_CHANNEL_LAST);
                break;
            }
        }

        if (0 == que_cnt) {
            continue;
        }
        
        total_len = 0;
        for(idx  = 0; idx < que_cnt;idx++) {
            ret = _hal_nb_pdma_deQueue(&privdata->sw_queue[queue], (void **)&descriptor);

            if(!descriptor->eop) {
                
                dma_unmap_single(&pdev->dev, descriptor->d_addr, descriptor->size, DMA_FROM_DEVICE);
                virt_addr = phys_to_virt(descriptor->d_addr);
                memcpy(&ptr_cookie[total_len],virt_addr,descriptor->size);

                clxdev_free_desc(pdev,descriptor->d_addr);
            }
            total_len += descriptor->size;
        }
        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously free tx descriptor for 1 tick */
        }

        clxdev_print_pkt_buf(ptr_cookie,total_len);

    } while(1);
}


static void pdma_rx_poll(void *cookie)
{
    UI32_T          channel = ((HAL_NB_PDMA_TASK_COOKIE *)cookie)->channel;
    struct pci_dev  *pdev = ((HAL_NB_PDMA_TASK_COOKIE *)cookie)->pdev;
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    UI32_T          last_pop_index = 0,pop_index = 0,work_index = 0;
    UI32_T          used_desc = 0;
    UI32_T          current_index = 0;
    UI32_T          i = 0;
    unsigned long   timeout  = 0;
    volatile HAL_NB_PDMA_DESC_T * descriptor = NULL;


    do {
        if(enable_rxenqueue == 0) { //set 1 to start this task
            usleep_range(100000, 150000);
            continue;
        }
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                            &pop_index,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                            &work_index,sizeof(UI32_T));

        /*
         * TODO:
         *      Error handler.
         */
        if(last_pop_index == pop_index) { /*no pkts receieved*/
            continue;
        } 
        else if(last_pop_index > pop_index) { /*wrap*/
            used_desc = pop_index - last_pop_index + (privdata->pdma_ring_size[channel] - 1);
        }
        else {
            used_desc = pop_index - last_pop_index;
        }

        for(i = 0;i < used_desc;i++) {
            current_index = (last_pop_index + i) % (privdata->pdma_ring_size[channel]);
            descriptor = &privdata->pdma_ring_base_align[channel][current_index];

            _hal_nb_pdma_enQueue(&privdata->sw_queue[channel], (void*)descriptor);

            clxdev_init_pdma_one_rx_desc(pdev,descriptor);
        } 

        work_index += used_desc;
        work_index %= privdata->pdma_ring_size[channel]; //descriptor pointed by the work_index is always null 
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                    &work_index,sizeof(UI32_T));

        last_pop_index = pop_index;


        /* prevent this task from executing too long */
        if (!(time_before(jiffies, timeout)))
        {
            schedule();
            timeout = jiffies + 1; /* continuously free tx descriptor for 1 tick */
        }
    } while(1);
}

static CLX_ERROR_NO_T clxdev_init_task(struct pci_dev* pdev) 
{
    CLX_ERROR_NO_T ret = CLX_E_OK;
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    UI32_T          channel = 0;
    HAL_NB_PDMA_TASK_COOKIE task_cookie;
    

    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_RX_CHANNEL_LAST;channel++) {
        task_cookie.channel = channel;
        task_cookie.pdev = pdev;
        ret = osal_createThread("pdmad", HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_STACK,
                        HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_PRI, pdma_rx_poll,
                        (void *)(&task_cookie), privdata->pdma_task[channel]);
        if(ret != CLX_E_OK) {
            clx_print(ERR,"create kthread failed,rc=%d,cchannel=%d\n",ret,channel);
            return CLX_E_OTHERS;
        }
    }
    
    task_cookie.pdev = pdev;
    ret = osal_createThread("pdma_rx_dequeue", HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_STACK,
                    HAL_DFLT_CFG_PKT_ERROR_ISR_THREAD_PRI, pdma_rx_dequeue_task,
                    (void *)(&task_cookie), privdata->pdma_dequeue_task);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"create kthread failed,rc=%d,cchannel=%d\n",ret,channel);
        return CLX_E_OTHERS;
    }
    return ret;
}
static int clxdev_deinit_task(struct pci_dev* pdev) 
{
    CLX_ERROR_NO_T ret = CLX_E_OK;
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    UI32_T          channel = 0;
    
    for(channel = HAL_NB_PDMA_RX_CHANNEL_0;channel < HAL_NB_PDMA_RX_CHANNEL_LAST;channel++) {
        osal_stopThread(privdata->pdma_task[channel]);
        osal_destroyThread(privdata->pdma_task[channel]);
    }
    osal_stopThread(privdata->pdma_dequeue_task);
    osal_destroyThread(privdata->pdma_dequeue_task);
    return ret;
}


/*sysfs test start*/
struct pdma_device_attribute{
        struct device_attribute dev_attr;
        int index;
};
#define to_pdma_dev_attr(_dev_attr) \
        container_of(_dev_attr, struct pdma_device_attribute, dev_attr)

#define PDMA_ATTR(_name, _mode, _show, _store, _index)        \
        { .dev_attr = __ATTR(_name, _mode, _show, _store),      \
          .index = _index }
          
#define PDMA_DEVICE_ATTR(_name, _mode, _show, _store, _index) \
struct pdma_device_attribute pdma_dev_attr_##_name          \
        = PDMA_ATTR(_name, _mode, _show, _store, _index)

#if 0
static HAL_NB_PDMA_CH_MODE clx_get_channelmode_by_channel(UI32_T channel)
{
    HAL_NB_PDMA_CH_MODE channel_mode = 0;
    switch (channel) {

        case HAL_NB_PDMA_GENERAL_CHANNEL_0:
            channel_mode = NB_PCX_DMA_HOSTMEM_TO_LOCALBUS;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_1:
            channel_mode = NB_PCX_DMA_LOCALBUS_TO_HOSTMEM;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_2:
            channel_mode = NB_PCX_DMA_LOCALBUS_TO_LOCALBUS;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_3:
            channel_mode = NB_PCX_DMA_HOSTMEM_TO_ECPU;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_4:
            channel_mode = NB_PCX_DMA_ECPU_TO_HOSTMEM;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_5:
            channel_mode = NB_PCX_DMA_ECPU_TO_ECPU;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_6:
            channel_mode = NB_PCX_DMA_LOCALBUS_TO_ECPU;
            break;
        case HAL_NB_PDMA_GENERAL_CHANNEL_7:
            channel_mode = NB_PCX_DMA_ECPU_TO_LOCALBUS;
            break;
        default:
            clx_print(WARNING,"get channel mode failed,channel:%d\n",channel);
            break;
    }
    clx_print(DEBUG,"channel:%d,channel_mode:%d\n",channel,channel_mode);
    return channel_mode;

}
#endif

/*general DMA channel*/
static ssize_t clxdev_init_general_pdma_ring(struct pci_dev* pdev,UI32_T channel)
{
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    CLX_ADDR_T          phy_addr = 0x0;
    UI32_T              byte_swap = 0;
    UI32_T              config_ring_size = 0;
    UI32_T              channel_mode = 0;

        clx_print(DEBUG,"channel:%d,ring size:%d,channel_mode:%d\n",
                  channel,privdata->pdma_ring_size[channel],privdata->channel_attr.channel_mode[channel]);
        //privdata->pdma_ring_size[channel] = HAL_NB_PDMA_GENERAL_RING_SIZE;
        if(privdata->pdma_ring_size[channel] < 2) {
            clx_print(ERR,"channel:%d,ring size:%d,please set correct ring size first\n",
                      channel,privdata->pdma_ring_size[channel]);
            return -EINVAL;
        }
        privdata->pdma_ring_base[channel] = (HAL_NB_PDMA_DESC_T *)osal_dma_alloc(&pdev->dev,
                                  (privdata->pdma_ring_size[channel] + 1) * sizeof(HAL_NB_PDMA_DESC_T));
        if(privdata->pdma_ring_base[channel] == NULL) {
            ret = CLX_E_NO_MEMORY;
            return ret;
        }

        memset(privdata->pdma_ring_base[channel], 0,
                    (privdata->pdma_ring_size[channel]) * sizeof(HAL_NB_PDMA_DESC_T));
        privdata->pdma_ring_base_align[channel] = (HAL_NB_PDMA_DESC_T *)HAL_NB_PDMA_RING_BASE_ALIGN_ADDR(
                                            (CLX_HUGE_T)privdata->pdma_ring_base[channel],sizeof(HAL_NB_PDMA_DESC_T));

        /*ring base*/
        phy_addr = virt_to_phys(privdata->pdma_ring_base[channel]);
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_BASE_REG(channel)),
                            &phy_addr,sizeof(CLX_ADDR_T));
        /*ring size*/
        config_ring_size = privdata->pdma_ring_size[channel];
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RING_SIZE_REG(channel)),
                            &config_ring_size,sizeof(UI32_T));

        /*channel mode*/
        channel_mode = privdata->channel_attr.channel_mode[channel];
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_MODE_REG(channel)),
                            &channel_mode,sizeof(UI32_T));

        /*byte swap*/
        byte_swap = HAL_NB_PDMA_BYTE_SWAP_DISABLE;
        clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_BYTE_ENDIAN_REG(channel)),
                            &byte_swap,sizeof(UI32_T));
        /*enable channel*/
        clxdev_pdma_enable_channel(pdev,channel);

        privdata->channel_attr.channel_inited[channel] = 1;

    return ret ;
}

static CLX_ERROR_NO_T clxdev_deinit_general_pdma_ring(struct pci_dev* pdev,UI32_T channel)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    struct clxdev_data  *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);

    clxdev_pdma_disable_channel(pdev,channel);
    osal_dma_free(&pdev->dev,privdata->pdma_ring_base[channel]);
    return rc;
}

static CLX_ERROR_NO_T clxdev_alloc_general_pdma_desc(
           struct pci_dev               *pdev,
           HAL_NB_PDMA_DESC_T volatile  *descriptor,
           CLX_ADDR_T                   src_phy_addr,
           CLX_ADDR_T                   dst_phy_addr,
           UI16_T                       len,
           HAL_NB_PDMA_CH_MODE          channel_mode) 
{
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    dma_addr_t          hostmem_phy_addr = 0x0;
    UI32_T              to_hostmem_size = (UI16_T)(HAL_NB_PDMA_DESC_DATA_MAX_LEN-1) + 1;
    void * desc_data = NULL;

    descriptor->size = len/4*4;
    descriptor->dinc = 1;
    descriptor->sinc = 1;
    
    switch (channel_mode) {
        case NB_PCX_DMA_HOSTMEM_TO_LOCALBUS:
        case NB_PCX_DMA_HOSTMEM_TO_ECPU:
            desc_data = osal_alloc(descriptor->size);
            if(desc_data == NULL) {
                return CLX_E_NO_MEMORY;
            }
            hostmem_phy_addr = dma_map_single(&pdev->dev, desc_data, descriptor->size, DMA_TO_DEVICE);
            if (dma_mapping_error(&pdev->dev, hostmem_phy_addr)) {
                osal_free(desc_data);
                clx_print(ERR,"Dma map failed\n");
                return CLX_E_OTHERS;
            }
            descriptor->s_addr = hostmem_phy_addr;
            descriptor->d_addr = dst_phy_addr;
            break;
        case NB_PCX_DMA_LOCALBUS_TO_HOSTMEM:
        case NB_PCX_DMA_ECPU_TO_HOSTMEM:
            desc_data = osal_alloc(to_hostmem_size);
            if(desc_data == NULL) {
                return CLX_E_NO_MEMORY;
            }
            hostmem_phy_addr = dma_map_single(&pdev->dev, desc_data, to_hostmem_size, DMA_FROM_DEVICE);
            if (dma_mapping_error(&pdev->dev, hostmem_phy_addr)) {
                osal_free(desc_data);
                clx_print(ERR,"Dma map failed\n");
                return CLX_E_OTHERS;
            }
            descriptor->s_addr = src_phy_addr;
            descriptor->d_addr = hostmem_phy_addr;
            break;
        case NB_PCX_DMA_LOCALBUS_TO_LOCALBUS:
        case NB_PCX_DMA_ECPU_TO_ECPU:
        case NB_PCX_DMA_LOCALBUS_TO_ECPU:
        case NB_PCX_DMA_ECPU_TO_LOCALBUS:
            descriptor->s_addr = src_phy_addr;
            descriptor->d_addr = dst_phy_addr;
            break;
        
        default:
            clx_print(WARNING,"wrong channel mode:%d\n",channel_mode);
            break;
    }

    return ret;
}

static ssize_t set_general_channel_ring_size(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    long val = 0;

    if (kstrtol(buf, 16, &val)) {
        return -EINVAL;
    }
    privdata->pdma_ring_size[channel] = val;
    return count;
}
static ssize_t get_general_channel_ring_size(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "%d\n", privdata->pdma_ring_size[channel]);
    return count;
}

static ssize_t set_general_channel_mode(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    long val = 0;

    if (kstrtol(buf, 16, &val)) {
        return -EINVAL;
    }
    privdata->channel_attr.channel_mode[channel] = val;
    return count;
}
static ssize_t get_general_channel_mode(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "%d\n",privdata->channel_attr.channel_mode[channel]);
    return count;
}

static ssize_t set_general_channel_src_addr(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    CLX_ADDR_T val = 0;

    if (kstrtou64(buf, 16, &val)) {
        return -EINVAL;
    }
    privdata->channel_attr.src_phy_addr[channel] = val;
    return count;
}
static ssize_t get_general_channel_src_addr(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "0x%llx\n", privdata->channel_attr.src_phy_addr[channel]);
    return count;
}

static ssize_t set_general_channel_dst_addr(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    CLX_ADDR_T val = 0;

    if (kstrtou64(buf, 16, &val)) {
        return -EINVAL;
    }
    privdata->channel_attr.dst_phy_addr[channel] = val;
    return count;
}
static ssize_t get_general_channel_dst_addr(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "0x%llx\n", privdata->channel_attr.dst_phy_addr[channel]);
    return count;
}
static ssize_t set_general_channel_data_len(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    CLX_ADDR_T val = 0;

    if (kstrtou64(buf, 16, &val)) {
        return -EINVAL;
    }
    privdata->channel_attr.data_len[channel] = val;
    return count;
}
static ssize_t get_general_channel_data_len(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "0x%x\n", privdata->channel_attr.data_len[channel]);
    return count;
}

static ssize_t init_general_channel(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    long val = 0;

    if (kstrtol(buf, 16, &val)) {
        return -EINVAL;
    }
    if (val == 1) {
        clxdev_init_general_pdma_ring(pdev,channel);
    }
    else if(val == 0) {
        if(privdata->channel_attr.channel_inited[channel] != 0)
            clxdev_deinit_general_pdma_ring(pdev,channel);
    }
   privdata->channel_attr.channel_inited[channel] = val;
    return count;
}
static ssize_t get_general_inited(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;

    count = sprintf(buf, "%d\n", privdata->channel_attr.channel_inited[channel]);
    return count;
}


static ssize_t clxdev_test_general_pdma(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T              channel = attr->index;
    UI32_T pop_index = 0,work_index = 0; 
    UI32_T              loop_cnt = 0;
    HAL_NB_PDMA_DESC_T volatile * descriptor = NULL;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    void                *virt_addr;
    UI32_T              len = privdata->channel_attr.data_len[channel];
    HAL_NB_PDMA_CH_MODE channel_mode = privdata->channel_attr.channel_mode[channel];;


    clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                        &work_index,sizeof(UI32_T));


    if((privdata->channel_attr.src_phy_addr[channel]) == 0x0 && 
        (privdata->channel_attr.dst_phy_addr[channel] == 0x0)) {
        clx_print(ERR,"set descriptor src addr and dst addr first\n");
        return -ENAVAIL;
    }

    descriptor = &privdata->pdma_ring_base_align[channel][work_index];
    clxdev_alloc_general_pdma_desc(pdev,descriptor,privdata->channel_attr.src_phy_addr[channel],privdata->channel_attr.dst_phy_addr[channel],len,channel_mode);
    /*log phy addr for test*/
    privdata->channel_attr.src_phy_addr[channel] = descriptor->s_addr;
    privdata->channel_attr.dst_phy_addr[channel] = descriptor->d_addr;
            
    work_index += 1;
    work_index %= privdata->pdma_ring_size[channel]; 
    clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    /*
     * TODO:
     *      check the src buffer and dest buffer.
     */
    
    switch (channel_mode) {
        case NB_PCX_DMA_HOSTMEM_TO_LOCALBUS:
        case NB_PCX_DMA_HOSTMEM_TO_ECPU:
            clxdev_free_desc(pdev,descriptor->s_addr); 
            break;
        case NB_PCX_DMA_LOCALBUS_TO_HOSTMEM:
        case NB_PCX_DMA_ECPU_TO_HOSTMEM:
            dma_unmap_single(&pdev->dev, descriptor->d_addr, descriptor->size, DMA_FROM_DEVICE);
            virt_addr = phys_to_virt(descriptor->d_addr);
            clxdev_print_pkt_buf((UI8_T*)virt_addr,descriptor->size);

            clxdev_free_desc(pdev,descriptor->d_addr);
            break;
        default:
            break;
    }
    
    return ret;
}



static ssize_t clxdev_test_pkt_pdma_tx(struct device *dev, struct device_attribute *da,
            const char *buf, size_t count)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T              channel = attr->index;

    UI32_T pop_index = 0,work_index = 0; 
    UI32_T              loop_cnt = 0;
    HAL_NB_PDMA_DESC_T volatile * descriptor = NULL;
    CLX_ERROR_NO_T      ret = CLX_E_OK;
    UI32_T              pkt_len = privdata->channel_attr.data_len[channel];
    UI32_T              dest_port = 0;


    clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                        &work_index,sizeof(UI32_T));


    descriptor = &privdata->pdma_ring_base_align[channel][work_index];
    clxdev_prepare_tx_pkt_desc(pdev,descriptor,dest_port,pkt_len);
            
    work_index += 1; //sop & eop in the same descriptor.
    work_index %= privdata->pdma_ring_size[channel]; 
    clxdev_write_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_WORK_IDX_REG(channel)),
                         &work_index,sizeof(UI32_T));

    do {
        loop_cnt ++;
        ret = CLX_E_TIMEOUT;
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                    &pop_index,sizeof(UI32_T));
        if(pop_index == work_index) {
            ret = CLX_E_OK;
            break;
        }
    } while(loop_cnt % HAL_NB_PKT_PDMA_TX_POLL_MAX_LOOP) ;

    dma_unmap_single(&pdev->dev, descriptor->d_addr, descriptor->size, DMA_TO_DEVICE);
    
    clxdev_free_desc(pdev,descriptor->s_addr);

    return ret;
}


static ssize_t clx_nb_dump_channel_info(struct device *dev, struct device_attribute *da,
             char *buf)
{
    struct pci_dev *pdev = to_pci_dev(dev);
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);
    struct pdma_device_attribute *attr = to_pdma_dev_attr(da);
    UI32_T channel = attr->index;
    ssize_t count = 0;
    
    UI32_T pop_index = 0,work_index = 0;
    UI32_T burst_en = 0;
    UI32_T wrr_weight = 0;
    UI32_T byte_endian = 0;
    UI32_T channel_state = 0;
    UI32_T channel_mode = 0;
    UI32_T int_msg = 0;
    UI32_T msg_per_desc = 0;
    UI64_T int_done_addr = 0;
    UI64_T int_err_addr = 0;
    UI32_T err_status = 0;
    UI32_T fectch_needed = 0;
    UI32_T channel_rdy = 0;
    UI32_T pending_read = 0;
    UI32_T pending_ack = 0;
    UI32_T desc_valid = 0;
    UI32_T rx_fifo_ctl_valid = 0;
    UI32_T rx_fifo_eop = 0;


        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                            &pop_index,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_POP_IDX_REG(channel)),
                            &work_index,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_BURST_EN_REG(channel)),
                            &burst_en,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_WRR_WEIGHT_REG(channel)),
                            &wrr_weight,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_BYTE_ENDIAN_REG(channel)),
                            &byte_endian,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ENABLE_REG(channel)),
                            &channel_state,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_MODE_REG(channel)),
                            &channel_mode,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_INT_MSG_REG(channel)),
                            &int_msg,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_MSG_PER_DESC_REG(channel)),
                            &msg_per_desc,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_INT_DONE_ADDR_REG(channel)),
                            &int_done_addr,sizeof(UI64_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_INT_ERROR_ADDR_REG(channel)),
                            &int_err_addr,sizeof(UI64_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_ERROR_STATUS_REG(channel)),
                            &err_status,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_FETCH_NEEDED_REG(channel)),
                            &fectch_needed,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_CHANNEL_RDY_REG(channel)),
                            &channel_rdy,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_PENDING_READS_REG(channel)),
                            &pending_read,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_PENDING_ACK_REG(channel)),
                            &pending_ack,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_DESC_VALID_REG(channel)),
                            &desc_valid,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RXFIFO_CTL_VALID_REG(channel)),
                            &rx_fifo_ctl_valid,sizeof(UI32_T));
        clxdev_read_pci_reg(pdev,HAL_NB_PDMA_GET_MMIO(HAL_NB_GET_PDMA_CH_RXFIFO_EOP_REG(channel)),
                            &rx_fifo_eop,sizeof(UI32_T));

        count = sprintf(buf, "channel:%d\n",channel);
        count += sprintf(buf, "ring base:%llx\n",virt_to_phys(privdata->pdma_ring_base_align[channel]));
        count += sprintf(buf, "ring size:%x\n",privdata->pdma_ring_size[channel]);
        count += sprintf(buf, "desc work_index:%d\n",work_index);
        count += sprintf(buf, "desc pop_index:%d\n",pop_index);
        count += sprintf(buf, "desc burst en:%d\n",burst_en);
        count += sprintf(buf, "wrr_weight:%d\n",wrr_weight);
        count += sprintf(buf, "swap byte:%d\n",byte_endian);
        count += sprintf(buf, "enable state:%d\n",channel_state);
        count += sprintf(buf, "channel mode:%d\n",channel_mode);
        count += sprintf(buf, "int msg:%d\n",int_msg);
        count += sprintf(buf, "int msg:%d\n",msg_per_desc);
        count += sprintf(buf, "int done addr:0x%llx\n",int_done_addr);
        count += sprintf(buf, "int err addr:0x%llx\n",int_err_addr);
        count += sprintf(buf, "err status:%d\n",err_status);
        count += sprintf(buf, "fetch needed:%d\n",fectch_needed);
        count += sprintf(buf, "channel rdy:%d\n",channel_rdy);
        count += sprintf(buf, "pending read:%d\n",pending_read);
        count += sprintf(buf, "pending ack:%d\n",pending_ack);
        count += sprintf(buf, "desc valid:%d\n",desc_valid);
        count += sprintf(buf, "rx fifo ctl valid:%d\n",rx_fifo_ctl_valid);
        count += sprintf(buf, "rx fifo eop:%d\n",rx_fifo_eop);
        count += sprintf(buf, "\n");
        return count;
}

static ssize_t clx_nb_pdma_help(struct device *dev, struct device_attribute *da,
             char *buf)
{
    ssize_t count = 0;
    
    count = sprintf(buf, "RX FIFO:There are 4 tasks for RX pkts from ASIC\n");
    count += sprintf(buf, "TX FIFO:echo 1 > */tx_pdma_channel4\n");
    count += sprintf(buf, "General DMA channel:\n");
    count += sprintf(buf, "    \t echo 8 > */general_channel8_ring_size\n");
    count += sprintf(buf, "    \t echo 1 > */init_general_channel8 (init PDMA channel)\n");
    count += sprintf(buf, "    \t echo 0x12345678 > */channel8_desc_src_addr\n");
    count += sprintf(buf, "    \t echo 0x87654321 > */channel8_desc_dst_addr\n");
    count += sprintf(buf, "    \t echo 1 > */test_general_pdma_channel8\n");
    count += sprintf(buf, "    \t echo 0 > */init_general_channel8 (deinit PDMA channel)\n\n");
    count += sprintf(buf, "cat */dump_channel4_info");
    return count;
}

static PDMA_DEVICE_ATTR(help,  S_IRUGO, clx_nb_pdma_help, NULL, 0);
#define SET_GENERAL_DMA_ATTR(_num)\
        static PDMA_DEVICE_ATTR(general_channel##_num##_ring_size,  S_IRUGO | S_IWUSR, get_general_channel_ring_size, set_general_channel_ring_size, _num);\
        static PDMA_DEVICE_ATTR(general_channel##_num##_mode,  S_IRUGO | S_IWUSR, get_general_channel_mode, set_general_channel_mode, _num);\
        static PDMA_DEVICE_ATTR(init_general_channel##_num,  S_IRUGO | S_IWUSR, get_general_inited, init_general_channel, _num);\
        static PDMA_DEVICE_ATTR(channel##_num##_desc_src_addr,  S_IRUGO | S_IWUSR, get_general_channel_src_addr, set_general_channel_src_addr, _num);\
        static PDMA_DEVICE_ATTR(channel##_num##_desc_dst_addr,  S_IRUGO | S_IWUSR, get_general_channel_dst_addr, set_general_channel_dst_addr, _num);\
        static PDMA_DEVICE_ATTR(channel##_num##_desc_data_len,  S_IRUGO | S_IWUSR, get_general_channel_data_len, set_general_channel_data_len, _num);\
        static PDMA_DEVICE_ATTR(test_general_pdma_channel##_num,  S_IWUSR, NULL, clxdev_test_general_pdma, _num);\
        static PDMA_DEVICE_ATTR(dump_channel##_num##_info,  S_IRUGO, clx_nb_dump_channel_info, NULL, _num);
        
#define SET_TX_PKT_DMA_ATTR(_num)\
    static PDMA_DEVICE_ATTR(channel##_num##_desc_data_len,  S_IRUGO | S_IWUSR, get_general_channel_data_len, set_general_channel_data_len, _num);\
    static PDMA_DEVICE_ATTR(tx_pdma_channel##_num,  S_IWUSR, NULL, clxdev_test_pkt_pdma_tx, _num);\
    static PDMA_DEVICE_ATTR(dump_channel##_num##_info,  S_IRUGO, clx_nb_dump_channel_info, NULL, _num);

SET_TX_PKT_DMA_ATTR(4);
SET_TX_PKT_DMA_ATTR(5);
SET_TX_PKT_DMA_ATTR(6);
SET_TX_PKT_DMA_ATTR(7);
SET_GENERAL_DMA_ATTR(8);
SET_GENERAL_DMA_ATTR(9);
SET_GENERAL_DMA_ATTR(10);
SET_GENERAL_DMA_ATTR(11);
SET_GENERAL_DMA_ATTR(12);
SET_GENERAL_DMA_ATTR(13);
SET_GENERAL_DMA_ATTR(14);
SET_GENERAL_DMA_ATTR(15);


static struct attribute *clx_pdma_attributes[] = {
    &pdma_dev_attr_help.dev_attr.attr,

    &pdma_dev_attr_general_channel8_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel9_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel10_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel11_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel12_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel13_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel14_mode.dev_attr.attr,
    &pdma_dev_attr_general_channel15_mode.dev_attr.attr,

    &pdma_dev_attr_general_channel8_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel9_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel10_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel11_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel12_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel13_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel14_ring_size.dev_attr.attr,
    &pdma_dev_attr_general_channel15_ring_size.dev_attr.attr,

    &pdma_dev_attr_init_general_channel8.dev_attr.attr,
    &pdma_dev_attr_init_general_channel9.dev_attr.attr,
    &pdma_dev_attr_init_general_channel10.dev_attr.attr,
    &pdma_dev_attr_init_general_channel11.dev_attr.attr,
    &pdma_dev_attr_init_general_channel12.dev_attr.attr,
    &pdma_dev_attr_init_general_channel13.dev_attr.attr,
    &pdma_dev_attr_init_general_channel14.dev_attr.attr,
    &pdma_dev_attr_init_general_channel15.dev_attr.attr,
    
    &pdma_dev_attr_channel8_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel9_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel10_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel11_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel12_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel13_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel14_desc_src_addr.dev_attr.attr,
    &pdma_dev_attr_channel15_desc_src_addr.dev_attr.attr,

    &pdma_dev_attr_channel8_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel9_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel10_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel11_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel12_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel13_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel14_desc_dst_addr.dev_attr.attr,
    &pdma_dev_attr_channel15_desc_dst_addr.dev_attr.attr,

    &pdma_dev_attr_channel4_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel5_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel6_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel7_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel8_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel9_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel10_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel11_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel12_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel13_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel14_desc_data_len.dev_attr.attr,
    &pdma_dev_attr_channel15_desc_data_len.dev_attr.attr,

    &pdma_dev_attr_test_general_pdma_channel8.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel9.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel10.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel11.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel12.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel13.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel14.dev_attr.attr,
    &pdma_dev_attr_test_general_pdma_channel15.dev_attr.attr,

    &pdma_dev_attr_tx_pdma_channel4.dev_attr.attr,
    &pdma_dev_attr_tx_pdma_channel5.dev_attr.attr,
    &pdma_dev_attr_tx_pdma_channel6.dev_attr.attr,
    &pdma_dev_attr_tx_pdma_channel7.dev_attr.attr,

    &pdma_dev_attr_dump_channel4_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel5_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel6_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel7_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel8_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel9_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel10_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel11_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel12_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel13_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel14_info.dev_attr.attr,
    &pdma_dev_attr_dump_channel15_info.dev_attr.attr,

    NULL
};

static const struct attribute_group clx_pdma_group = { .attrs = clx_pdma_attributes};
/*sysfs test end*/



static int nb_probe(struct pci_dev* pdev, const struct pci_device_id* ent)
{
    struct clxdev_data *privdata;
    CLX_ERROR_NO_T ret = CLX_E_OK;

    privdata = kzalloc(sizeof(struct clxdev_data), GFP_KERNEL);
    if (!privdata)
        return -ENOMEM;
    privdata->pdev = pdev;
    
    ret = pci_enable_device(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Enable pci device failed\n!");
        goto err_free;
    }
    pci_read_config_word(pdev, PCI_DEVICE_ID, &privdata->id.device_id);
    pci_read_config_word(pdev, PCI_VENDOR_ID, &privdata->id.vendor_id);
    pci_read_config_byte(pdev, PCI_REVISION_ID, &privdata->id.revision_id);
    clx_print(DEBUG,"PCIE device_id:0x%x,vendor_id:0x%x,revision_id:0x%x",
              privdata->id.device_id,privdata->id.vendor_id,privdata->id.revision_id);
   
    ret = clxdev_get_pci_mmio_info(pdev,(UI32_T**)&privdata->mmio);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Get pci mmio faild\n");
        goto err_exit;
    }

    pci_set_drvdata(pdev, privdata);
    if (dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(48))) {
        clx_print(ERR,"dma_set_mask_and_coherent failed\n");
        goto err_unmap; 
    }

    ret = clxdev_init_pdma(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init PDMA failed\n");
        goto err_unmap;
    }
    ret = clxdev_init_task(pdev);
    if(ret != CLX_E_OK) {
        clx_print(ERR,"Init clx tasks failed\n");
        clxdev_deinit_pdma(pdev);
        goto err_unmap;
    }
    ret = sysfs_create_group(&pdev->dev.kobj, &clx_pdma_group);
    if (ret) {
        goto err_unmap;
    }

    return ret;

err_unmap:
    iounmap(privdata->mmio);
err_exit:
    pci_disable_device(pdev);
err_free:
    kfree(privdata);
    return ret;
}

static void nb_remove(struct pci_dev* pdev)
{
    struct clxdev_data *privdata = (struct clxdev_data *)pci_get_drvdata(pdev);

    sysfs_remove_group(&pdev->dev.kobj, &clx_pdma_group);
    if(clxdev_deinit_task(pdev)) {
        clx_print(INFO,"clxdev_deinit_task success\n");
    }
    if(clxdev_deinit_pdma(pdev)) {
        clx_print(INFO,"clxdev_deinit_pdma success\n");
    }

    iounmap(privdata->mmio);
    pci_release_region(pdev, 0x0);
    pci_disable_device(pdev);
    kfree(privdata);
}

static const struct pci_device_id nb_pci_tbl[] = {
    {PCI_DEVICE(HAL_CLX_VENDOR_ID, PCI_ANY_ID), clx_nb },
    {PCI_DEVICE(HAL_CL_VENDOR_ID, PCI_ANY_ID), clx_nb },
	{0, }
};

static struct pci_driver nb_driver = {
    .name = nb_driver_name,
    .id_table = nb_pci_tbl,
    .probe = nb_probe,
    .remove = nb_remove
};

static int __init nb_driver_init_module(void)
{
    int ret;

    ret = pci_register_driver(&nb_driver);
    if (ret) {
        return ret;
    }

    return 0;
}

module_init(nb_driver_init_module);

static void __exit nb_exit_module(void)
{
    pci_unregister_driver(&nb_driver);
}
module_exit(nb_exit_module);

module_param(loglevel, uint, 0664);
module_param(enable_rxdequeue, uint, 0664);
module_param(enable_rxenqueue, uint, 0664);
MODULE_LICENSE("GPL v2");