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

/* FILE NAME:  cmlib_rbt.c
 * PURPOSE:
 * this file is used to provide red black tree operations to other users.
 * NOTES:
 *
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_rbt.h>
#include <osal/osal.h>

/* NAMING CONSTANT DECLARATIONS
 */
#define CMLIB_RBT_MAX_NODE_COUNT    (0xFFFFFFFF)    /* the maximum value of node_count in rbt_head */
#define CMLIB_RBT_MAX_HEIGHT        (2 * 32)        /* the max height would be bound by 2*log(n+1) */

/* MACRO FUNCTION DECLARATIONS
 */
#define CMLIB_RBT_ALLOC_NODE(__ptr_head__, __ptr_node__) \
    do { \
        if (0 == (__ptr_head__)->capacity) \
        { \
            (__ptr_node__) = osal_alloc(sizeof(CMLIB_RBT_NODE_T)); \
        } \
        else \
        { \
            (__ptr_node__) = cmlib_mpool_alloc((__ptr_head__)->ptr_node_pool); \
        } \
    } while (0)
#define CMLIB_RBT_FREE_NODE(__ptr_head__, __ptr_node__) \
    do { \
        if (0 == (__ptr_head__)->capacity) \
        { \
            osal_free((__ptr_node__)); \
        } \
        else \
        { \
            cmlib_mpool_free((__ptr_head__)->ptr_node_pool, (__ptr_node__)); \
        } \
    } while (0)

/* DATA TYPE DECLARATIONS
 */

/* GLOBAL VARIABLE DECLARATIONS
 */
//DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_rbt.c");

/* LOCAL SUBPROGRAM DECLARATIONS
 */
static CMLIB_RBT_NODE_T *
_cmlib_rbt_rotateRight(
    CMLIB_RBT_HEAD_T    *ptr_head,
    CMLIB_RBT_NODE_T    *ptr_root);

static CMLIB_RBT_NODE_T *
_cmlib_rbt_rotateLeft(
    CMLIB_RBT_HEAD_T    *ptr_head,
    CMLIB_RBT_NODE_T    *ptr_root);

/* EXPORTED SUBPROGRAM BODIES
 */
