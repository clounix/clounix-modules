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

/* FILE NAME:  cmlib_hash.c
 * PURPOSE:
 * NOTES:
 *
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_hash.h>
#include <cmlib/cmlib_mpool.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
/* DATA TYPE DECLARATIONS
 */
/* GLOBAL VARIABLE DECLARATIONS
 */
//DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_hash.c");
/* LOCAL SUBPROGRAM DECLARATIONS
 */

static CLX_ERROR_NO_T
_cmlib_hash_getBucketIdx(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    UI32_T              *ptr_bkt_idx );

static CLX_ERROR_NO_T
_cmlib_hash_find(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    void                **pptr_found_data,
    BOOL_T              is_delete );

/* STATIC VARIABLE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM BODIES
 */


/* FUNCTION NAME: cmlib_hash_create
 * PURPOSE:
 *      it is used to create a hash table control block, set the bucket array
 *  size as bucket_count, allocate  max_entry_count entries. then return the
 *  control block to user.
 * INPUT:
 *      max_entry_count -- max entry count in hash table.
 *      bucket_count    -- the count of bucket in the hash table, it must be
 *                          more than 0.
 *      key_size        -- the hash key length, in bytes, it will be put in hash
 *                         callback and compare callback function.
 *      hash_callback   -- hash function provided by user.
 *      cmp_callback    -- compare key with user data, provided by user.
 *      ptr_tbl_name    -- the hash table name, max length is CMLIB_NAME_MAX_LEN
 *                          (include '\0')
 * OUTPUT:
 *      pptr_hash_tbl   -- the new hash table control block.
 * RETURN:
 *      CLX_E_OK            -- create ok.
 *      CLX_E_BAD_PARAMETER -- null pointer or count is 0.
 *      CLX_E_NO_MEMORY     -- allocate hash table or buckets failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_hash_create(
    const UI32_T                max_entry_count,
    const UI32_T                bucket_count,
    const UI32_T                key_size,
    const CMLIB_HASH_FUNC_T     hash_callback,
    const CMLIB_HASH_CMP_FUNC_T cmp_callback,
    const C8_T                  *ptr_tbl_name,
    CMLIB_HASH_TBL_T            **pptr_hash_tbl )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_HASH_TBL_T *ptr_hash_tbl = NULL;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    UI32_T bkt_idx = 0;

    /* BODY */
    HAL_CHECK_PTR(pptr_hash_tbl);
    HAL_CHECK_PTR(ptr_tbl_name);
    HAL_CHECK_PTR(hash_callback);
    HAL_CHECK_PTR(cmp_callback);
    if(max_entry_count == 0
        || bucket_count == 0
        || key_size == 0)
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(CMLIB_MULTI_OVERFLOW(sizeof(CMLIB_HASH_TBL_BUCKET_T), bucket_count))
    {
        return CLX_E_BAD_PARAMETER;
    }

    if(CMLIB_ADD_OVERFLOW(sizeof(CMLIB_HASH_TBL_T),
        sizeof(CMLIB_HASH_TBL_BUCKET_T)*bucket_count))
    {
        return CLX_E_BAD_PARAMETER;
    }
    ptr_hash_tbl = (CMLIB_HASH_TBL_T *)osal_alloc(sizeof(CMLIB_HASH_TBL_T)
                    + sizeof(CMLIB_HASH_TBL_BUCKET_T)*bucket_count);

    if(ptr_hash_tbl == NULL)
    {
        return CLX_E_NO_MEMORY;
    }

    ptr_hash_tbl->max_entry_count = max_entry_count;
    ptr_hash_tbl->bucket_count = bucket_count;
    ptr_hash_tbl->entry_count = 0;
    ptr_hash_tbl->key_size = key_size;
    ptr_hash_tbl->hash_callback = hash_callback;
    ptr_hash_tbl->cmp_callback = cmp_callback;
    ptr_hash_tbl->ptr_buckets = (CMLIB_HASH_TBL_BUCKET_T*)(ptr_hash_tbl+1);

    ret = cmlib_mpool_create(sizeof(CMLIB_HASH_TBL_ENTRY_T),
                             max_entry_count,
                             NULL,
                             "hshtbl",
                             (CMLIB_MPOOL_T **)&ptr_hash_tbl->ptr_entry_pool);
    if(ret != CLX_E_OK)
    {
        osal_free(ptr_hash_tbl);
        return CLX_E_OTHERS;
    }
    for(bkt_idx=0;bkt_idx<bucket_count;bkt_idx++)
    {
        ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry = NULL;
    }

    *pptr_hash_tbl = ptr_hash_tbl;

    return CLX_E_OK;
}   /* End of cmlib_hash_create */

