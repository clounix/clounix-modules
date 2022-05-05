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

/* FILE NAME:  cmlib_list.c
 * PURPOSE:
 * NOTES:
 *
 *
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_list.h>
#include <cmlib/cmlib_mpool.h>

/* NAMING CONSTANT DECLARATIONS
 */
/* MACRO FUNCTION DECLARATIONS
 */
#define _CMLIB_LIST_CONTAINER_OF(ptr, type, member) \
    ((type*)((char*)(ptr)-(unsigned long)(&((type*)0)->member)))

/* DATA TYPE DECLARATIONS
 */
/* GLOBAL VARIABLE DECLARATIONS
 */
DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_list.c");
/* LOCAL SUBPROGRAM DECLARATIONS
 */

static CLX_ERROR_NO_T
_cmlib_list_freeNode(
    CMLIB_LIST_T      *ptr_list,
    CMLIB_LIST_NODE_T *ptr_node );

static CMLIB_LIST_NODE_T *
_cmlib_list_allocNode(
    CMLIB_LIST_T       *ptr_list);

static CLX_ERROR_NO_T
_cmlib_list_getNodeData(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                **pptr_node_data );

static CLX_ERROR_NO_T
_cmlib_list_insertByFunc(
    CMLIB_LIST_T          *ptr_list,
    void                  *ptr_data,
    CMLIB_LIST_CMP_FUNC_T cmp_callback );

static CLX_ERROR_NO_T
_cmlib_list_insertToHead(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data );

static CLX_ERROR_NO_T
_cmlib_list_insertToTail(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data );

static CLX_ERROR_NO_T
_cmlib_list_insertAfter(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                *ptr_data );

static CLX_ERROR_NO_T
_cmlib_list_insertBefore(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                *ptr_data );

static CLX_ERROR_NO_T
_cmlib_list_delete(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node );

static CLX_ERROR_NO_T
_cmlib_list_deleteByData(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data );

static CLX_ERROR_NO_T
_cmlib_list_locateByFunc(
    CMLIB_LIST_T                    *ptr_list,
    void                            *ptr_cookie,
    const CMLIB_LIST_LOCATE_FUNC_T  locate_callback,
    CMLIB_LIST_NODE_T               **pptr_node );

static CLX_ERROR_NO_T
_cmlib_list_locateHead(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   **pptr_node );

static CLX_ERROR_NO_T
_cmlib_list_locateTail(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   **pptr_node );

static CLX_ERROR_NO_T
_cmlib_list_next(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    CMLIB_LIST_NODE_T   **pptr_next_node );

static CLX_ERROR_NO_T
_cmlib_list_prev(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    CMLIB_LIST_NODE_T   **pptr_prev_node );

static CLX_ERROR_NO_T
_cmlib_list_destroy(
    CMLIB_LIST_T              *ptr_list,
    CMLIB_LIST_DESTROY_FUNC_T destroy_callback );

static CLX_ERROR_NO_T
_cmlib_list_deleteAll(
    CMLIB_LIST_T              *ptr_list,
    CMLIB_LIST_DELETE_FUNC_T delete_callback );

/* STATIC VARIABLE DECLARATIONS
 */

static CMLIB_LIST_OPS_T _cmlib_list_ops =
{
    /*.getNodeData   = */_cmlib_list_getNodeData,
    /*.insertByFunc  = */_cmlib_list_insertByFunc,
    /*.insertToHead  = */_cmlib_list_insertToHead,
    /*.insertToTail  = */_cmlib_list_insertToTail,
    /*.insertBefore  = */_cmlib_list_insertBefore,
    /*.insertAfter   = */_cmlib_list_insertAfter,
    /*.deleteNode    = */_cmlib_list_delete,
    /*.deleteByData  = */_cmlib_list_deleteByData,
    /*.locateByFunc  = */_cmlib_list_locateByFunc,
    /*.locateHead    = */_cmlib_list_locateHead,
    /*.locateTail    = */_cmlib_list_locateTail,
    /*.next          = */_cmlib_list_next,
    /*.prev          = */_cmlib_list_prev,
    /*.getLength     = */NULL,
    /*.destroy       = */_cmlib_list_destroy,
    /*.deleteAll     =*/_cmlib_list_deleteAll,
};

/* EXPORTED SUBPROGRAM BODIES
 */

