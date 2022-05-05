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

/* FILE NAME:  cmlib_avl.h
 * PURPOSE:
 *  this file is used to provide AVL tree operations to other users.
 * NOTES:
 *  it contains operations as below:
 *      1. create an AVL tree
 *      2. destroy an AVL tree
 *      3. insert user data to an AVL tree
 *      4. delete a user data from an AVL tree
 *      5. lookup a user data from an AVL tree
 *      6. traver an AVL tree in-order
 *      7. get the AVL tree nodes count
 *
 *
 */
#ifndef CMLIB_AVL_H
#define CMLIB_AVL_H
/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <clx_error.h>
#include <cmlib/cmlib.h>
#include <cmlib/cmlib_mpool.h>
#include <osal/osal.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */


/* DATA TYPE DECLARATIONS
 */


/* FUNCTION TYPE NAME: CMLIB_AVL_CMP_FUNC_T
 * PURPOSE:
 *     it is used to compare user data with node data.
 * INPUT:
 *     ptr_user_param  -- user param, saved in avl tree head
 *     ptr_user_data   -- user data
 *     ptr_node_data   -- the data saved in node
 * OUTPUT:
 *     None.
 * RETURN:
 *     ==0 : user data is equal to node data
 *     <0  : user data is less than node data
 *     >0  : user data is more than node data
 * NOTES:
 *
 */
typedef I32_T (*CMLIB_AVL_CMP_FUNC_T)(
    void *ptr_user_param,
    void *ptr_user_data,
    void *ptr_node_data);

/* FUNCTION TYPE NAME: CMLIB_AVL_DESTROY_FUNC_T
 * PURPOSE:
 *     it is used in destroy function, when user want to destroy an avl tree,
 *     use this function to release node data.
 * INPUT:
 *     ptr_user_param  -- user param, saved in avl tree head
 *     ptr_node_data   -- the data saved in node.
 * OUTPUT:
 *     None.
 * RETURN:
 *     None
 * NOTES:
 *
 */
typedef void (*CMLIB_AVL_DESTROY_FUNC_T)(
    void *ptr_user_param,
    void *ptr_node_data);

/* FUNCTION TYPE NAME: CMLIB_AVL_TRAV_FUN_T
 * PURPOSE:
 *     it is used in traverse function, when traverse an avl tree, this function
 *     is used to process avl tree node data.
 * INPUT:
 *     ptr_user_param  -- user param, saved in avl tree head
 *     ptr_node_data   -- the data saved in node will be traversed.
 *     ptr_cookie      -- traverse data, a cookie data which is one of
 *                          parameters of traverse func
 * OUTPUT:
 *     None.
 * RETURN:
 *     CLX_E_OK     -- traverse success
 *     CLX_E_OTHERS -- traverse fail, stop traversing.
 * NOTES:
 *
 */
typedef CLX_ERROR_NO_T
(*CMLIB_AVL_TRAV_FUNC_T)(
    void *ptr_user_param,
    void *ptr_node_data,
    void *ptr_cookie);


/* avl tree node structure */
typedef struct CMLIB_AVL_NODE_S
{
    struct CMLIB_AVL_NODE_S *ptr_left;        /* point to left avl subtree   */
    struct CMLIB_AVL_NODE_S *ptr_right;       /* point to right avl subtree  */
    void                    *ptr_data;        /* user data saved in the node */
    UI32_T                  height;           /* height of the tree node     */
} CMLIB_AVL_NODE_T;

/* avl tree head node structure*/
typedef struct CMLIB_AVL_HEAD_S
{
    CMLIB_AVL_NODE_T     *ptr_root;       /* root pointer                   */
    C8_T                 *ptr_name;       /* name of avl tree               */
    CMLIB_MPOOL_T        *ptr_node_pool;  /* node pool                      */
    CMLIB_AVL_CMP_FUNC_T cmp_func;        /* compare function               */
    void                 *ptr_user_param; /* user parameter data            */
    UI32_T               node_count;      /* number of node in the tree     */
    UI32_T               capacity;        /* the capacity of the tree
                                           * 0 : unfixed size
                                           * >0: fixed size                 */
} CMLIB_AVL_HEAD_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */


