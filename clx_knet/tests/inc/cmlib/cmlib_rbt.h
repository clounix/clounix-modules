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

/* FILE NAME:  cmlib_rbt.h
 * PURPOSE:
 *  this file is used to provide red black tree operations to other users.
 * NOTES:
 *
 *
 */
#ifndef CMLIB_RBT_H
#define CMLIB_RBT_H
/* INCLUDE FILE DECLARATIONS
 */
#include <clx_types.h>
#include <clx_error.h>
#include <cmlib/cmlib_mpool.h>

/* NAMING CONSTANT DECLARATIONS
 */

/* MACRO FUNCTION DECLARATIONS
 */

/* DATA TYPE DECLARATIONS
 */
typedef enum
{
    CMLIB_RBT_COLOR_TYPE_BLACK  = 0x0,
    CMLIB_RBT_COLOR_TYPE_RED,
    CMLIB_RBT_COLOR_TYPE_DOUBLE_BLACK,
    CMLIB_RBT_COLOR_TYPE_LAST
} CMLIB_RBT_COLOR_TYPE_T;

/* FUNCTION TYPE NAME: CMLIB_RBT_CMP_FUNC_T
 * PURPOSE:
 *     it is used to compare user data with node data.
 * INPUT:
 *     ptr_user_param  -- user param, saved in red black tree head
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
typedef I32_T (*CMLIB_RBT_CMP_FUNC_T)(
    void *ptr_user_param,
    void *ptr_user_data,
    void *ptr_node_data);

/* FUNCTION TYPE NAME: CMLIB_RBT_DESTROY_FUNC_T
 * PURPOSE:
 *     it is used in destroy function, when user want to destroy an rb tree,
 *     use this function to release node data.
 * INPUT:
 *     ptr_user_param  -- user param, saved in rb tree head
 *     ptr_node_data   -- the data saved in node.
 * OUTPUT:
 *     None.
 * RETURN:
 *     None
 * NOTES:
 *
 */
typedef void (*CMLIB_RBT_DESTROY_FUNC_T)(
    void *ptr_user_param,
    void *ptr_node_data);

typedef CLX_ERROR_NO_T
(*CMLIB_RBT_TRAV_FUNC_T)(
    void *ptr_user_param,
    void *ptr_node_data,
    void *ptr_cookie);

typedef void (*CMLIB_RBT_UPDATE_FUNC_T)(
    void *ptr_node,
    void *ptr_user_data);

/* rb tree node structure */
typedef struct CMLIB_RBT_NODE_S
{
    struct CMLIB_RBT_NODE_S *ptr_left;   /* left child */
    struct CMLIB_RBT_NODE_S *ptr_right;  /* right child */
    struct CMLIB_RBT_NODE_S *ptr_parent; /* parent */
    void                    *ptr_data;   /* user data saved in the node */
    CMLIB_RBT_COLOR_TYPE_T  color;       /* node color (BLACK, RED and DOUBLE_BLACK) */
} CMLIB_RBT_NODE_T;

/* rb tree head node structure*/
typedef struct CMLIB_RBT_HEAD_S
{
    CMLIB_RBT_NODE_T        *ptr_root;      /* root pointer */
    C8_T                    *ptr_name;      /* name of rb tree */
    CMLIB_MPOOL_T           *ptr_node_pool; /* node pool */
    CMLIB_RBT_CMP_FUNC_T    cmp_func;       /* compare function             */
    void                    *ptr_user_param;/* user parameter data          */
    UI32_T                   node_count;    /* the count of nodes in the rb tree */
    UI32_T                   capacity;      /* the capacity of the rb tree
                                             * 0 : unfixed size
                                             * >0: fixed size
                                             */
    CMLIB_RBT_NODE_T        nil_node;       /* the null leaf node */
} CMLIB_RBT_HEAD_T;

/* EXPORTED SUBPROGRAM SPECIFICATIONS
 */