/* FUNCTION NAME: cmlib_list_create
 * PURPOSE:
 *      it is used to create a linked list head.
 * INPUT:
 *      list_type -- the linked list type.
 *                   CMLIB_LIST_TYPE_SINGLY  : singly linked list.
 *                   CMLIB_LIST_TYPE_DOUBLY  : doubly linked list.
 *      capacity  -- the linked list capacity, if it is 0 then the linked list
 *                   size is unfixed, or is fixed.
 *      ptr_name  -- the linked list name, max length is CMLIB_NAME_MAX_LEN(include '\0')
 * OUTPUT:
 *      ptr_list  -- the new list head.
 * RETURN:
 *      CLX_E_OK            -- create success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null or the type is invalid.
 *      CLX_E_NO_MEMORY     -- list head failed.
 *      CLX_E_OTHERS        -- create node pool failed.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_create(
    const UI32_T            capacity,
    const CMLIB_LIST_TYPE_T list_type,
    const C8_T             *ptr_name,
    CMLIB_LIST_T            **pptr_list )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_T *ptr_list = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_name);
    HAL_CHECK_PTR(pptr_list);
    HAL_CHECK_ENUM_RANGE(list_type, CMLIB_LIST_TYPE_LAST);

    ptr_list = (CMLIB_LIST_T *)osal_alloc(sizeof(CMLIB_LIST_T));
    if(ptr_list == NULL)
    {
        return CLX_E_NO_MEMORY;
    }

    osal_memset(ptr_list, 0, sizeof(*ptr_list));
    ptr_list->capacity = capacity;
    ptr_list->type = list_type;

    if(capacity > 0)
    {
        CMLIB_MPOOL_T *ptr_node_pool = NULL;
        if(cmlib_mpool_create(sizeof(CMLIB_LIST_NODE_T),
             capacity, NULL, "singly list", &ptr_node_pool) != CLX_E_OK)
        {
            osal_free(ptr_list);
            return CLX_E_OTHERS;
        }
        ptr_list->ptr_node_pool = (void*)ptr_node_pool;
    }
    ptr_list->ptr_ops = &_cmlib_list_ops;

    *pptr_list = ptr_list;

    return CLX_E_OK;
}   /* End of cmlib_list_create */

/* FUNCTION NAME: cmlib_list_destroy
 * PURPOSE:
 *      it is used to destroy a linked list and release head and nodes. if user
 *      provide a destroy function, it will release every node data in the
 *      linked list.
 * INPUT:
 *      ptr_list         -- the destroyed linked list.
 *      destroy_callback -- destroy function is used to destroy node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- destroy success.
 *      CLX_E_BAD_PARAMETER  -- ptr_list is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_destroy(
    CMLIB_LIST_T                    *ptr_list,
    const CMLIB_LIST_DESTROY_FUNC_T destroy_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->destroy))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->destroy(ptr_list, destroy_callback);

}   /* End of cmlib_list_destroy */


/* FUNCTION NAME: cmlib_list_getNodeData
 * PURPOSE:
 *      it is used to get the node data, the node may be singly/doubly/circular
 *      linked list node.
 * INPUT:
 *      ptr_list  -- the list owns the node
 *      ptr_node  -- the node will be get data
 * OUTPUT:
 *      pptr_node_data -- the data saved in the node.
 * RETURN:
 *      CLX_E_OK             -- destroy success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_getNodeData(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  *ptr_node,
    void               **pptr_node_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_node_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->getNodeData))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->getNodeData(ptr_list, ptr_node, pptr_node_data);

}   /* End of cmlib_list_getNodeData */

/* FUNCTION NAME: cmlib_list_insertByFunc
 * PURPOSE:
 *      it is used to insert a data by a insert function which is used to decide
 *      where the inserted node should be inserted. if the insert function does
 *      not give a position(node), the node will be inserted to the tail of the
 *      linked list. if the insert function give a position(node), the node will
 *      be inserted before the position(node).
 * INPUT:
 *      ptr_list        -- the node will be inseted in it.
 *      ptr_data        -- the inserted data.
 *      cmp_callback    -- the inserted callback function, used to find the
 *                         insert position.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_insertByFunc(
    CMLIB_LIST_T                *ptr_list,
    void                        *ptr_data,
    const CMLIB_LIST_CMP_FUNC_T cmp_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(cmp_callback);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->insertByFunc))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->insertByFunc(ptr_list, ptr_data, cmp_callback);
}   /* End of cmlib_list_insertByFunc */

/* FUNCTION NAME: cmlib_list_insertToHead
 * PURPOSE:
 *      it is used to insert a data to the front of linked list.
 * INPUT:
 *      ptr_list  -- the node will be inserted in it.
 *      ptr_data  -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_insertToHead(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->insertToHead))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->insertToHead(ptr_list, ptr_data);

}   /* End of cmlib_list_insertToHead */

/* FUNCTION NAME: cmlib_list_insertToTail
 * PURPOSE:
 *      it is used to insert a data to the tail of linked list.
 * INPUT:
 *      ptr_list  -- the node will be inserted in it.
 *      ptr_data  -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_insertToTail(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->insertToTail))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->insertToTail(ptr_list, ptr_data);

}   /* End of cmlib_list_insertToTail */

/* FUNCTION NAME: cmlib_list_insertBefore
 * PURPOSE:
 *      it is used to insert a data before a specified node.
 * INPUT:
 *      ptr_list -- the node will be inserted in it.
 *      ptr_node -- insert before it.
 *      ptr_data -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 *      CLX_E_UNSUPPORT      -- list don't support this operation
 * NOTES:
 *      singly & circular linked list don't support this operation.
 */