/* FUNCTION NAME: cmlib_avl_create
 * PURPOSE:
 *      allocate an avl tree head and init with parameters.
 *      if capacity > 0 then allocate all the nodes and linked in one free node
 *      list. if capacity == 0 do not allocate nodes when init the head and the
 *      count of nodes is not limited, it's only limited by system memory.
 * INPUT:
 *      ptr_user_param  -- user private parameter, can be NULL. it is a cookie
 *                          data used in compare callback, traverse callback,
 *                          destroy callback.
 *      capacity        -- >0 : the avl tree capacity is fixed.
 *                         ==0: the capacity is not fixed.
 *      cmp_func        -- the compare function, must not be NULL.
 *      ptr_name        -- the avl tree name, max length is CMLIB_NAME_MAX_LEN(include '\0')
 * OUTPUT:
 *      pptr_head       -- the avl tree head will be init with parameters.
 * RETURN:
 *      CLX_E_OK            -- create success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_NO_MEMORY     -- alloc avl tree head failed.
 *      CLX_E_OTHERS        -- create node pool failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_create(
    void                        *ptr_user_param,
    const UI32_T                capacity,
    const CMLIB_AVL_CMP_FUNC_T  cmp_func,
    const C8_T                  *ptr_name,
    CMLIB_AVL_HEAD_T            **pptr_head);



/* FUNCTION NAME: cmlib_avl_destroy
 * PURPOSE:
 *      it is used to destroy an AVL tree, if user provide a destroy function,
 *      then when remove nodes from the tree, every node data will be processed
 *      by destroy function. it will release the head.
 * INPUT:
 *      ptr_head         -- the avl tree head will be destroy.
 *      destroy_func     -- for processing the node data when removing nodes,
 *                          if it is null, don't process node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- destroy success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_destroy(
    const CMLIB_AVL_HEAD_T          *ptr_head,
    const CMLIB_AVL_DESTROY_FUNC_T  destroy_func);

/* FUNCTION NAME: cmlib_avl_insert
 * PURPOSE:
 *      insert a data into the avl tree. it allocate an avl tree node and save
 *      the data in the node, then insert the node into the tree. if the data
 *      has been in the tree, and overwrite_flag is false, then insert failed.
 *      or if overwrite_flag is true, intert the data to the node and the node
 *      data saved in pptr_overwriten_data and return to user.
 * INPUT:
 *      ptr_head        -- the avl head will be inserted.
 *      ptr_data        -- it will be passed to 2nd parameter of compare
 *                          callback, if insertion success, the ptr_user_data
 *                          will be saved in tree node and the node will be
 *                          inserted into the tree.
 *      overwrite_flag  -- TRUE : if data exists, overwrite and return the old
 *                                  data in pptr_overwriten_data.
 *                         FALSE: if data exists, insert failed, return
 *                                  CLX_E_ENTRY_EXISTS.
 * OUTPUT:
 *      pptr_overwritten_data -- if a node data is overwritten, the node data
 *                              will be saved in it and return to user. if
 *                              overwrite_flag is FALSE, this can be NULL.
 * RETURN:
 *      CLX_E_OK            -- insert success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_NO_MEMORY     -- alloc avl tree node failed, it is for unfixed
 *                              size avl tree
 *      CLX_E_ENTRY_EXISTS  -- the node has existed in the tree and
 *                              overwrite_flag is false.
 *      CLX_E_TABLE_FULL    -- no free nodes. it is for fixed size avl tree
 * NOTES:
 *      if overwrite_flag is TRUE, when insert success, please check the
 *      pptr_overwriten_data if it is NULL. it may return overwritten data to user.
 */
CLX_ERROR_NO_T
cmlib_avl_insert(
    CMLIB_AVL_HEAD_T    *ptr_head,
    void                *ptr_data,
    const BOOL_T        overwrite_flag,
    void                **pptr_overwritten_data);