CLX_ERROR_NO_T
cmlib_rbt_create(
    void                        *ptr_user_param,
    const UI32_T                capacity,
    const CMLIB_RBT_CMP_FUNC_T  cmp_func,
    const C8_T                  *ptr_name,
    CMLIB_RBT_HEAD_T            **pptr_head)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    CMLIB_RBT_HEAD_T    *ptr_head;

    HAL_CHECK_PTR(cmp_func);
    HAL_CHECK_PTR(ptr_name);
    HAL_CHECK_PTR(pptr_head);

    ptr_head = osal_alloc(sizeof(CMLIB_RBT_HEAD_T));
    if (NULL == ptr_head)
    {
        return CLX_E_NO_MEMORY;
    }
    osal_memset(ptr_head, 0x0, sizeof(CMLIB_RBT_HEAD_T));
    ptr_head->ptr_name = (C8_T *) ptr_name;
    ptr_head->cmp_func = cmp_func;
    ptr_head->ptr_user_param = ptr_user_param;
    ptr_head->capacity = capacity;
    ptr_head->nil_node.color = CMLIB_RBT_COLOR_TYPE_BLACK;
    ptr_head->ptr_root = &(ptr_head->nil_node);

    if (capacity > 0)
    {
        rc = cmlib_mpool_create(sizeof(CMLIB_RBT_NODE_T),
                                capacity,
                                NULL,
                                ptr_name,
                                &(ptr_head->ptr_node_pool));
    }

    if (CLX_E_OK == rc)
    {
        *pptr_head = ptr_head;
    }
    else
    {
        osal_free(ptr_head);
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_destroy(
    CMLIB_RBT_HEAD_T                *ptr_head,
    const CMLIB_RBT_DESTROY_FUNC_T  destroy_func)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    CMLIB_RBT_NODE_T    *ptr_root;
    void                *ptr_user_param;
    CMLIB_RBT_NODE_T    *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    ptr_nil_node = &(ptr_head->nil_node);
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_root = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_root)
    {
        if (ptr_nil_node != ptr_root->ptr_left)
        {
            ptr_root = _cmlib_rbt_rotateRight(ptr_head, ptr_root);
        }
        else
        {
            ptr_head->ptr_root = ptr_root->ptr_right;
            ptr_head->ptr_root->ptr_parent = ptr_nil_node;

            if (NULL != destroy_func)
            {
                destroy_func(ptr_user_param, ptr_root->ptr_data);
            }
            CMLIB_RBT_FREE_NODE(ptr_head, ptr_root);
            ptr_root = ptr_head->ptr_root;
        }
    }

    if (ptr_head->capacity > 0)
    {
        rc = cmlib_mpool_destroy(ptr_head->ptr_node_pool, NULL);
    }

    if (CLX_E_OK == rc)
    {
        osal_free(ptr_head);
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_insert(
    CMLIB_RBT_HEAD_T    *ptr_head,
    void                *ptr_data,
    const BOOL_T        overwrite_flag,
    void                **pptr_overwritten_data,
    void                **pptr_insert_node)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_RBT_NODE_T        *ptr_node;
    CMLIB_RBT_NODE_T        **pptr_node;
    CMLIB_RBT_NODE_T        *ptr_parent;
    I32_T                   ret;
    CMLIB_RBT_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;
    CMLIB_RBT_NODE_T        *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    if ((TRUE == overwrite_flag) &&
        (NULL == pptr_overwritten_data))
    {
        return CLX_E_BAD_PARAMETER;
    }
    else if (CMLIB_RBT_MAX_NODE_COUNT == ptr_head->node_count)
    {
        return CLX_E_TABLE_FULL;
    }
    ptr_nil_node = &(ptr_head->nil_node);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    pptr_node = &(ptr_head->ptr_root);
    ptr_node = *pptr_node;
    ptr_parent = ptr_node;
    while (ptr_nil_node != ptr_node)
    {
        ret = cmp_func(ptr_user_param, ptr_data, ptr_node->ptr_data);
        if (ret < 0)
        {
            pptr_node = &(ptr_node->ptr_left);
        }
        else if (ret > 0)
        {
            pptr_node = &(ptr_node->ptr_right);
        }
        else
        {
            if (TRUE == overwrite_flag)
            {
                *pptr_overwritten_data = ptr_node->ptr_data;
                ptr_node->ptr_data = ptr_data;
                if (NULL != pptr_insert_node)
                {
                    *pptr_insert_node = ptr_node;
                }
            }
            else
            {
                rc = CLX_E_ENTRY_EXISTS;
            }
            return rc;
        }

        ptr_parent = ptr_node;
        ptr_node = *pptr_node;
    }

    CMLIB_RBT_ALLOC_NODE(ptr_head, ptr_node);
    if (NULL != ptr_node)
    {
        osal_memset(ptr_node, 0x0, sizeof(CMLIB_RBT_NODE_T));
        ptr_node->ptr_parent = ptr_parent;
        ptr_node->ptr_left = ptr_nil_node;
        ptr_node->ptr_right = ptr_nil_node;
        ptr_node->color = CMLIB_RBT_COLOR_TYPE_RED;
        ptr_node->ptr_data = ptr_data;

        *pptr_node = ptr_node;
        if (NULL != pptr_insert_node)
        {
            *pptr_insert_node = ptr_node;
        }
    }
    else
    {
        if (0 == ptr_head->capacity)
        {
            rc = CLX_E_NO_MEMORY;
        }
        else
        {
            rc = CLX_E_TABLE_FULL;
        }
    }

    if (CLX_E_OK == rc)
    {
        CMLIB_RBT_NODE_T    *ptr_grandpa;
        CMLIB_RBT_NODE_T    *ptr_uncle;

        /* to resolve the issue for adjacent nodes being red */
        while ((ptr_head->ptr_root != ptr_node) &&
               (CMLIB_RBT_COLOR_TYPE_RED == ptr_node->ptr_parent->color))
        {
            ptr_parent = ptr_node->ptr_parent;
            ptr_grandpa = ptr_parent->ptr_parent;
            ptr_uncle = (ptr_parent != ptr_grandpa->ptr_left) ? ptr_grandpa->ptr_left :
                                                                ptr_grandpa->ptr_right;

            if (CMLIB_RBT_COLOR_TYPE_RED == ptr_uncle->color)
            {
                /* the color of parent and uncle are red(i.e. the grandpa must be black),
                   then change the grandpa's color to red and recur the flow
                */
                ptr_grandpa->color = CMLIB_RBT_COLOR_TYPE_RED;
                ptr_uncle->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                ptr_parent->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                ptr_node = ptr_grandpa;
            }
            else
            {
                /* rotate the tree to let two adjacent nodes as siblings,
                   new grandpa would be black and new uncle would be red
                 */
                ptr_grandpa->color = CMLIB_RBT_COLOR_TYPE_RED;
                if (ptr_parent == ptr_grandpa->ptr_left)
                {
                    if (ptr_node == ptr_parent->ptr_left)
                    {
                        ptr_grandpa = _cmlib_rbt_rotateRight(ptr_head, ptr_grandpa);
                    }
                    else
                    {
                        _cmlib_rbt_rotateLeft(ptr_head, ptr_parent);
                        ptr_grandpa = _cmlib_rbt_rotateRight(ptr_head, ptr_grandpa);
                    }
                }
                else
                {
                    if (ptr_node == ptr_parent->ptr_right)
                    {
                        ptr_grandpa = _cmlib_rbt_rotateLeft(ptr_head, ptr_grandpa);
                    }
                    else
                    {
                        _cmlib_rbt_rotateRight(ptr_head, ptr_parent);
                        ptr_grandpa = _cmlib_rbt_rotateLeft(ptr_head, ptr_grandpa);
                    }
                }
                ptr_grandpa->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                break;
            }
        }

        /* root's color is always black, so recoloring flow doesn't recolor it */
        if (ptr_head->ptr_root == ptr_node)
        {
            ptr_node->color = CMLIB_RBT_COLOR_TYPE_BLACK;
        }
        ptr_head->node_count++;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_delete(
    CMLIB_RBT_HEAD_T                *ptr_head,
    void                            *ptr_data,
    void                            **pptr_node_data,
    const CMLIB_RBT_CMP_FUNC_T      dyn_cmp_func,
    const CMLIB_RBT_UPDATE_FUNC_T   update_func)
{
    CLX_ERROR_NO_T          rc;
    CMLIB_RBT_NODE_T        *ptr_node;
    I32_T                   ret;
    CMLIB_RBT_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;
    CMLIB_RBT_NODE_T        *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node_data);
    ptr_nil_node = &(ptr_head->nil_node);
    if (NULL == dyn_cmp_func)
    {
        cmp_func = ptr_head->cmp_func;
    }
    else
    {
        cmp_func = dyn_cmp_func;
    }
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_node)
    {
        ret = cmp_func(ptr_user_param, ptr_data, ptr_node->ptr_data);
        if (ret < 0)
        {
            ptr_node = ptr_node->ptr_left;
        }
        else if (ret > 0)
        {
            ptr_node = ptr_node->ptr_right;
        }
        else
        {
            break;
        }
    }

    rc = cmlib_rbt_deleteNode(ptr_head, ptr_node, pptr_node_data, update_func);

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_deleteNode(
    CMLIB_RBT_HEAD_T                *ptr_head,
    const CMLIB_RBT_NODE_T          *ptr_del_node,
    void                            **pptr_del_node_data,
    const CMLIB_RBT_UPDATE_FUNC_T   update_func)
{
    CMLIB_RBT_NODE_T    **pptr_node;
    CMLIB_RBT_NODE_T    *ptr_node;
    CMLIB_RBT_NODE_T    *ptr_parent;
    CMLIB_RBT_NODE_T    *ptr_child;
    CMLIB_RBT_NODE_T    *ptr_sibling;
    CMLIB_RBT_NODE_T    *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_del_node);
    HAL_CHECK_PTR(pptr_del_node_data);
    if ((&(ptr_head->nil_node)) == ptr_del_node)
    {
        return CLX_E_ENTRY_NOT_FOUND;
    }
    ptr_nil_node = &(ptr_head->nil_node);

    *pptr_del_node_data = ptr_del_node->ptr_data;
    if (ptr_nil_node != ptr_del_node->ptr_parent)
    {
        pptr_node = (ptr_del_node->ptr_parent->ptr_left == ptr_del_node) ? &(ptr_del_node->ptr_parent->ptr_left) :
                                                                           &(ptr_del_node->ptr_parent->ptr_right);
    }
    else
    {
        pptr_node = &(ptr_head->ptr_root);
    }
    ptr_node = (CMLIB_RBT_NODE_T *) ptr_del_node;

    /* find the node to be deleted according to different cases */
    if ((ptr_nil_node != ptr_node->ptr_left) &&
        (ptr_nil_node != ptr_node->ptr_right))
    {
        CMLIB_RBT_NODE_T    *ptr_branch_node = ptr_node;
        CMLIB_RBT_NODE_T    **pptr_leaf_node;

        pptr_node = &(ptr_branch_node->ptr_left);
        ptr_node = *pptr_node;
        while (ptr_nil_node != ptr_node)
        {
            pptr_leaf_node = pptr_node;
            pptr_node = &(ptr_node->ptr_right);
            ptr_node = *pptr_node;
        }
        pptr_node = pptr_leaf_node;
        ptr_node = *pptr_node;
        ptr_branch_node->ptr_data = ptr_node->ptr_data;
        ptr_child = ptr_node->ptr_left;

        if (NULL != update_func)
        {
            update_func(ptr_branch_node, ptr_branch_node->ptr_data);
        }
    }
    else if ((ptr_nil_node == ptr_node->ptr_left) &&
             (ptr_nil_node == ptr_node->ptr_right))
    {
        ptr_child = ptr_nil_node;
    }
    else
    {
        ptr_child = (ptr_nil_node != ptr_node->ptr_left) ? ptr_node->ptr_left : ptr_node->ptr_right;
    }
    *pptr_node = ptr_child;
    ptr_child->ptr_parent = ptr_node->ptr_parent;

    if ((CMLIB_RBT_COLOR_TYPE_BLACK == ptr_node->color) &&
        (CMLIB_RBT_COLOR_TYPE_BLACK == ptr_child->color))
    {
        ptr_child->color = CMLIB_RBT_COLOR_TYPE_DOUBLE_BLACK;
    }
    else
    {
        if (CMLIB_RBT_COLOR_TYPE_RED == ptr_child->color)
        {
            ptr_child->color = CMLIB_RBT_COLOR_TYPE_BLACK;
        }
        /* to break the following while-loop */
        ptr_child = ptr_head->ptr_root;
    }
    CMLIB_RBT_FREE_NODE(ptr_head, ptr_node);

    /* to resolve the issue for the node with color double-black */
    ptr_node = ptr_child;
    while (ptr_head->ptr_root != ptr_node)
    {
        ptr_parent = ptr_node->ptr_parent;
        ptr_sibling = (ptr_node != ptr_parent->ptr_left) ? ptr_parent->ptr_left :
                                                           ptr_parent->ptr_right;

        if (CMLIB_RBT_COLOR_TYPE_BLACK == ptr_sibling->color)
        {
            /* if the nephew is nil node, its color maybe be double-back */
            if ((CMLIB_RBT_COLOR_TYPE_RED != ptr_sibling->ptr_left->color) &&
                (CMLIB_RBT_COLOR_TYPE_RED != ptr_sibling->ptr_right->color))
            {
                ptr_sibling->color = CMLIB_RBT_COLOR_TYPE_RED;
                /* pass the double-black to its parent */
                ptr_node->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                if (CMLIB_RBT_COLOR_TYPE_BLACK == ptr_parent->color)
                {
                    ptr_parent->color = CMLIB_RBT_COLOR_TYPE_DOUBLE_BLACK;
                    ptr_node = ptr_parent;
                }
                else
                {
                    /* red + double-black = single black */
                    ptr_parent->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                    break;
                }
            }
            else
            {
                /* 1. one of the sibling's children is red, use red nephew as new sibling to maintain the black depth.
                   2. sibling would be the new sub root to add one more level compensating the double-black.
                 */
                if (ptr_sibling == ptr_parent->ptr_right)
                {
                    if (CMLIB_RBT_COLOR_TYPE_RED == ptr_sibling->ptr_right->color)
                    {
                        ptr_sibling->ptr_right->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                        _cmlib_rbt_rotateLeft(ptr_head, ptr_parent);
                    }
                    else
                    {
                        ptr_sibling->ptr_left->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                        ptr_sibling = _cmlib_rbt_rotateRight(ptr_head, ptr_sibling);
                        _cmlib_rbt_rotateLeft(ptr_head, ptr_parent);
                    }
                }
                else
                {
                    if (CMLIB_RBT_COLOR_TYPE_RED == ptr_sibling->ptr_left->color)
                    {
                        ptr_sibling->ptr_left->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                        _cmlib_rbt_rotateRight(ptr_head, ptr_parent);
                    }
                    else
                    {
                        ptr_sibling->ptr_right->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                        ptr_sibling = _cmlib_rbt_rotateLeft(ptr_head, ptr_sibling);
                        _cmlib_rbt_rotateRight(ptr_head, ptr_parent);
                    }
                }
                ptr_sibling->color = ptr_parent->color;
                ptr_parent->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                ptr_node->color = CMLIB_RBT_COLOR_TYPE_BLACK;
                break;
            }
        }
        else
        {
            /* 1. if sibling's color is red and my color is double-black, sibling must has black children.
               2. rotate the sibling as new sub root and select its child as new sibling.
               3. the case would be that my color is double-black and sibling's color is black.
             */
            if (ptr_sibling == ptr_parent->ptr_right)
            {
                _cmlib_rbt_rotateLeft(ptr_head, ptr_parent);
            }
            else
            {
                _cmlib_rbt_rotateRight(ptr_head, ptr_parent);
            }
            ptr_parent->color = CMLIB_RBT_COLOR_TYPE_RED;
            ptr_sibling->color = CMLIB_RBT_COLOR_TYPE_BLACK;
        }
    }

    if (ptr_head->ptr_root == ptr_node)
    {
        ptr_node->color = CMLIB_RBT_COLOR_TYPE_BLACK;
    }
    ptr_head->node_count--;

    return CLX_E_OK;
}