CLX_ERROR_NO_T
cmlib_list_insertBefore(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  *ptr_node,
    void               *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(ptr_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->insertBefore))
    {
        return CLX_E_NOT_SUPPORT;
    }


    return ptr_list->ptr_ops->insertBefore(ptr_list, ptr_node, ptr_data);


}   /* End of cmlib_list_insertBefore */

/* FUNCTION NAME: cmlib_list_insertAfter
 * PURPOSE:
 *      it is used to insert a data after a specified node.
 * INPUT:
 *      ptr_list -- the node will be inserted in it.
 *      ptr_node -- insert after it.
 *      ptr_data -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- insert success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_TABLE_FULL    -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY     -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_insertAfter(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(ptr_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->insertAfter))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->insertAfter(ptr_list, ptr_node, ptr_data);

}   /* End of cmlib_list_insertAfter */

/* FUNCTION NAME: cmlib_list_delete
 * PURPOSE:
 *      it is used to delete a node from a linked list.
 * INPUT:
 *      ptr_list         -- the node will delete from it.
 *      ptr_delete_node  -- the deleted node pointer.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the delete node is not in the linked list
 * NOTES:
 *      the delete node data will not process in this function
 */
CLX_ERROR_NO_T
cmlib_list_delete(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  *ptr_delete_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_delete_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->deleteNode))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->deleteNode(ptr_list, ptr_delete_node);

}   /* End of cmlib_list_delete */

/* FUNCTION NAME: cmlib_list_deleteAll
 * PURPOSE:
 *      it is used to delete alll node from a linked list.
 * INPUT:
 *      ptr_list          -- the node will delete from it.
 *      delete_callback  -- delete function is used to destroy node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 * NOTE:
 *      If the list is empty, we return CLX_E_OK;
 */
CLX_ERROR_NO_T
cmlib_list_deleteAll(
    CMLIB_LIST_T       *ptr_list,
    const CMLIB_LIST_DELETE_FUNC_T delete_callback)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);

    if((NULL == ptr_list->ptr_ops) || (NULL == ptr_list->ptr_ops->deleteAll))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->deleteAll(ptr_list, delete_callback);
}

/* FUNCTION NAME: cmlib_list_deleteByData
 * PURPOSE:
 *      it is used to delete a node from a linked list by user data pointer.
 * INPUT:
 *      ptr_list         -- the node will delete from it.
 *      ptr_delete_data  -- the deleted user data pointer.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the delete node is not in the linked list
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_deleteByData(
    CMLIB_LIST_T       *ptr_list,
    void               *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->deleteByData))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->deleteByData(ptr_list, ptr_data);

}   /* End of cmlib_list_delete */


/* FUNCTION NAME: cmlib_list_locateByFunc
 * PURPOSE:
 *      it is used to locate a node data from a linked list by a locate function.
 * INPUT:
 *      ptr_list        -- the linked list.
 *      ptr_cookie      -- cookie data for locate function.
 *      locate_callback -- locate function for location.
 * OUTPUT:
 *      pptr_node       -- the located node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the locate data is not in the linked list
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_locateByFunc(
    CMLIB_LIST_T                   *ptr_list,
    void                           *ptr_cookie,
    const CMLIB_LIST_LOCATE_FUNC_T locate_callback,
    CMLIB_LIST_NODE_T              **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_cookie);
    HAL_CHECK_PTR(locate_callback);
    HAL_CHECK_PTR(pptr_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->locateByFunc))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->locateByFunc(ptr_list,
                ptr_cookie, locate_callback, pptr_node);

}   /* End of cmlib_list_locateByFunc */

/* FUNCTION NAME: cmlib_list_locateHead
 * PURPOSE:
 *      it is used to get the first node of the linked list.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      pptr_node -- the head data node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no data node, it means list is empty
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_locateHead(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(pptr_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->locateHead))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->locateHead(ptr_list, pptr_node);

}   /* End of cmlib_list_locateHead */

/* FUNCTION NAME: cmlib_list_locateTail
 * PURPOSE:
 *      it is used to get the last node of the linked list.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      pptr_node -- the tail node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no data node, it means list is empty
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_locateTail(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(pptr_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->locateTail))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->locateTail(ptr_list, pptr_node);

}   /* End of cmlib_list_locateTail */

/* FUNCTION NAME: cmlib_list_next
 * PURPOSE:
 *      it is used to get the next node of a specified node.
 * INPUT:
 *      ptr_list  -- the linked list.
 *      ptr_node  -- the specified node.
 * OUTPUT:
 *      pptr_next_node -- the next node.
 * RETURN:
 *      CLX_E_OK              -- get next node success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no next node
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_next(
    CMLIB_LIST_T      *ptr_list,
    CMLIB_LIST_NODE_T *ptr_node,
    CMLIB_LIST_NODE_T **pptr_next_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_next_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->next))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->next(ptr_list, ptr_node, pptr_next_node);

}   /* End of cmlib_list_next */

