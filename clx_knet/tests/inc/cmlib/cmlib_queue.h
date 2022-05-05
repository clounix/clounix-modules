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

/* FILE NAME:  cmlib_queue.h
 * PURPOSE:
 *  this file is used to provide queue operations to other users.
 * NOTES:
 *  it contains operations as below:
 *      1. create a queue
 *      2. destroy a queue
 *      3. push one element to tail of a queue
 *      4. pop 1~N elements from head of a queue
 *      5. get usage(length) of a queue.
 *
 */
#ifndef CMLIB_QUEUE_H
#define CMLIB_QUEUE_H
/* INCLUDE FILE DECLARATIONS
 */

#include <cmlib/cmlib.h>


/* NAMING CONSTANT DECLARATIONS
 */

/* the queue operation position */
typedef enum
{
    CMLIB_QUEUE_POS_HEAD = 0,         /* operation position at head */
    CMLIB_QUEUE_POS_TAIL,             /* operation position at tail */
    CMLIB_QUEUE_POS_LAST,
} CMLIB_QUEUE_POS_T;

/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
 */

/* the queue entry structure */
typedef struct
{
    void *ptr_data;         /* the user data saved in the entrys */
} CMLIB_QUEUE_ENTRY_T;


/* FUNCTION TYPE NAME: CMLIB_QUEUE_DESTROY_FUNC_T
 * PURPOSE:
 *      it is used to release user data saved in queue entry when destoy the
 *      queue.
 * INPUT:
 *      ptr_user_data  -- the queue element data will be released.
 * OUTPUT:
 *      None.
 * RETURN:
 *      None.
 * NOTES:
 *
 */
typedef void (*CMLIB_QUEUE_DESTROY_FUNC_T)(
    void *ptr_user_data );

/* the queue head structure */
typedef struct CMLIB_QUEUE_S
{
    I32_T                head_index;  /* index of the queue head entry can be read  */
    I32_T                tail_index;  /* index of the queue tail entry can be write */
    UI32_T               wr_cnt;      /* enqueue total count                        */
    UI32_T               rd_cnt;      /* dequeue total count                        */
    UI32_T               capacity;    /* the queue size                             */
    CMLIB_QUEUE_ENTRY_T  *ptr_entrys; /* the queue entry buffer                     */
} CMLIB_QUEUE_T;


/* EXPORTED SUBPROGRAM SPECIFICATIONS
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
    const UI32_T   capacity,
    const C8_T     *ptr_name,
    CMLIB_QUEUE_T  **pptr_q );



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
    CMLIB_QUEUE_T                     *ptr_q,
    const CMLIB_QUEUE_DESTROY_FUNC_T  destroy_callback );


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
    void          *ptr_data);


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
    CMLIB_QUEUE_T *ptr_q,
    const UI32_T  count,
    void          **pptr_datas);

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
    void          **pptr_data );


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
    UI32_T        *ptr_count );

#endif /* End of CMLIB_QUEUE_H */