CMLIB_RBT_NODE_T *
cmlib_rbt_getFirst(
    const CMLIB_RBT_HEAD_T  *ptr_head)
{
    CMLIB_RBT_NODE_T        *ptr_node;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    if (NULL == ptr_head)
    {
        return NULL;
    }
    ptr_nil_node = &(ptr_head->nil_node);

    ptr_node = ptr_head->ptr_root;
    if (ptr_nil_node != ptr_node)
    {
        while (ptr_nil_node != ptr_node->ptr_left)
        {
            ptr_node = ptr_node->ptr_left;
        }
    }
    else
    {
        ptr_node = NULL;
    }

    return ptr_node;
}

CMLIB_RBT_NODE_T *
cmlib_rbt_getNext(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    const CMLIB_RBT_NODE_T  *ptr_node)
{
    CMLIB_RBT_NODE_T        *ptr_next_node;
    CMLIB_RBT_NODE_T        *ptr_parent;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    if ((NULL == ptr_head) ||
        (NULL == ptr_node))
    {
        return NULL;
    }
    else if ((&(ptr_head->nil_node)) == ptr_node)
    {
        return NULL;
    }
    ptr_nil_node = &(ptr_head->nil_node);

    if (ptr_nil_node != ptr_node->ptr_right)
    {
        ptr_next_node = ptr_node->ptr_right;
        while (ptr_nil_node != ptr_next_node->ptr_left)
        {
            ptr_next_node = ptr_next_node->ptr_left;
        }
    }
    else
    {
        ptr_parent = ptr_node->ptr_parent;
        ptr_next_node = (CMLIB_RBT_NODE_T *) ptr_node;
        while ((ptr_nil_node != ptr_parent) &&
               (ptr_parent->ptr_right == ptr_next_node))
        {
            ptr_next_node = ptr_next_node->ptr_parent;
            ptr_parent = ptr_next_node->ptr_parent;
        }

        if (ptr_nil_node != ptr_parent)
        {
            ptr_next_node = ptr_parent;
        }
        else
        {
            ptr_next_node = NULL;
        }
    }

    return ptr_next_node;
}