/* FUNCTION NAME: cmlib_avl_delete
 * PURPOSE:
 *      it used to delete an avl tree node by a specified data. after deletion,
 *      the deleted node will be returned by a parameter. if the deleted
 *      node doesn't exist, return CLX_E_ENTRY_NOT_FOUND.
 * INPUT:
 *      ptr_head     -- the avl head will be deleted from
 *      ptr_data     -- the data is used to find the node which will be deleted
 *                      from avl tree, it will be passed to compare callback as
 *                      the 2nd parameter to compare with node data. if the
 *                      compare result is 0, the node will be deleted.
 * OUTPUT:
 *      pptr_node_data  -- the node data saved in the node which is deleted.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the deleted node doesn't exist.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_delete(
    CMLIB_AVL_HEAD_T    *ptr_head,
    void                *ptr_data,
    void                **pptr_node_data);


/* FUNCTION NAME: cmlib_avl_lookup
 * PURPOSE:
 *      find a node data by ptr_data in the avl tree. if the node doesn't exist
 *      return failure.
 * INPUT:
 *      ptr_head     -- the avl head will be looked up
 *      ptr_data     -- the data is used to find the node which is required by
 *                      user from avl tree, if it compare with a node data and
 *                      the result is 0, the node will be found.
 * OUTPUT:
 *      pptr_node_data  -- after lookup success, it is used save the node data.
 * RETURN:
 *      CLX_E_OK              -- lookup success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the finding node doesn't exist.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_lookup(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_node_data);


/* FUNCTION NAME: cmlib_avl_traverse
 * PURPOSE:
 *      traverse every tree node in order to do traverse callback function for
 *      every node data. if callback return error, stop traversing and return
 *      the error to user.
 * INPUT:
 *      ptr_head         -- the avl head will be traversed
 *      trav_func        -- the traverse callback function
 *      ptr_cookie       -- traverse data, a cookie data for traverse function,
 *                          can be NULL.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- traverse success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_OTHERS        -- traverse stop by traverse callback.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_traverse(
    const CMLIB_AVL_HEAD_T      *ptr_head,
    const CMLIB_AVL_TRAV_FUNC_T trav_func,
    void                        *ptr_cookie);

/* FUNCTION NAME: cmlib_avl_getCount
 * PURPOSE:
 *      get the number of nodes in the avl tree.
 * INPUT:
 *      ptr_head     -- the avl head will be counted
 * OUTPUT:
 *      ptr_count    -- the count of nodes
 * RETURN:
 *      CLX_E_OK              -- get count success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_avl_getCount(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    UI32_T                  *ptr_count);


/* FUNCTION NAME: cmlib_avl_getNextData
 * PURPOSE:
 *      get the next closest data for the specified user data in the avl tree.
 *      (In other words, the specified user data(ptr_data) may not be in the avl tree)
 * INPUT:
 *      ptr_head    -- the avl head will be lookup
 *      ptr_data    -- pointer to the date that will be used to find its next closest data
 * OUTPUT:
 *      pptr_next_data    -- pointer to the next data address
 * RETURN:
 *      CLX_E_OK              -- get the next data success.
 *      CLX_E_ENTRY_NOT_FOUND -- get the next data failed.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 * NOTES:
 *
 */

CLX_ERROR_NO_T
cmlib_avl_getNextData(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_next_data);

/* FUNCTION NAME: cmlib_avl_getPrevData
 * PURPOSE:
 *      get the prev closest data for the specified user data in the avl tree.
 *      (In other words, the specified user data(ptr_data) may not be in the avl tree)
 * INPUT:
 *      ptr_head    -- the avl head will be lookup
 *      ptr_data    -- pointer to the date that will be used to find its prev closest data
 * OUTPUT:
 *      pptr_prev_data    -- pointer to the prev data address
 * RETURN:
 *      CLX_E_OK              -- get the prev data success.
 *      CLX_E_ENTRY_NOT_FOUND -- get the prev data failed.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 * NOTES:
 *
 */

CLX_ERROR_NO_T
cmlib_avl_getPrevData(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_prev_data);

#endif /* End of CMLIB_AVL_H */

