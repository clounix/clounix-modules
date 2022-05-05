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

/* FILE NAME:  cmlib_hash.h
 * PURPOSE:
 * NOTES:
 *
 *
 *
 */
#ifndef CMLIB_HASH_H
#define CMLIB_HASH_H
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

typedef UI32_T CMLIB_HASH_T;


/* FUNCTION TYPE NAME: CMLIB_HASH_FUNC_T
 * PURPOSE:
 *      it is used to get hash value from a key.
 * INPUT:
 *      ptr_key  -- the key used to get hash value.
 *      key_size -- the length of key, in bytes.
 * OUTPUT:
 *      ptr_hash  -- the hash value of the key.
 * RETURN:
 *      CLX_E_OK            -- get hash value success.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_HASH_FUNC_T)(
    void         *ptr_key,
    const UI32_T key_size,
    CMLIB_HASH_T *ptr_hash );


/* FUNCTION TYPE NAME: CMLIB_HASH_CMP_FUNC_T
 * PURPOSE:
 *      it is used to compare key with user data, if the key matches the
 *  user data.
 * INPUT:
 *      ptr_key       -- the key used to compare.
 *      key_size      -- the length of key, in bytes.
 *      ptr_user_data -- the user data pointer.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- the key matches the user data.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 *      CLX_E_OTHERS        -- the key does not belong to the user data.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_HASH_CMP_FUNC_T)(
    void         *ptr_key,
    const UI32_T key_size,
    void         *ptr_user_data );


/* FUNCTION TYPE NAME: CMLIB_HASH_DESTROY_FUNC_T
 * PURPOSE:
 *      it is used to process entry data when destroy a hash table.
 * INPUT:
 *      ptr_entry_data  -- user data saved in the hash entry.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- process ok.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_HASH_DESTROY_FUNC_T)(
    void    *ptr_entry_data );

/* FUNCTION TYPE NAME: CMLIB_HASH_DELETE_FUNC_T
 * PURPOSE:
 *      it is used to process the deleted data entry.
 * INPUT:
 *      ptr_cookie     -- the cookie data from delete function, it can be NULL.
 *      ptr_entry_data -- the entry data will be deleted.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- process ok.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_HASH_DELETE_FUNC_T)(
    void    *ptr_cookie,
    void    *ptr_entry_data );

/* FUNCTION TYPE NAME: CMLIB_HASH_TRAV_FUNC_T
 * PURPOSE:
 *      it is used to process entry data when traverse a hash table.
 * INPUT:
 *      ptr_cookie     -- the cookie data.
 *      ptr_entry_data -- the user data saved in the entry.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- process ok.
 *      CLX_E_BAD_PARAMETER -- null pointer.
 *      CLX_E_OTHERS        -- traverse error.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_HASH_TRAV_FUNC_T)(
    void    *ptr_cookie,
    void    *ptr_entry_data );



typedef struct CMLIB_HASH_TBL_ENTRY_S
{
    struct CMLIB_HASH_TBL_ENTRY_S *ptr_next;   /* point to next entry */
    void                          *ptr_data;   /* point to user data  */
} CMLIB_HASH_TBL_ENTRY_T;

typedef struct CMLIB_HASH_TBL_BUCKET_S
{
    CMLIB_HASH_TBL_ENTRY_T *ptr_entry;        /* point to the entry list */
} CMLIB_HASH_TBL_BUCKET_T;


typedef struct
{
    UI32_T                      max_entry_count;   /* max count of entry      */
    UI32_T                      bucket_count;      /* the bucket array size   */
    UI32_T                      entry_count;       /* the count of entry in the
                                                    * hash table.
                                                    */
    UI32_T                      key_size;          /* hash key lenght: bytes  */
    CMLIB_HASH_FUNC_T           hash_callback;     /* hash callback           */
    CMLIB_HASH_CMP_FUNC_T       cmp_callback;      /* compare callback        */
    CMLIB_HASH_TBL_BUCKET_T     *ptr_buckets;      /* point to bucket array   */

    void                       *ptr_entry_pool;    /* it is used to manage table
                                                    * entrys.
                                                    */
} CMLIB_HASH_TBL_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
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
    CMLIB_HASH_TBL_T            **pptr_hash_tbl );

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
    void                **pptr_overwritten_data);


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
    void                **pptr_deleted_data );

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
    const CMLIB_HASH_DELETE_FUNC_T del_callback );


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
    void                **pptr_found_data );

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
 *      CLX_E_OTHERS          -- traverse callback return error.
 * NOTES:
 *      the traverse order is from the first bucket to the last. in one bucket,
 *  it is from the first entry to the last entry.
 */
CLX_ERROR_NO_T
cmlib_hash_traverse(
    CMLIB_HASH_TBL_T        *ptr_hash_tbl,
    void                    *ptr_cookie,
    CMLIB_HASH_TRAV_FUNC_T  trav_callback );

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
    UI32_T                  *ptr_count );

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
    CMLIB_HASH_DESTROY_FUNC_T   destroy_callback );

#endif /* End of CMLIB_HASH_H */