CMLIB_RBT_NODE_T *
cmlib_rbt_getPrev(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    const CMLIB_RBT_NODE_T  *ptr_node)
{
    CMLIB_RBT_NODE_T        *ptr_prev_node;
    CMLIB_RBT_NODE_T        *ptr_parent;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    if ((NULL == ptr_head) ||
        (NULL == ptr_node))
    {
        return NULL;
    }
    else if ((&(ptr_head->nil_node)) == ptr_node)
    {
        return NULL;
    }
    ptr_nil_node = &(ptr_head->nil_node);

    if (ptr_nil_node != ptr_node->ptr_left)
    {
        ptr_prev_node = ptr_node->ptr_left;
        while (ptr_nil_node != ptr_prev_node->ptr_right)
        {
            ptr_prev_node = ptr_prev_node->ptr_right;
        }
    }
    else
    {
        ptr_parent = ptr_node->ptr_parent;
        ptr_prev_node = (CMLIB_RBT_NODE_T *) ptr_node;
        while ((ptr_nil_node != ptr_parent) &&
               (ptr_parent->ptr_left == ptr_prev_node))
        {
            ptr_prev_node = ptr_prev_node->ptr_parent;
            ptr_parent = ptr_prev_node->ptr_parent;
        }

        if (ptr_nil_node != ptr_parent)
        {
            ptr_prev_node = ptr_parent;
        }
        else
        {
            ptr_prev_node = NULL;
        }
    }

    return ptr_prev_node;
}

