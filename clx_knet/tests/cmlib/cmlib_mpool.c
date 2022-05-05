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

/* FILE NAME:  cmlib_mpool.c
 * PURPOSE:
 * NOTES:
 *
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_mpool.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
 */
/* GLOBAL VARIABLE DECLARATIONS
 */
DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_mpool.c");
/* LOCAL SUBPROGRAM DECLARATIONS
 */
/* STATIC VARIABLE DECLARATIONS
 */
/* EXPORTED SUBPROGRAM BODIES
 */


/* FUNCTION NAME: cmlib_mpool_free
 * PURPOSE:
 *      it is used to release a data to mem pool.
 * INPUT:
 *      ptr_mpool  -- the mem pool owns the data.
 *      ptr_data   -- the data will be released.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- free succes.
 *      CLX_E_BAD_PARAMETER -- parameter is null pointer or data is not belonged
 *                            the mem pool.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_mpool_free(
    CMLIB_MPOOL_T *ptr_mpool,
    void          *ptr_data )
{
    CMLIB_MPOOL_NODE_T *ptr_node = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_mpool);
    HAL_CHECK_PTR(ptr_data);

    ptr_node = (CMLIB_MPOOL_NODE_T*)ptr_data;
    ptr_node->ptr_next = ptr_mpool->ptr_free_list;

    ptr_mpool->ptr_free_list = ptr_node;
    return CLX_E_OK;
}   /* End of cmlib_mpool_free */

/* FUNCTION NAME: cmlib_mpool_alloc
 * PURPOSE:
 *      it is used to allocate a data from the mem pool.
 * INPUT:
 *      ptr_mpool  -- the mem pool owns the data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      void pointer  -- Successfully allocate this size of memory pool.
 *      NULL          -- Fail to allocate data memory.
 * NOTES:
 *
 */
void *
cmlib_mpool_alloc(
    CMLIB_MPOOL_T *ptr_mpool)
{
    void *ptr_data = NULL;
    /* BODY */
    if (NULL == ptr_mpool)
    {
        return NULL;
    }

    if(ptr_mpool->init_count < ptr_mpool->block_count)
    {
        ptr_data = (void*)(ptr_mpool->ptr_buf
            + ptr_mpool->init_count*ptr_mpool->block_size);
        ptr_mpool->init_count++;
        return ptr_data;
    }

    if(ptr_mpool->ptr_free_list == NULL)
    {
        return NULL;
    }
    else
    {
        ptr_data = (void*)&ptr_mpool->ptr_free_list->ptr_next;
        ptr_mpool->ptr_free_list = ptr_mpool->ptr_free_list->ptr_next;

        return ptr_data;
    }
}   /* End of cmlib_mpool_alloc */


/* FUNCTION NAME: cmlib_mpool_create
 * PURPOSE:
 *      it is used to create a mem pool.
 * INPUT:
 *      block_size   -- the mem block size, in bytes.
 *      block_count  -- the count of blocks in the mem pool.
 *      ptr_user_buf -- NULL    : the mem pool need allocate all the blocks.
 *                      non NULL: user provide the blocks buffer.
 *      ptr_name  -- the mpool name, max length is CMLIB_NAME_MAX_LEN(include '\0')
 * OUTPUT:
 *      pptr_mpool  -- new mem pool control block.
 * RETURN:
 *      CLX_E_OK            -- free succes.
 *      CLX_E_BAD_PARAMETER -- block size * block_count overflow or block size
 *                              is 0 or block count is 0, or there is null
 *                              pointer.
 *      CLX_E_NO_MEMORY     -- allocate mem pool control block or blocks buffer
 *                              failed.
 * NOTES:
 *      if user provide the buffer:
 *      if the block size is more than or equal to 4 bytes, the user buffer size can be block_size * block_count.
 *      if the block size is less than 4 bytes, the user buffer size should be
 *      4 * block_count.
 *      eg:
 *          block_size = 1, block_count = 1K, the user buffer must be 4*1K=4K.
 *          block_size = 5, block_count = 1K, the user buffer must be 5*1K=5K.
 *      in 64-bits environment, above "4 bytes" should change to "8 bytes"
 *
 */
CLX_ERROR_NO_T
cmlib_mpool_create(
    UI32_T        block_size,
    const UI32_T  block_count,
    void * const  ptr_user_buf,
    const C8_T    *ptr_name,
    CMLIB_MPOOL_T **pptr_mpool )
{
    CMLIB_MPOOL_T *ptr_mpool = NULL;

    /* BODY */
    HAL_CHECK_PTR(pptr_mpool);
    HAL_CHECK_PTR(ptr_name);
    if(block_size == 0 || block_count == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(block_size < sizeof(void*))
    {
        block_size = sizeof(void*);
    }

    /* if block size * block count overflow */
    if(CMLIB_MULTI_OVERFLOW(block_size,block_count))
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(ptr_user_buf == NULL)
    {
        /* if block size * block count + sizeof(CMLIB_MPOOL_T) overflow */
        if(CMLIB_ADD_OVERFLOW(block_size * block_count, sizeof(CMLIB_MPOOL_T)))
        {
            return CLX_E_BAD_PARAMETER;
        }

        ptr_mpool =
            (CMLIB_MPOOL_T *)osal_alloc(block_size*block_count+sizeof(CMLIB_MPOOL_T));

        if(ptr_mpool == NULL)
        {
            return CLX_E_NO_MEMORY;
        }

        ptr_mpool->ptr_buf = (UI8_T*)(ptr_mpool+1);

        ptr_mpool->ptr_user_buf = NULL;
    }
    else
    {
        ptr_mpool = (CMLIB_MPOOL_T *)osal_alloc(sizeof(CMLIB_MPOOL_T));
        if(ptr_mpool == NULL)
        {
            return CLX_E_NO_MEMORY;
        }

        ptr_mpool->ptr_buf = (UI8_T*)ptr_user_buf;
        ptr_mpool->ptr_user_buf = ptr_user_buf;
    }

    ptr_mpool->ptr_free_list = NULL;

    ptr_mpool->init_count = 0;
    ptr_mpool->block_count = block_count;
    ptr_mpool->block_size = block_size;

    *pptr_mpool = ptr_mpool;

    return CLX_E_OK;
}   /* End of cmlib_mpool_create */


/* FUNCTION NAME: cmlib_mpool_destroy
 * PURPOSE:
 *      it is used to destroy a mem pool.
 * INPUT:
 *      pptr_mpool  -- the mem pool will be destroyed.
 * OUTPUT:
 *      pptr_user_buf  -- if the mem pool block buffer is provided by user, it
 *                          is used to return the buffer to user. or it can be
 *                          null.
 * RETURN:
 *      CLX_E_OK            -- free succes.
 *      CLX_E_BAD_PARAMETER -- there is null pointer. if the mem pool block
 *                              buffer is not provided by user, pptr_user_buf
 *                              can be null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_mpool_destroy(
    CMLIB_MPOOL_T *ptr_mpool,
    void          **pptr_user_buf )
{
    HAL_CHECK_PTR(ptr_mpool);
    if(ptr_mpool->ptr_user_buf != NULL
        && pptr_user_buf == NULL)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(ptr_mpool->ptr_user_buf == NULL)
    {
        osal_free(ptr_mpool);
    }
    else
    {
        *pptr_user_buf = ptr_mpool->ptr_user_buf;
        osal_free((void*)ptr_mpool);
    }

    return CLX_E_OK;
}   /* End of cmlib_mpool_destroy */

/* LOCAL SUBPROGRAM BODIES
 */

