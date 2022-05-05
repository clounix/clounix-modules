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

/* FILE NAME:  cmlib_queue.c
 * PURPOSE:
 * NOTES:
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_queue.h>
#include <osal/osal.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
 */
/* GLOBAL VARIABLE DECLARATIONS
 */
//DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_queue.c");
/* LOCAL SUBPROGRAM DECLARATIONS
 */



/* STATIC VARIABLE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM BODIES
 */


/* FUNCTION NAME: cmlib_queue_create
 * PURPOSE:
 *      it is used to create a new queue.
 * INPUT:
 *      capacity  -- the queue capacity, it should be 1~N.
 *      ptr_name  -- the queue name, max length is CMLIB_NAME_MAX_LEN(include '\0')
 * OUTPUT:
 *      pptr_q  -- the new queue head
 * RETURN:
 *      CLX_E_OK            -- create success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_NO_MEMORY     -- alloc queue head failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_queue_create(
    const UI32_T       capacity,
    const C8_T         *ptr_name,
    CMLIB_QUEUE_T      **pptr_q )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CLX_ERROR_NO_T ret = CLX_E_OK;

    CMLIB_QUEUE_T *ptr_queue = NULL;

    /* BODY */
    HAL_CHECK_PTR(pptr_q);
    HAL_CHECK_PTR(ptr_name);
    if(capacity == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(CMLIB_MULTI_OVERFLOW(sizeof(CMLIB_QUEUE_ENTRY_T), capacity))
    {
        return CLX_E_BAD_PARAMETER;
    }

    ptr_queue = (CMLIB_QUEUE_T*)osal_alloc(sizeof(CMLIB_QUEUE_T));
    if(ptr_queue == NULL)
    {
        return CLX_E_NO_MEMORY;
    }

    ptr_queue->head_index = 0;
    ptr_queue->tail_index = 0;
    ptr_queue->wr_cnt = 0;
    ptr_queue->rd_cnt = 0;
    ptr_queue->capacity = capacity;

    ptr_queue->ptr_entrys = (CMLIB_QUEUE_ENTRY_T*)osal_alloc(sizeof(CMLIB_QUEUE_ENTRY_T)*capacity);
    if(ptr_queue->ptr_entrys == NULL)
    {
        osal_free(ptr_queue);
        return CLX_E_NO_MEMORY;
    }
    osal_memset((void*)ptr_queue->ptr_entrys, 0, sizeof(CMLIB_QUEUE_ENTRY_T)*capacity);
    *pptr_q = ptr_queue;
    return (ret);
}   /* End of cmlib_queue_create */

/* FUNCTION NAME: cmlib_queue_destroy
 * PURPOSE:
 *      it is used to destroy a queue and release queue head and resource,
 *      if destroy callback is not null, release entry data.
 * INPUT:
 *      ptr_q       -- the queue will be destroyed.
 *      destroy_callback -- destroy function, used to release entry data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- destroy success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_queue_destroy(
    CMLIB_QUEUE_T                    *ptr_q,
    const CMLIB_QUEUE_DESTROY_FUNC_T destroy_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CLX_ERROR_NO_T ret = CLX_E_OK;

    /* BODY */
    HAL_CHECK_PTR(ptr_q);

    if(destroy_callback != NULL)
    {
        while(ptr_q->rd_cnt != ptr_q->wr_cnt)
        {
            if(NULL != ptr_q->ptr_entrys[ptr_q->head_index].ptr_data)
            {
                destroy_callback(ptr_q->ptr_entrys[ptr_q->head_index].ptr_data);
            }

            ptr_q->head_index++;
            if((UI32_T)ptr_q->head_index >= ptr_q->capacity)
            {
                ptr_q->head_index = 0;
            }
            ptr_q->rd_cnt++;
        }
    }

    osal_free(ptr_q->ptr_entrys);
    osal_free(ptr_q);

    return (ret);
}   /* End of cmlib_queue_destroy */