CLX_ERROR_NO_T
cmlib_rbt_traverse(
    const CMLIB_RBT_HEAD_T      *ptr_head,
    const CMLIB_RBT_TRAV_FUNC_T trav_func,
    void                        *ptr_cookie)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_RBT_NODE_T        *ptr_node;
    CMLIB_RBT_NODE_T        *node_path[CMLIB_RBT_MAX_HEIGHT];
    I32_T                   depth = 0;
    void                    *ptr_user_param;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(trav_func);
    ptr_nil_node = &(ptr_head->nil_node);
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_node)
    {
        if (ptr_nil_node != ptr_node->ptr_left)
        {
            node_path[depth++] = ptr_node;
            ptr_node = ptr_node->ptr_left;
        }
        else
        {
            rc = trav_func(ptr_user_param, ptr_node->ptr_data, ptr_cookie);
            if (CLX_E_OK != rc)
            {
                return rc;
            }

            while (ptr_nil_node == ptr_node->ptr_right)
            {
                if (depth > 0)
                {
                    ptr_node = node_path[--depth];
                    rc = trav_func(ptr_user_param, ptr_node->ptr_data, ptr_cookie);
                    if (CLX_E_OK != rc)
                    {
                        return rc;
                    }
                }
                else
                {
                    break;
                }
            }
            ptr_node = ptr_node->ptr_right;
        }
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_getCount(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    UI32_T                  *ptr_count)
{
    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_count);

    *ptr_count = ptr_head->node_count;

    return CLX_E_OK;
}