/* FUNCTION NAME: cmlib_list_prev
 * PURPOSE:
 *      it is used to get the previous node of a specified node.
 * INPUT:
 *      ptr_list  -- the linked list.
 *      ptr_node  -- the specified node.
 * OUTPUT:
 *      pptr_prev_node -- the previous node.
 * RETURN:
 *      CLX_E_OK              -- get previous node success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no previous node
 *      CLX_E_UNSUPPORT       -- list don't support this operation
 * NOTES:
 *      singly & circular linked list don't support this operation.
 */
CLX_ERROR_NO_T
cmlib_list_prev(
    CMLIB_LIST_T       *ptr_list,
    CMLIB_LIST_NODE_T  *ptr_node,
    CMLIB_LIST_NODE_T  **pptr_prev_node)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_prev_node);

    if((ptr_list->ptr_ops == NULL) || (NULL == ptr_list->ptr_ops->prev))
    {
        return CLX_E_NOT_SUPPORT;
    }

    return ptr_list->ptr_ops->prev(ptr_list, ptr_node, pptr_prev_node);

}   /* End of cmlib_list_prev */

/* FUNCTION NAME: cmlib_list_getLength
 * PURPOSE:
 *      it is used to get the count of nodes in the linked list.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      ptr_length -- the number of nodes in the linked list.
 * RETURN:
 *      CLX_E_OK             -- destroy success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 * NOTES:
 *
 */
CLX_ERROR_NO_T
cmlib_list_getLength(
    CMLIB_LIST_T *ptr_list,
    UI32_T       *ptr_length )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_length);

    *ptr_length = ptr_list->node_count;

    return CLX_E_OK;
}   /* End of cmlib_list_getLength */

/* LOCAL SUBPROGRAM BODIES
 */

/* FUNCTION NAME: _cmlib_list_freeNode
 * PURPOSE:
 *      it is used to free a linked list node.
 * INPUT:
 *      ptr_list  -- the linked list.
 *      ptr_snode -- the linked list node
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_BAD_PARAMETER -- null pointer.
 *      CLX_E_OK            -- free node success.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_freeNode(
    CMLIB_LIST_T      *ptr_list,
    CMLIB_LIST_NODE_T *ptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);

    if(ptr_list->capacity == 0)
    {
        osal_free((void*)ptr_node);
        return CLX_E_OK;
    }
    if(CLX_E_OK != cmlib_mpool_free((CMLIB_MPOOL_T *)ptr_list->ptr_node_pool, (void*)ptr_node))
    {
        return CLX_E_OTHERS;
    }

    return CLX_E_OK;
}   /* End of _cmlib_list_freeNode */

/* FUNCTION NAME: _cmlib_list_allocNode
 * PURPOSE:
 *      it is used to allocate a linked list node.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      None.
 * RETURN:
 *      NULL     -- no allocated node.
 *      non NULL -- the allocated node.
 * NOTES:
 *
 */
static CMLIB_LIST_NODE_T *
_cmlib_list_allocNode(
    CMLIB_LIST_T       *ptr_list)
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_tmp = NULL;

    /* BODY */
    if(NULL == ptr_list)
    {
        return NULL;
    }

    if(ptr_list->capacity == 0)
    {
        ptr_tmp = (CMLIB_LIST_NODE_T *)osal_alloc(sizeof(CMLIB_LIST_NODE_T));
        return ptr_tmp;
    }

    ptr_tmp = (CMLIB_LIST_NODE_T *)cmlib_mpool_alloc((CMLIB_MPOOL_T *)ptr_list->ptr_node_pool);

    return ptr_tmp;
}   /* End of _cmlib_list_allocNode */

/* FUNCTION NAME: _cmlib_list_getNodeData
 * PURPOSE:
 *      it is used to get linked list node data which user saved it in
 *  the node.
 * INPUT:
 *      ptr_list  -- the linked list
 *      ptr_node  -- the data will get from this node.
 * OUTPUT:
 *      pptr_node_data  -- the node data return to user.
 * RETURN:
 *      CLX_E_BAD_PARAMETER -- parameter is NULL pointer.
 *      CLX_E_OK            -- get data success.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_getNodeData(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                **pptr_node_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_node_data);

    *pptr_node_data = ptr_node->ptr_data;

    return CLX_E_OK;
}   /* End of _cmlib_list_getNodeData */