/* FUNCTION NAME: cmlib_hash_insert
 * PURPOSE:
 *      it is used to insert user data into the hash table. if the hash table is
 *  full before the insertion, it return CLX_E_TABLE_FULL. if the insert user
 *  data exists in the hash table, if "is_overwrite" is true, the existed data
 *  will be overwritten and the overwritten data will be returned by the last
 *  parameter. if "is_overwrite" is false, it will return CLX_E_ENTRY_EXISTS.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 *      ptr_key       -- the key of the user data.
 *      ptr_user_data -- the user data pointer.
 *      is_overwrite  -- if overwrite the existing data
 *                         TRUE: overwrite the existing data and return the
 *                               overwritten data by the last parameter.
 *                         FALSE: do not overwrite the existing data, return
 *                                CLX_E_ENTRY_EXISTS.
 * OUTPUT:
 *      pptr_overwritten_data -- if the entry exists, then replace the entry
 *                              data with the new data, this is used to return
 *                              old entry data to user. it could be NULL when
 *                              is_overwrite is false.
 * RETURN:
 *      CLX_E_OK            -- insert ok.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 *      CLX_E_ENTRY_EXISTS  -- entry exists and the is_overwrite is FALSE.
 *      CLX_E_OTHERS        -- hash function return error.
 *      CLX_E_TABLE_FULL    -- allocate entry failed, all entrys are allocated.
 * NOTES:
 *      if "is_overwrite" is true, when return CLX_E_OK, please check the
 *  "pptr_overwritten_data" if is NULL, when it is not NULL pointer, it means
 *  there is an overwritten data returned to user.
 */
CLX_ERROR_NO_T
cmlib_hash_insert(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    void                *ptr_user_data,
    const BOOL_T        is_overwrite,
    void                **pptr_overwritten_data)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T bucket_idx = 0;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    CMLIB_HASH_TBL_ENTRY_T *ptr_entry = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);
    HAL_CHECK_PTR(ptr_key);
    HAL_CHECK_PTR(ptr_user_data);

    ret = _cmlib_hash_getBucketIdx(ptr_hash_tbl, ptr_key, &bucket_idx);
    if(ret != CLX_E_OK)
    {
        return CLX_E_OTHERS;
    }

    ptr_entry = ptr_hash_tbl->ptr_buckets[bucket_idx].ptr_entry;

    while(ptr_entry != NULL)
    {
        if(ptr_hash_tbl->cmp_callback(ptr_key,
            ptr_hash_tbl->key_size, ptr_entry->ptr_data) == CLX_E_OK)
        {
            if(is_overwrite == FALSE)
            {
                return CLX_E_ENTRY_EXISTS;
            }
            else
            {
                if(pptr_overwritten_data == NULL)
                {
                    return CLX_E_BAD_PARAMETER;
                }
                *pptr_overwritten_data = ptr_entry->ptr_data;
                ptr_entry->ptr_data = ptr_user_data;
                return CLX_E_OK;
            }
        }
        ptr_entry = ptr_entry->ptr_next;
    }

    ptr_entry = (CMLIB_HASH_TBL_ENTRY_T *)cmlib_mpool_alloc((CMLIB_MPOOL_T *)ptr_hash_tbl->ptr_entry_pool);

    if(ptr_entry == NULL)
    {
        return CLX_E_TABLE_FULL;
    }

    ptr_entry->ptr_data = ptr_user_data;
    ptr_entry->ptr_next = ptr_hash_tbl->ptr_buckets[bucket_idx].ptr_entry;
    ptr_hash_tbl->ptr_buckets[bucket_idx].ptr_entry = ptr_entry;
    ptr_hash_tbl->entry_count++;

    return CLX_E_OK;
}   /* End of cmlib_hash_insert */


/* FUNCTION NAME: cmlib_hash_delete
 * PURPOSE:
 *      it is used to delete an entry from a hash table by a key. if the entry
 *  does not exist, it return CLX_E_ENTRY_NOT_FOUND.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 *      ptr_key       -- the key used to find the entry which will be deleted.
 * OUTPUT:
 *      pptr_deleted_data  -- if the entry exists, return the entry data to user
 * RETURN:
 *      CLX_E_OK              -- delete ok.
 *      CLX_E_BAD_PARAMETER   -- null pointer.
 *      CLX_E_ENTRY_NOT_FOUND -- the entry does not exist.
 *      CLX_E_OTHERS          -- get bucket index failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_hash_delete(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    void                **pptr_deleted_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    return _cmlib_hash_find(ptr_hash_tbl, ptr_key, pptr_deleted_data, TRUE);
}   /* End of cmlib_hash_delete */