CLX_ERROR_NO_T
cmlib_rbt_find(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_RBT_NODE_T        *ptr_node;
    I32_T                   ret;
    CMLIB_RBT_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node);
    ptr_nil_node = &(ptr_head->nil_node);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_node)
    {
        ret = cmp_func(ptr_user_param, ptr_data, ptr_node->ptr_data);
        if (ret < 0)
        {
            ptr_node = ptr_node->ptr_left;
        }
        else if (ret > 0)
        {
            ptr_node = ptr_node->ptr_right;
        }
        else
        {
            break;
        }
    }

    if (ptr_nil_node != ptr_node)
    {
        *pptr_node = ptr_node;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_findNext(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_RBT_NODE_T        *ptr_node;
    CMLIB_RBT_NODE_T        *ptr_next_node = NULL;
    I32_T                   ret;
    CMLIB_RBT_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node);
    ptr_nil_node = &(ptr_head->nil_node);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_node)
    {
        ret = cmp_func(ptr_user_param, ptr_data, ptr_node->ptr_data);
        if (ret < 0)
        {
            ptr_next_node = ptr_node;
            ptr_node = ptr_node->ptr_left;
        }
        else if (ret > 0)
        {
            ptr_node = ptr_node->ptr_right;
        }
        else
        {
            break;
        }
    }

    if ((ptr_nil_node != ptr_node) &&
        (ptr_nil_node != ptr_node->ptr_right))
    {
        ptr_node = ptr_node->ptr_right;
        while (ptr_nil_node != ptr_node->ptr_left)
        {
            ptr_node = ptr_node->ptr_left;
        }
        ptr_next_node = ptr_node;
    }

    if (NULL != ptr_next_node)
    {
        *pptr_node = ptr_next_node;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_findGE(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node)
{
    CLX_ERROR_NO_T  rc;

    rc = cmlib_rbt_find(ptr_head, ptr_data, pptr_node);
    if (CLX_E_OK != rc)
    {
        rc = cmlib_rbt_findNext(ptr_head, ptr_data, pptr_node);
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_rbt_findPrev(
    const CMLIB_RBT_HEAD_T  *ptr_head,
    void                    *ptr_data,
    CMLIB_RBT_NODE_T        **pptr_node)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_RBT_NODE_T        *ptr_node;
    CMLIB_RBT_NODE_T        *ptr_prev_node = NULL;
    I32_T                   ret;
    CMLIB_RBT_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;
    const CMLIB_RBT_NODE_T  *ptr_nil_node;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node);
    ptr_nil_node = &(ptr_head->nil_node);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (ptr_nil_node != ptr_node)
    {
        ret = cmp_func(ptr_user_param, ptr_data, ptr_node->ptr_data);
        if (ret < 0)
        {
            ptr_node = ptr_node->ptr_left;
        }
        else if (ret > 0)
        {
            ptr_prev_node = ptr_node;
            ptr_node = ptr_node->ptr_right;
        }
        else
        {
            break;
        }
    }

    if ((ptr_nil_node != ptr_node) &&
        (ptr_nil_node != ptr_node->ptr_left))
    {
        ptr_node = ptr_node->ptr_left;
        while (ptr_nil_node != ptr_node->ptr_right)
        {
            ptr_node = ptr_node->ptr_right;
        }
        ptr_prev_node = ptr_node;
    }

    if (NULL != ptr_prev_node)
    {
        *pptr_node = ptr_prev_node;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

/* LOCAL SUBPROGRAM BODIES
*/
static CMLIB_RBT_NODE_T *
_cmlib_rbt_rotateRight(
    CMLIB_RBT_HEAD_T    *ptr_head,
    CMLIB_RBT_NODE_T    *ptr_root)
{
    CMLIB_RBT_NODE_T    *ptr_new_root = ptr_root->ptr_left;
    CMLIB_RBT_NODE_T    *ptr_new_left = ptr_new_root->ptr_right;
    CMLIB_RBT_NODE_T    *ptr_nil_node = &(ptr_head->nil_node);

    ptr_new_root->ptr_right = ptr_root;
    ptr_root->ptr_left = ptr_new_left;
    if (ptr_nil_node != ptr_root->ptr_parent)
    {
        if (ptr_root->ptr_parent->ptr_left == ptr_root)
        {
            ptr_root->ptr_parent->ptr_left = ptr_new_root;
        }
        else
        {
            ptr_root->ptr_parent->ptr_right = ptr_new_root;
        }
    }
    else
    {
        ptr_head->ptr_root = ptr_new_root;
    }

    ptr_new_root->ptr_parent = ptr_root->ptr_parent;
    ptr_root->ptr_parent = ptr_new_root;
    ptr_new_left->ptr_parent = ptr_root;

    return ptr_new_root;
}

static CMLIB_RBT_NODE_T *
_cmlib_rbt_rotateLeft(
    CMLIB_RBT_HEAD_T    *ptr_head,
    CMLIB_RBT_NODE_T    *ptr_root)
{
    CMLIB_RBT_NODE_T    *ptr_new_root = ptr_root->ptr_right;
    CMLIB_RBT_NODE_T    *ptr_new_right = ptr_new_root->ptr_left;
    CMLIB_RBT_NODE_T    *ptr_nil_node = &(ptr_head->nil_node);

    ptr_new_root->ptr_left = ptr_root;
    ptr_root->ptr_right = ptr_new_right;
    if (ptr_nil_node != ptr_root->ptr_parent)
    {
        if (ptr_root->ptr_parent->ptr_left == ptr_root)
        {
            ptr_root->ptr_parent->ptr_left = ptr_new_root;
        }
        else
        {
            ptr_root->ptr_parent->ptr_right = ptr_new_root;
        }
    }
    else
    {
        ptr_head->ptr_root = ptr_new_root;
    }

    ptr_new_root->ptr_parent = ptr_root->ptr_parent;
    ptr_root->ptr_parent = ptr_new_root;
    ptr_new_right->ptr_parent = ptr_root;

    return ptr_new_root;
}