/* FUNCTION NAME: _cmlib_list_insertByFunc
 * PURPOSE:
 *      it is used to insert a data by a insert function which is used to decide
 *      where the inserted node should be inserted. if the insert function does
 *      not give a position(node), the node will be inserted to the tail of the
 *      linked list. if the insert function give a position(node), the node will
 *      be inserted before the position(node).
 * INPUT:
 *      ptr_list        -- the node will be inseted in it.
 *      ptr_data        -- the inserted data.
 *      cmp_callback    -- the inserted callback function, used to find the
 *                         insert position.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_insertByFunc(
    CMLIB_LIST_T          *ptr_list,
    void                  *ptr_data,
    CMLIB_LIST_CMP_FUNC_T cmp_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CLX_ERROR_NO_T rc = CLX_E_OK;
    CMLIB_LIST_NODE_T **pptr_node = NULL, *ptr_node = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(cmp_callback);

    pptr_node = &ptr_list->ptr_head_node;
    while(*pptr_node != NULL)
    {
        if(cmp_callback((*pptr_node)->ptr_data, ptr_data) == 0)
        {
            break;
        }
        pptr_node = &(*pptr_node)->ptr_next;
    }

    ptr_node = _cmlib_list_allocNode(ptr_list);
    if(ptr_node == NULL)
    {
        return ptr_list->capacity ? CLX_E_TABLE_FULL : CLX_E_NO_MEMORY;
    }
    ptr_node->ptr_data = ptr_data;
    ptr_node->ptr_next = *pptr_node;
    ptr_node->ptr_prev = NULL;

    /* insert to tail, modify tail pointer */
    switch (ptr_list->type)
    {
        case CMLIB_LIST_TYPE_SINGLY:
            if(*pptr_node == NULL)
            {
                ptr_list->ptr_tail_node = ptr_node;
            }
            break;
        case CMLIB_LIST_TYPE_DOUBLY:
            if(*pptr_node == NULL)
            {
                ptr_node->ptr_prev = ptr_list->ptr_tail_node;
                ptr_list->ptr_tail_node = ptr_node;
            }

            if(ptr_node->ptr_next != NULL)
            {
                ptr_node->ptr_prev = ptr_node->ptr_next->ptr_prev;
                ptr_node->ptr_next->ptr_prev = ptr_node;
            }
            break;
        default:
            rc = CLX_E_OTHERS;
            break;
    }

    if (CLX_E_OK == rc)
    {
        *pptr_node = ptr_node;
        ptr_list->node_count++;
    }
    else
    {
        _cmlib_list_freeNode(ptr_list, ptr_node);
    }

    return rc;
}   /* End of _cmlib_list_insertByFunc */

/* FUNCTION NAME: _cmlib_list_insertToHead
 * PURPOSE:
 *      it is used to insert a data to the front of linked list.
 * INPUT:
 *      ptr_list  -- the node will be inserted in it.
 *      ptr_data  -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_insertToHead(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T **pptr_node = NULL, *ptr_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    pptr_node = &ptr_list->ptr_head_node;

    ptr_node = _cmlib_list_allocNode(ptr_list);
    if(ptr_node == NULL)
    {
        return ptr_list->capacity ? CLX_E_TABLE_FULL : CLX_E_NO_MEMORY;
    }
    ptr_node->ptr_data = ptr_data;
    ptr_node->ptr_next = *pptr_node;
    ptr_node->ptr_prev = NULL;

    /* if front pointer is NULL, modify tail pointer too */
    if(*pptr_node == NULL)
    {
        ptr_list->ptr_tail_node = ptr_node;
    }

    if((CMLIB_LIST_TYPE_DOUBLY == ptr_list->type) && (ptr_node->ptr_next != NULL))
    {
        /*ptr_node->ptr_prev = ptr_dnode->ptr_next->ptr_prev;*/
        ptr_node->ptr_next->ptr_prev = ptr_node;
    }

    *pptr_node = ptr_node;
    ptr_list->node_count++;

    return CLX_E_OK;
}   /* End of _cmlib_list_insertToHead */

/* FUNCTION NAME: _cmlib_list_insertToTail
 * PURPOSE:
 *      it is used to insert a data to the tail of linked list.
 * INPUT:
 *      ptr_list  -- the node will be inserted in it.
 *      ptr_data  -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_insertToTail(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_node = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    ptr_node = _cmlib_list_allocNode(ptr_list);
    if(ptr_node == NULL)
    {
        return ptr_list->capacity ? CLX_E_TABLE_FULL : CLX_E_NO_MEMORY;
    }
    ptr_node->ptr_data = ptr_data;
    ptr_node->ptr_next = NULL;

    if (CMLIB_LIST_TYPE_DOUBLY == ptr_list->type)
    {
        ptr_node->ptr_prev = ptr_list->ptr_tail_node;
    }

    /* if tail pointer is NULL, modify front pointer too */
    if(ptr_list->ptr_tail_node == NULL)
    {
        ptr_list->ptr_head_node= ptr_node;
    }
    else
    {
        ptr_list->ptr_tail_node->ptr_next = ptr_node;
    }

    ptr_list->ptr_tail_node = ptr_node;

    ptr_list->node_count++;

    return CLX_E_OK;
}   /* End of _cmlib_list_insertToTail */