/* FUNCTION NAME: cmlib_hash_deleteAll
 * PURPOSE:
 *      it is used to delete all table entries in the hash table.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 *      ptr_cookie    -- cookie data used in del_callback, it can be NULL.
 *      del_callback  -- it is used to process the deleted data entries.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete ok.
 *      CLX_E_OTHERS          -- delete callback return error.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_hash_deleteAll(
    CMLIB_HASH_TBL_T               *ptr_hash_tbl,
    void                           *ptr_cookie,
    const CMLIB_HASH_DELETE_FUNC_T del_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T bkt_idx = 0;
    CMLIB_HASH_TBL_ENTRY_T *ptr_entry = NULL, *ptr_tmp = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);

    /* loop in all bucket */
    for(bkt_idx=0;bkt_idx<ptr_hash_tbl->bucket_count;bkt_idx++)
    {
        /* delete all entry in the bucket */
        ptr_entry = ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry;
        while(ptr_entry != NULL)
        {
            if(del_callback != NULL)
            {
                if(CLX_E_OK != del_callback(ptr_cookie, ptr_entry->ptr_data))
                {
                    ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry = ptr_entry;
                    return CLX_E_OTHERS;
                }
            }
            ptr_tmp = ptr_entry->ptr_next;
            if(CLX_E_OK !=
                cmlib_mpool_free((CMLIB_MPOOL_T *)ptr_hash_tbl->ptr_entry_pool,
                                (void*)ptr_entry))
            {
                ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry = ptr_tmp;
                return CLX_E_OTHERS;
            }
            ptr_hash_tbl->entry_count--;
            ptr_entry = ptr_tmp;
        }
        /* init the ptr_entry as NULL */
        ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry = NULL;
    }

    OSAL_ASSERT(ptr_hash_tbl->entry_count == 0)

    return CLX_E_OK;
}   /* End of cmlib_hash_deleteAll */


/* FUNCTION NAME: cmlib_hash_lookup
 * PURPOSE:
 *      it is used to lookup an entry data by a key. if the entry
 *  does not exist, it return CLX_E_ENTRY_NOT_FOUND.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 *      ptr_key       -- the key used to find the entry.
 * OUTPUT:
 *      pptr_found_data  -- the found entry data.
 * RETURN:
 *      CLX_E_OK              -- delete ok.
 *      CLX_E_BAD_PARAMETER   -- null pointer.
 *      CLX_E_ENTRY_NOT_FOUND -- the entry does not exist.
 *      CLX_E_OTHERS          -- get bucket index failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_hash_lookup(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    void                **pptr_found_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    return _cmlib_hash_find(ptr_hash_tbl, ptr_key, pptr_found_data, FALSE);
}   /* End of cmlib_hash_lookup */

/* FUNCTION NAME: cmlib_hash_traverse
 * PURPOSE:
 *      it is used to traverse all entry data of a hash table and call
 *  trav_callback function to access the entry data.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 *      ptr_cookie    -- it is used in traverse callback function.
 *      trav_callback -- traverse callback function.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- traverse ok.
 *      CLX_E_BAD_PARAMETER   -- null pointer.
 *      CLX_E_OTHERS          -- traverse callback return error
 * NOTES:
 *      the traverse order is from the first bucket to the last. in one bucket,
 *  it is from the first entry to the last entry.
 */
CLX_ERROR_NO_T
cmlib_hash_traverse(
    CMLIB_HASH_TBL_T        *ptr_hash_tbl,
    void                    *ptr_cookie,
    CMLIB_HASH_TRAV_FUNC_T  trav_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T bkt_idx = 0;
    CMLIB_HASH_TBL_ENTRY_T *ptr_entry = NULL;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);
    HAL_CHECK_PTR(trav_callback);

    for(bkt_idx=0;bkt_idx<ptr_hash_tbl->bucket_count;bkt_idx++)
    {
        ptr_entry = ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry;
        while(ptr_entry != NULL)
        {
            ret = trav_callback(ptr_cookie, ptr_entry->ptr_data);
            if(ret != CLX_E_OK)
            {
                return CLX_E_OTHERS;
            }
            ptr_entry = ptr_entry->ptr_next;
        }
    }

    return CLX_E_OK;
}   /* End of cmlib_hash_traverse */

/* FUNCTION NAME: cmlib_hash_getCount
 * PURPOSE:
 *      it is used to get the count of entry in the hash table.
 * INPUT:
 *      ptr_hash_tbl  -- the hash table control block.
 * OUTPUT:
 *      ptr_count  -- it is used to output the count.
 * RETURN:
 *      CLX_E_OK              -- get count ok.
 *      CLX_E_BAD_PARAMETER   -- null pointer.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_hash_getCount(
    CMLIB_HASH_TBL_T        *ptr_hash_tbl,
    UI32_T                  *ptr_count )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);
    HAL_CHECK_PTR(ptr_count);

    *ptr_count = ptr_hash_tbl->entry_count;

    return CLX_E_OK;
}   /* End of cmlib_hash_getCount */