/* FUNCTION NAME: cmlib_rbt_create
 * PURPOSE:
 *      allocate an red black tree head and nodes.
 *
 * INPUT:
 *      ptr_user_param  -- user private parameter, can be NULL. it is a cookie
 *                         data used in compare callback, traverse callback,
 *                         destroy callback.
 *      capacity        -- the rb tree node number.
 *      cmp_func        -- the compare function, must not be NULL.
 *      ptr_name        -- the rb tree name, max length is CMLIB_NAME_MAX_LEN(include '\0')
 * OUTPUT:
 *      pptr_head       -- the returned red black tree head.
 * RETURN:
 *      CLX_E_OK            -- create success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_NO_MEMORY     -- alloc red black tree head failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_create(
    void                        *ptr_user_param,
    const UI32_T                capacity,
    const CMLIB_RBT_CMP_FUNC_T  cmp_func,
    const C8_T                  *ptr_name,
    CMLIB_RBT_HEAD_T            **pptr_head);

/* FUNCTION NAME: cmlib_rbt_destroy
 * PURPOSE:
 *      it is used to destroy an red black tree, if user provide a destroy function,
 *      then when remove nodes from the tree, every node data will be processed
 *      by destroy function. it will release the head.
 * INPUT:
 *      ptr_head        -- the red black tree head will be destroy.
 *      destroy_func    -- for processing the node data when removing nodes,
 *                         if it is null, don't process node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- destroy success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_destroy(
    CMLIB_RBT_HEAD_T                *ptr_head,
    const CMLIB_RBT_DESTROY_FUNC_T  destroy_func);

/* FUNCTION NAME: cmlib_rbt_insert
 * PURPOSE:
 *      Insert a node into the red black tree.
 *
 * INPUT:
 *      ptr_head                -- rb tree root head.
 *      ptr_data                -- include key value.
 *      overwrite_flag          -- TRUE : if data exists, overwrite and return the old
 *                                        data in pptr_overwriten_data.
 *                                 FALSE: if data exists, insert failed, return
 *                                        CLX_E_ENTRY_EXISTS.
 * OUTPUT:
 *      pptr_overwritten_data   -- if a node data is overwritten, the node data
 *                                 will be saved in it and return to user. if
 *                                 overwrite_flag is FALSE, it would be NULL.
 *      pptr_insert_node        -- save insert node pointer.
 * RETURN:
 *      CLX_E_OK            -- insert success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_NO_MEMORY     -- alloc rb tree node failed.
 *      CLX_E_TABLE_FULL    -- alloc rb tree node from pool failed.
 *      CLX_E_ENTRY_EXISTS  -- the node has existed in the tree.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_insert(
    CMLIB_RBT_HEAD_T    *ptr_head,
    void                *ptr_data,
    const BOOL_T        overwrite_flag,
    void                **pptr_overwritten_data,
    void                **pptr_insert_node);

/* FUNCTION NAME: cmlib_rbt_delete
 * PURPOSE:
 *      Delete a node from the red black tree.
 *
 * INPUT:
 *      ptr_head        -- rb tree root head.
 *      ptr_data        -- include key value.
 *      dyn_cmp_func    -- user compare function.  If it is null,
 *                         default compare function will be used.
 *      update_func     -- update user data and tree node pointer maping relationship.
 *                         Because user data may save corresponding tree node pointer.
 *                         If it is null, do not perform this callback function.
 * OUTPUT:
 *      pptr_node_data      -- return user data pointer.
 * RETURN:
 *      CLX_E_OK                -- delete success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- delete fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_delete(
    CMLIB_RBT_HEAD_T                *ptr_head,
    void                            *ptr_data,
    void                            **pptr_node_data,
    const CMLIB_RBT_CMP_FUNC_T      dyn_cmp_func,
    const CMLIB_RBT_UPDATE_FUNC_T   update_func);

/* FUNCTION NAME: cmlib_rbt_deleteNode
 * PURPOSE:
 *      Delete a node from the red black tree.
 *
 * INPUT:
 *      ptr_head        -- rb tree root head.
 *      ptr_del_node    -- the rb tree node to be deleted.
 *      update_func     -- update user data and tree node pointer maping relationship.
 *                         Because user data may save corresponding tree node pointer.
 *                         If it is null, do not perform this callback function.
 * OUTPUT:
 *      pptr_del_node_data  -- return user data pointer.
 * RETURN:
 *      CLX_E_OK                -- delete success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- delete fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_deleteNode(
    CMLIB_RBT_HEAD_T                *ptr_head,
    const CMLIB_RBT_NODE_T          *ptr_del_node,
    void                            **pptr_del_node_data,
    const CMLIB_RBT_UPDATE_FUNC_T   update_func);

/* FUNCTION NAME: cmlib_rbt_getFirst
 * PURPOSE:
 *      find first node in red black tree.
 *
 * INPUT:
 *      ptr_head    -- red black tree root head.
 * OUTPUT:
 *
 * RETURN:
 *      CMLIB_RBT_NODE_T    -- the first node in red black tree.
 *      NULL                -- the red black tree has no nodes.
 * NOTES:
 *
 */