/* FUNCTION NAME: _cmlib_list_insertAfter
 * PURPOSE:
 *      it is used to insert a data after a specified node.
 * INPUT:
 *      ptr_list -- the node will be inserted in it.
 *      ptr_node -- insert after it.
 *      ptr_data -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK            -- insert success.
 *      CLX_E_BAD_PARAMETER -- parameter pointer is null.
 *      CLX_E_TABLE_FULL    -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY     -- allocate node failed, this is for capacity==0.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_insertAfter(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_new_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(ptr_data);

    ptr_new_node = _cmlib_list_allocNode(ptr_list);
    if(ptr_new_node == NULL)
    {
        return ptr_list->capacity ? CLX_E_TABLE_FULL : CLX_E_NO_MEMORY;
    }
    ptr_new_node->ptr_data = ptr_data;
    ptr_new_node->ptr_next = ptr_node->ptr_next;
    ptr_node->ptr_next = ptr_new_node;
    if(ptr_node == ptr_list->ptr_tail_node)
    {
        ptr_list->ptr_tail_node = ptr_new_node;
    }

    if (CMLIB_LIST_TYPE_DOUBLY == ptr_list->type)
    {
        ptr_new_node->ptr_prev = ptr_node;
        if(ptr_new_node->ptr_next != NULL)
        {
            ptr_new_node->ptr_next->ptr_prev = ptr_new_node;
        }
    }

    ptr_list->node_count++;

    return CLX_E_OK;
}   /* End of _cmlib_list_insertAfter */

/* FUNCTION NAME: _cmlib_list_insertBefore
 * PURPOSE:
 *      it is used to insert a data before a specified node.
 * INPUT:
 *      ptr_list -- the node will be inserted in it.
 *      ptr_node -- insert before it.
 *      ptr_data -- the inserted data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- insert success.
 *      CLX_E_BAD_PARAMETER  -- parameter pointer is null.
 *      CLX_E_TABLE_FULL     -- the linked list is full, this is for capacity>0
 *      CLX_E_NO_MEMORY      -- allocate node failed, this is for capacity==0.
 *      CLX_E_NOT_SUPPORT    -- list don't support this operation
 * NOTES:
 *      singly linked list don't support this operation.
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_insertBefore(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    void                *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_new_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(ptr_data);

    if (CMLIB_LIST_TYPE_DOUBLY != ptr_list->type)
    {
        return CLX_E_NOT_SUPPORT;
    }

    ptr_new_node = _cmlib_list_allocNode(ptr_list);
    if(ptr_new_node == NULL)
    {
        return ptr_list->capacity ? CLX_E_TABLE_FULL : CLX_E_NO_MEMORY;
    }
    ptr_new_node->ptr_data = ptr_data;
    ptr_new_node->ptr_next = ptr_node;
    ptr_new_node->ptr_prev = ptr_new_node->ptr_next->ptr_prev;
    ptr_node->ptr_prev = ptr_new_node;
    if(ptr_new_node->ptr_prev != NULL)
    {
        ptr_new_node->ptr_prev->ptr_next = ptr_new_node;
    }

    if(ptr_node == ptr_list->ptr_head_node)
    {
        ptr_list->ptr_head_node= ptr_new_node;
    }

    ptr_list->node_count++;

    return CLX_E_OK;
}   /* End of _cmlib_list_doubly_insertBefore */

/* FUNCTION NAME: _cmlib_list_delete
 * PURPOSE:
 *      it is used to delete a node from a linked list.
 * INPUT:
 *      ptr_list         -- the node will delete from it.
 *      ptr_delete_node  -- the deleted node pointer.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the delete node is not in the linked list
 * NOTES:
 *      the delete node data will not process in this function
 */
static CLX_ERROR_NO_T
_cmlib_list_delete(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T **pptr_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);

    pptr_node = &ptr_list->ptr_head_node;

    while(*pptr_node != NULL)
    {
        if(*pptr_node == ptr_node)
        {
            *pptr_node = ptr_node->ptr_next;

            switch (ptr_list->type)
            {
                case CMLIB_LIST_TYPE_SINGLY:
                    if(ptr_list->ptr_head_node == NULL)
                    {
                        ptr_list->ptr_tail_node = NULL;
                    }
                    else
                    {
                        if(ptr_node == ptr_list->ptr_tail_node)
                        {
                            ptr_list->ptr_tail_node =
                                _CMLIB_LIST_CONTAINER_OF(pptr_node, CMLIB_LIST_NODE_T, ptr_next);
                        }
                    }
                    break;

                case CMLIB_LIST_TYPE_DOUBLY:
                    *pptr_node = ptr_node->ptr_next;
                    if(*pptr_node != NULL)
                    {
                        (*pptr_node)->ptr_prev = ptr_node->ptr_prev;
                    }
                    else
                    {
                        ptr_list->ptr_tail_node = ptr_node->ptr_prev;
                    }
                    break;
                default:
                    return CLX_E_OTHERS;
                    break;
            }

            _cmlib_list_freeNode(ptr_list, ptr_node);

            ptr_list->node_count--;

            return CLX_E_OK;
        }
        pptr_node = &((*pptr_node)->ptr_next);
    }

    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of _cmlib_list_delete */

