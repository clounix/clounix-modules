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

/* FILE NAME:  cmlib_mpool.h
 * PURPOSE:
 *  provide memory pool API to other modules.
 * NOTES:
 *  1. create a new memory pool.
 *  2. allocate a memory block from the memory pool.
 *  3. free a memory block to the memory pool.
 *  4. destroy a memory pool.
 *
 */
#ifndef CMLIB_MPOOL_H
#define CMLIB_MPOOL_H
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib.h>
#include <osal/osal.h>
/* NAMING CONSTANT DECLARATIONS
 */


/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
*/

struct CMLIB_MPOOL_NODE_S;

/* mem pool node */
typedef struct CMLIB_MPOOL_NODE_S
{
    struct CMLIB_MPOOL_NODE_S *ptr_next; /* point to next node */
} CMLIB_MPOOL_NODE_T;

/* mem pool control block */
typedef struct
{
    UI8_T              *ptr_buf;        /* mem pool start address      */
    CMLIB_MPOOL_NODE_T *ptr_free_list;  /* free list                   */
    UI32_T             block_count;     /* block count in the mem pool */
    UI32_T             block_size;      /* block size in bytes         */
    UI32_T             init_count;      /* the inited count of block   */
    void               *ptr_user_buf;   /* user provide to mem pool    */
} CMLIB_MPOOL_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
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
    void          *ptr_data );

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
    CMLIB_MPOOL_T *ptr_mpool );


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
 *      if the block size is more than or equal to 4 bytes, the user buffer size
 *      can be block_size * block_count.
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
    CMLIB_MPOOL_T **pptr_mpool );

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
    void          **pptr_user_buf );

#endif /* End of CMLIB_MPOOL_H */