CMLIB_RBT_NODE_T *
cmlib_rbt_getFirst(
    const CMLIB_RBT_HEAD_T  *ptr_head);

/* FUNCTION NAME: cmlib_rbt_getNext
 * PURPOSE:
 *      find next node in red black tree.
 *
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_node    -- current red black tree node.
 * OUTPUT:
 *
 * RETURN:
 *      CMLIB_RBT_NODE_T    -- the node next to ptr_node.
 *      NULL                -- the red black tree has no next nodes.
 * NOTES:
 *
 */
CMLIB_RBT_NODE_T *
cmlib_rbt_getNext(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    const CMLIB_RBT_NODE_T  *ptr_node);

/* FUNCTION NAME: cmlib_rbt_getPrev
 * PURPOSE:
 *      find previous node in red black tree.
 *
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_node    -- current red black tree node.
 * OUTPUT:
 *
 * RETURN:
 *      CMLIB_RBT_NODE_T    -- the previous node to ptr_node.
 *      NULL                -- the red black tree has no previous nodes.
 * NOTES:
 *
 */
CMLIB_RBT_NODE_T *
cmlib_rbt_getPrev(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    const CMLIB_RBT_NODE_T  *ptr_node);

/* FUNCTION NAME: cmlib_rbt_traverse
 * PURPOSE:
 *      traverse every node in red black tree and
 *      perform callback function in node.
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      trav_func   -- user register function in node.
 *      ptr_cookie  -- cookie data for trav_callback.
 * OUTPUT:
 *      None
 * RETURN:
 *      CLX_E_OK            -- traverse success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_OTHERS        -- traverse stop by traverse callback.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_traverse(
    const CMLIB_RBT_HEAD_T      *ptr_head,
    const CMLIB_RBT_TRAV_FUNC_T trav_func,
    void                        *ptr_cookie);

/* FUNCTION NAME: cmlib_rbt_getCount
 * PURPOSE:
 *      get the number of nodes in the red black tree.
 * INPUT:
 *      ptr_head    -- the red black tree head will be counted
 * OUTPUT:
 *      ptr_count   -- the count of nodes
 * RETURN:
 *      CLX_E_OK              -- get count success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_getCount(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    UI32_T                  *ptr_count);

/* FUNCTION NAME: cmlib_rbt_find
 * PURPOSE:
 *      find node in red black tree with given key.
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_data    -- include key value.
 * OUTPUT:
 *      pptr_node   -- return found node pointer.
 * RETURN:
 *      CLX_E_OK                -- find success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- find fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_find(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node);

/* FUNCTION NAME: cmlib_rbt_findNext
 * PURPOSE:
 *      find next node in red black tree with given key.
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_data    -- include key value.
 * OUTPUT:
 *      pptr_node   -- return found node pointer.
 * RETURN:
 *      CLX_E_OK                -- find success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- find fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_findNext(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node);

/* FUNCTION NAME: cmlib_rbt_findGE
 * PURPOSE:
 *      find next node in red black tree with given key,
 *      the found node data key >= input data key.
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_data    -- include key value.
 * OUTPUT:
 *      pptr_node   -- return found node pointer.
 * RETURN:
 *      CLX_E_OK                -- find success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- find fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_findGE(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node);

/* FUNCTION NAME: cmlib_rbt_findPrev
 * PURPOSE:
 *      find prev node in red black tree with given key.
 * INPUT:
 *      ptr_head    -- red black tree root head.
 *      ptr_data    -- include key value.
 * OUTPUT:
 *      pptr_node   -- return found node pointer.
 * RETURN:
 *      CLX_E_OK                -- find success.
 *      CLX_E_BAD_PARAMETER     -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND   -- find fail.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_rbt_findPrev(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node);

#endif /* End of CMLIB_RBT_H */