/* FUNCTION NAME: _cmlib_list_deleteByData
 * PURPOSE:
 *      it is used to delete a node from a linked list by user data
 *  pointer.
 * INPUT:
 *      ptr_list         -- the node will delete from it.
 *      ptr_delete_data  -- the deleted user data pointer.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK              -- delete success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the delete node is not in the linked list
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_deleteByData(
    CMLIB_LIST_T *ptr_list,
    void         *ptr_data )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_node = NULL, **pptr_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_data);

    pptr_node = &ptr_list->ptr_head_node;

    while(*pptr_node != NULL)
    {
        if((*pptr_node)->ptr_data == ptr_data)
        {
            ptr_node = *pptr_node;
            *pptr_node = ptr_node->ptr_next;

            switch (ptr_list->type)
            {
                case CMLIB_LIST_TYPE_SINGLY:
                    if(ptr_list->ptr_head_node == NULL)
                    {
                        ptr_list->ptr_tail_node = NULL;
                    }
                    else
                    {
                        if(ptr_node == ptr_list->ptr_tail_node)
                        {
                            ptr_list->ptr_tail_node =
                                _CMLIB_LIST_CONTAINER_OF(pptr_node, CMLIB_LIST_NODE_T, ptr_next);
                        }
                    }
                    break;
                case CMLIB_LIST_TYPE_DOUBLY:
                    if(*pptr_node != NULL)
                    {
                        (*pptr_node)->ptr_prev = ptr_node->ptr_prev;
                    }
                    else
                    {
                        ptr_list->ptr_tail_node = ptr_node->ptr_prev;
                    }
                    break;
                default:
                    return CLX_E_OTHERS;
                    break;
            }
            _cmlib_list_freeNode(ptr_list, ptr_node);

            ptr_list->node_count--;

            return CLX_E_OK;
        }
        pptr_node = &((*pptr_node)->ptr_next);
    }

    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of _cmlib_list_deleteByData */

/* FUNCTION NAME: _cmlib_list_locateByFunc
 * PURPOSE:
 *      it is used to locate a node data from a linked list by a locate
 *  function.
 * INPUT:
 *      ptr_list        -- the linked list.
 *      ptr_cookie      -- cookie data for locate function.
 *      locate_callback -- locate function for location.
 * OUTPUT:
 *      pptr_node       -- the located node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- the locate data is not in the linked list
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_locateByFunc(
    CMLIB_LIST_T                   *ptr_list,
    void                           *ptr_cookie,
    const CMLIB_LIST_LOCATE_FUNC_T locate_callback,
    CMLIB_LIST_NODE_T              **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_node = NULL;

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_cookie);
    HAL_CHECK_PTR(locate_callback);
    HAL_CHECK_PTR(pptr_node);

    ptr_node = ptr_list->ptr_head_node;
    while(ptr_node != NULL)
    {
        if(locate_callback(ptr_node->ptr_data, ptr_cookie) == 0)
        {
            *pptr_node = ptr_node;
            return CLX_E_OK;
        }
        ptr_node = ptr_node->ptr_next;
    }

    *pptr_node = NULL;
    return CLX_E_ENTRY_NOT_FOUND;
}   /* End of _cmlib_list_locateByFunc */


/* FUNCTION NAME: _cmlib_list_locateHead
 * PURPOSE:
 *      it is used to get the first node of the linked list.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      pptr_node -- the head data node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no data node, it means list is empty
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_locateHead(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(pptr_node);

    if(ptr_list->ptr_head_node == NULL)
    {
        *pptr_node = NULL;
        return CLX_E_ENTRY_NOT_FOUND;
    }

    *pptr_node = ptr_list->ptr_head_node;

    return CLX_E_OK;
}   /* End of _cmlib_list_locateHead */

/* FUNCTION NAME: _cmlib_list_locateTail
 * PURPOSE:
 *      it is used to get the last node of the linked list.
 * INPUT:
 *      ptr_list  -- the linked list.
 * OUTPUT:
 *      pptr_node -- the tail node.
 * RETURN:
 *      CLX_E_OK              -- locate success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no data node, it means list is empty
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_locateTail(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   **pptr_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(pptr_node);

    if(ptr_list->ptr_tail_node == NULL)
    {
        *pptr_node = NULL;
        return CLX_E_ENTRY_NOT_FOUND;
    }

    *pptr_node = ptr_list->ptr_tail_node;

    return CLX_E_OK;
}   /* End of _cmlib_list_locateTail */

/* FUNCTION NAME: _cmlib_list_next
 * PURPOSE:
 *      it is used to get the next node of a specified node.
 * INPUT:
 *      ptr_list  -- the singly linked list.
 *      ptr_node  -- the specified node.
 * OUTPUT:
 *      pptr_next_node -- the next node.
 * RETURN:
 *      CLX_E_OK              -- get next node success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_ENTRY_NOT_FOUND -- no next node
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_next(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    CMLIB_LIST_NODE_T   **pptr_next_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_next_node);

    if(ptr_node->ptr_next == NULL)
    {
        *pptr_next_node = NULL;
        return CLX_E_ENTRY_NOT_FOUND;
    }

    *pptr_next_node = ptr_node->ptr_next;

    return CLX_E_OK;
}   /* End of _cmlib_list_next */