/* FUNCTION NAME: cmlib_queue_enqueue
 * PURPOSE:
 *      enqueue only one queue entry to tail.
 * INPUT:
 *      ptr_q    -- the queue will be pushed.
 *      ptr_data -- the user data will be pushed.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- push success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_TABLE_FULL    -- the queue is full.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_queue_enqueue(
    CMLIB_QUEUE_T *ptr_q,
    void          *ptr_data)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_q);
    HAL_CHECK_PTR(ptr_data);

    if(ptr_q->wr_cnt - ptr_q->rd_cnt >= ptr_q->capacity)
    {
        return CLX_E_TABLE_FULL;
    }

    /* save user data to the tail */
    ptr_q->ptr_entrys[ptr_q->tail_index].ptr_data = ptr_data;

    ptr_q->tail_index++;
    if((UI32_T)ptr_q->tail_index >= ptr_q->capacity)
    {
        ptr_q->tail_index = 0;
    }

    ptr_q->wr_cnt++;

    return CLX_E_OK;
}   /* End of cmlib_queue_enqueue */

/* FUNCTION NAME: cmlib_queue_dequeue
 * PURPOSE:
 *      dequeue only one queue entry from head or tail.
 * INPUT:
 *      ptr_q   -- the queue will be dequeued.
 * OUTPUT:
 *      pptr_data   -- the popped entry data pointer.
 * RETURN:
 *      CLX_E_OK            -- dequeue success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_OTHERS        -- the queue is empty
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_queue_dequeue(
    CMLIB_QUEUE_T *ptr_q,
    void          **pptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_q);
    HAL_CHECK_PTR(pptr_data);

    if(ptr_q->wr_cnt == ptr_q->rd_cnt)
    {
        return CLX_E_OTHERS;
    }

    /* increase head index and get the data at the index */
    *pptr_data = ptr_q->ptr_entrys[ptr_q->head_index].ptr_data;
    ptr_q->ptr_entrys[ptr_q->head_index].ptr_data = NULL;

    ptr_q->head_index++;
    if((UI32_T)ptr_q->head_index >= ptr_q->capacity)
    {
        ptr_q->head_index = 0;
    }

    ptr_q->rd_cnt++;

    return CLX_E_OK;
}   /* End of cmlib_queue_dequeue */

/* FUNCTION NAME: cmlib_queue_dequeues
 * PURPOSE:
 *      dequeue queue entrys from head.
 * INPUT:
 *      ptr_q   -- the queue will be dequeued.
 *      count   -- the dequeue entry number.
 * OUTPUT:
 *      pptr_datas -- the dequeued entry data pointers will be fill in this buffer.
 * RETURN:
 *      CLX_E_OK            -- dequeue success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_OTHERS        -- the entrys in the queue is not enough for popping.
 * NOTES:
 *      the output datas is from head to tail.
 *      eg:
 *          tail| a, b, c|head
 *          pop 2 elements from head, the output datas are:
 *          [0]=c, [1]=b
 */
CLX_ERROR_NO_T
cmlib_queue_dequeues(
    CMLIB_QUEUE_T  *ptr_q,
    const UI32_T   count,
    void           **pptr_datas )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T i = 0;

    /* BODY */
    HAL_CHECK_PTR(ptr_q);
    HAL_CHECK_PTR(pptr_datas);
    if(count == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(ptr_q->wr_cnt - ptr_q->rd_cnt < count)
    {
        return CLX_E_OTHERS;
    }

    for(i=0;i<count;i++)
    {
        pptr_datas[i] = ptr_q->ptr_entrys[ptr_q->head_index].ptr_data;
        ptr_q->ptr_entrys[ptr_q->head_index].ptr_data = NULL;
        ptr_q->head_index++;
        if((UI32_T)ptr_q->head_index >= ptr_q->capacity)
        {
            ptr_q->head_index = 0;
        }
    }

    ptr_q->rd_cnt += count;

    return CLX_E_OK;
}   /* End of cmlib_queue_dequeues */


/* FUNCTION NAME: cmlib_queue_getCount
 * PURPOSE:
 *      it used to get the number of entry in the queue.
 * INPUT:
 *      ptr_q  -- the queue will be get the number of entry.
 * OUTPUT:
 *      ptr_count  -- the count of queue entry.
 * RETURN:
 *      CLX_E_OK            -- get count success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_queue_getCount(
    CMLIB_QUEUE_T *ptr_q,
    UI32_T        *ptr_count )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_q);
    HAL_CHECK_PTR(ptr_count);

    *ptr_count = ptr_q->wr_cnt - ptr_q->rd_cnt;

    return CLX_E_OK;
}   /* End of cmlib_queue_getCount */

/* LOCAL SUBPROGRAM BODIES
 */