/* FUNCTION NAME: cmlib_hash_destroy
 * PURPOSE:
 *      it is used to destroy a hash table. if user provides a destroy callback
 *  the destroy callback will access every entry data in the hash table.
 * INPUT:
 *      ptr_hash_tbl     -- the hash table control block.
 *      destroy_callback -- it is used to process every entry data by user
 *                          purpose. it could be NULL.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- destroy ok.
 *      CLX_E_BAD_PARAMETER   -- null pointer.
 *      CLX_E_OTHERS          -- destroy callback return error.
 * NOTES:
 *      destroy callback is used to release entry data. it could be null, when
 *  user do not process entry data.
 */
CLX_ERROR_NO_T
cmlib_hash_destroy(
    CMLIB_HASH_TBL_T            *ptr_hash_tbl,
    CMLIB_HASH_DESTROY_FUNC_T   destroy_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T bkt_idx = 0;
    CMLIB_HASH_TBL_ENTRY_T *ptr_entry = NULL;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);

    if(destroy_callback != NULL)
    {
        for(bkt_idx=0;bkt_idx<ptr_hash_tbl->bucket_count;bkt_idx++)
        {
            ptr_entry = ptr_hash_tbl->ptr_buckets[bkt_idx].ptr_entry;
            while(ptr_entry != NULL)
            {
                ret = destroy_callback(ptr_entry->ptr_data);
                if(ret != CLX_E_OK)
                {
                    return CLX_E_OTHERS;
                }
                ptr_entry = ptr_entry->ptr_next;
            }
        }
    }
    ret = cmlib_mpool_destroy((CMLIB_MPOOL_T *)ptr_hash_tbl->ptr_entry_pool, NULL);
    if(ret != CLX_E_OK)
    {
        DIAG_PRINT(HAL_DBG_ERR, "invoke cmlib_mpool_destroy failed, rc=%d\n", ret);
        return CLX_E_OTHERS;
    }

    osal_free(ptr_hash_tbl);

    return CLX_E_OK;
}   /* End of cmlib_hash_destroy */

/* LOCAL SUBPROGRAM BODIES
 */
static CLX_ERROR_NO_T
_cmlib_hash_getBucketIdx(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    UI32_T              *ptr_bkt_idx )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_HASH_T hash = 0;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);
    HAL_CHECK_PTR(ptr_key);
    HAL_CHECK_PTR(ptr_bkt_idx);

    ret = ptr_hash_tbl->hash_callback(ptr_key, ptr_hash_tbl->key_size, &hash);
    if(ret != CLX_E_OK)
    {
        return CLX_E_OTHERS;
    }

    *ptr_bkt_idx = hash % ptr_hash_tbl->bucket_count;
    /* DIAG_PRINT(HAL_DBG_INFO, "hash=%u bkt-idx=%u>\n", hash, *ptr_bkt_idx); */

    return CLX_E_OK;
}   /* End of _cmlib_hash_getBucketIdx */

static CLX_ERROR_NO_T
_cmlib_hash_find(
    CMLIB_HASH_TBL_T    *ptr_hash_tbl,
    void                *ptr_key,
    void                **pptr_found_data,
    BOOL_T              is_delete )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    UI32_T bucket_idx = 0;
    CLX_ERROR_NO_T ret = CLX_E_OK;
    CMLIB_HASH_TBL_ENTRY_T **pptr_entry = NULL, *ptr_entry = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_hash_tbl);
    HAL_CHECK_PTR(ptr_key);
    HAL_CHECK_PTR(pptr_found_data);

    ret = _cmlib_hash_getBucketIdx(ptr_hash_tbl, ptr_key, &bucket_idx);
    if(ret != CLX_E_OK)
    {
        return CLX_E_OTHERS;
    }

    pptr_entry = &ptr_hash_tbl->ptr_buckets[bucket_idx].ptr_entry;

    while(*pptr_entry != NULL)
    {
        if(ptr_hash_tbl->cmp_callback(ptr_key, ptr_hash_tbl->key_size,
                                        (*pptr_entry)->ptr_data) == CLX_E_OK)
        {
            ptr_entry = *pptr_entry;
            *pptr_found_data = ptr_entry->ptr_data;
            if(is_delete == TRUE)
            {
                *pptr_entry = ptr_entry->ptr_next;
                ret = cmlib_mpool_free((CMLIB_MPOOL_T *)ptr_hash_tbl->ptr_entry_pool,
                                       (void*)ptr_entry);
                if(ret != CLX_E_OK)
                {
                    DIAG_PRINT(HAL_DBG_WARN, "invoke cmlib_mpool_free failed, rc=%d\n", ret);
                }

                ptr_hash_tbl->entry_count--;
            }
            return CLX_E_OK;
        }
        pptr_entry = &(*pptr_entry)->ptr_next;
    }

    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of cmlib_hash_delete */