/* FUNCTION NAME: _cmlib_list_prev
 * PURPOSE:
 *      it is used to get the previous node of a specified node.
 * INPUT:
 *      ptr_list  -- the linked list.
 *      ptr_node  -- the specified node.
 * OUTPUT:
 *      pptr_prev_node -- the previous node.
 * RETURN:
 *      CLX_E_OK              -- get previous node success.
 *      CLX_E_BAD_PARAMETER   -- parameter pointer is null.
 *      CLX_E_NOT_SUPPORT    -- list don't support this operation
 * NOTES:
 *      singly linked list don't support this operation.
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_prev(
    CMLIB_LIST_T        *ptr_list,
    CMLIB_LIST_NODE_T   *ptr_node,
    CMLIB_LIST_NODE_T   **pptr_prev_node )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */

    /* BODY */
    HAL_CHECK_PTR(ptr_list);
    HAL_CHECK_PTR(ptr_node);
    HAL_CHECK_PTR(pptr_prev_node);

    if (CMLIB_LIST_TYPE_DOUBLY != ptr_list->type)
    {
        return CLX_E_NOT_SUPPORT;
    }

    if(ptr_node->ptr_prev == NULL)
    {
        *pptr_prev_node = NULL;
        return CLX_E_ENTRY_NOT_FOUND;
    }

    *pptr_prev_node = ptr_node->ptr_prev;

    return CLX_E_OK;
}   /* End of _cmlib_list_doubly_prev */

/* FUNCTION NAME: _cmlib_list_destroy
 * PURPOSE:
 *      it is used to destroy a linked list and release head and nodes.
 *      if user provide a destroy function, it will release every node data in
 *      the singly linked list.
 * INPUT:
 *      ptr_list         -- the destroyed singly linked list.
 *      destroy_callback -- destroy function is used to destroy node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- destroy success.
 *      CLX_E_BAD_PARAMETER  -- ptr_list is null.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_destroy(
    CMLIB_LIST_T              *ptr_list,
    CMLIB_LIST_DESTROY_FUNC_T destroy_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CLX_ERROR_NO_T ret = CLX_E_OK;
    CMLIB_LIST_NODE_T *ptr_node = NULL, *ptr_tmp = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_list);

    if(destroy_callback != NULL || ptr_list->capacity == 0)
    {
        ptr_node = ptr_list->ptr_head_node;
        while(ptr_node != NULL)
        {
            if(ptr_node->ptr_data != NULL && destroy_callback != NULL)
            {
                destroy_callback(ptr_node->ptr_data);
            }
            ptr_tmp = ptr_node->ptr_next;
            if(ptr_list->capacity == 0)
            {
                _cmlib_list_freeNode(ptr_list, ptr_node);
            }
            ptr_node = ptr_tmp;
        }
    }

    if(ptr_list->capacity != 0)
    {
        ret = cmlib_mpool_destroy((CMLIB_MPOOL_T *)ptr_list->ptr_node_pool, NULL);
        if(CLX_E_OK != ret)
        {
            DIAG_PRINT(HAL_DBG_WARN, "invoke cmlib_mpool_destroy failed, rc=%d\n", ret);
        }
    }

    osal_free(ptr_list);

    return CLX_E_OK;
}   /* End of _cmlib_list_destroy */

/* FUNCTION NAME: _cmlib_list_deleteAll
 * PURPOSE:
 *      it is used to destroy all nodes from a linked list.
 *      if user provide a destroy function, it will release every node data in
 *      the doubly linked list.
 * INPUT:
 *      ptr_list         -- the singly linked list.
 *      delete_callback  -- delete function is used to destroy node data.
 * OUTPUT:
 *      None.
 * RETURN:
 *      CLX_E_OK             -- destroy all nodes success.
 *      CLX_E_BAD_PARAMETER  -- ptr_list is null.
 * NOTES:
 *
 */
static CLX_ERROR_NO_T
_cmlib_list_deleteAll(
    CMLIB_LIST_T              *ptr_list,
    CMLIB_LIST_DELETE_FUNC_T delete_callback )
{
    /* LOCAL CONSTANT DECLARATIONS
     */
    /* LOCAL VARIABLES DECLARATIONS
     */
    CMLIB_LIST_NODE_T *ptr_node = NULL, *ptr_tmp = NULL;
    /* BODY */
    HAL_CHECK_PTR(ptr_list);

    ptr_node = ptr_list->ptr_head_node;
    while(NULL != ptr_node)
    {
        if(NULL != delete_callback)
        {
            delete_callback(ptr_node->ptr_data);
        }
        ptr_tmp = ptr_node->ptr_next;
        _cmlib_list_freeNode(ptr_list, ptr_node);
        ptr_node = ptr_tmp;
    }

    ptr_list->node_count = 0;
    ptr_list->ptr_head_node = NULL;
    ptr_list->ptr_tail_node = NULL;
    return CLX_E_OK;
}

