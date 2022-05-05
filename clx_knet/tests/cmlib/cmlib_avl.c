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

/* FILE NAME:  cmlib_avl.c
 * PURPOSE:
 * NOTES:
 */
/* INCLUDE FILE DECLARATIONS
 */
#include <cmlib/cmlib_avl.h>

/* NAMING CONSTANT DECLARATIONS
 */
#define CMLIB_AVL_MAX_COUNT     (0xFFFFFFFF) /* the maximum value of node_count in avl_head */
#define CMLIB_AVL_MAX_HEIGHT    (46) /* the max height for node_count=0x11e8d0a3f */

/* MACRO FUNCTION DECLARATIONS
 */
#define CMLIB_AVL_MAX_VAL(__val_1__, __val_2__) (((__val_1__) >= (__val_2__)) ? (__val_1__) : (__val_2__))
#define CMLIB_AVL_NODE_HEIGHT(__ptr_node__) ((NULL != (__ptr_node__)) ? (__ptr_node__)->height : 0)
#define CMLIB_AVL_ALLOC_NODE(__ptr_head__, __ptr_node__) \
    do { \
        if (0 == (__ptr_head__)->capacity) \
        { \
            (__ptr_node__) = osal_alloc(sizeof(CMLIB_AVL_NODE_T)); \
        } \
        else \
        { \
            (__ptr_node__) = cmlib_mpool_alloc((__ptr_head__)->ptr_node_pool); \
        } \
    } while (0)
#define CMLIB_AVL_FREE_NODE(__ptr_head__, __ptr_node__) \
    do { \
        if (0 == (__ptr_head__)->capacity) \
        { \
            osal_free((__ptr_node__)); \
        } \
        else \
        { \
            cmlib_mpool_free((__ptr_head__)->ptr_node_pool, (void *) (__ptr_node__)); \
        } \
    } while (0)

/* DATA TYPE DECLARATIONS
 */

/* GLOBAL VARIABLE DECLARATIONS
 */
DIAG_SET_MODULE_INFO(CLX_MODULE_CMLIB, "cmlib_avl.c");

/* LOCAL SUBPROGRAM DECLARATIONS
 */
static CMLIB_AVL_NODE_T *
_cmlib_avl_rotateRight(
    CMLIB_AVL_NODE_T    *ptr_root);

static CMLIB_AVL_NODE_T *
_cmlib_avl_rotateLeft(
    CMLIB_AVL_NODE_T    *ptr_root);

static CMLIB_AVL_NODE_T *
_cmlib_avl_balance(
    CMLIB_AVL_NODE_T    *ptr_root,
    const I32_T         height_diff);

/* STATIC VARIABLE DECLARATIONS
 */

/* EXPORTED SUBPROGRAM BODIES
 */
CLX_ERROR_NO_T
cmlib_avl_create(
    void                        *ptr_user_param,
    const UI32_T                capacity,
    const CMLIB_AVL_CMP_FUNC_T  cmp_func,
    const C8_T                  *ptr_name,
    CMLIB_AVL_HEAD_T            **pptr_head)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    CMLIB_AVL_HEAD_T    *ptr_head;

    HAL_CHECK_PTR(pptr_head);
    HAL_CHECK_PTR(cmp_func);
    HAL_CHECK_PTR(ptr_name);

    ptr_head = osal_alloc(sizeof(CMLIB_AVL_HEAD_T));
    if(NULL == ptr_head)
    {
        return CLX_E_NO_MEMORY;
    }
    osal_memset(ptr_head, 0x0, sizeof(CMLIB_AVL_HEAD_T));
    ptr_head->ptr_name = (C8_T *) ptr_name;
    ptr_head->cmp_func = cmp_func;
    ptr_head->ptr_user_param = ptr_user_param;
    ptr_head->capacity = capacity;

    if(capacity > 0)
    {
        rc = cmlib_mpool_create(sizeof(CMLIB_AVL_NODE_T),
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
cmlib_avl_destroy(
    const CMLIB_AVL_HEAD_T          *ptr_head,
    const CMLIB_AVL_DESTROY_FUNC_T  destroy_func)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    CMLIB_AVL_NODE_T    *ptr_node;
    CMLIB_AVL_NODE_T    *ptr_new_root;
    void                *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (NULL != ptr_node)
    {
        /* rotate the tree until the node to be deleted being the root */
        if (NULL != ptr_node->ptr_left)
        {
            /* both of the node and its left child are valid */
            ptr_node = _cmlib_avl_rotateRight(ptr_node);
        }
        else
        {
            ptr_new_root = ptr_node;
            ptr_node = ptr_new_root->ptr_right;

            if (NULL != destroy_func)
            {
                destroy_func(ptr_user_param, ptr_new_root->ptr_data);
            }
            CMLIB_AVL_FREE_NODE(ptr_head, ptr_new_root);
        }
    }

    if(ptr_head->capacity > 0)
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
cmlib_avl_insert(
    CMLIB_AVL_HEAD_T    *ptr_head,
    void                *ptr_data,
    const BOOL_T        overwrite_flag,
    void                **pptr_overwritten_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_AVL_NODE_T        **pptr_node;
    CMLIB_AVL_NODE_T        *ptr_node;
    CMLIB_AVL_NODE_T        **node_path[CMLIB_AVL_MAX_HEIGHT];
    I32_T                   depth = 0;
    I32_T                   ret;
    CMLIB_AVL_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    if((TRUE == overwrite_flag) &&
       (NULL == pptr_overwritten_data))
    {
        return CLX_E_BAD_PARAMETER;
    }
    else if (CMLIB_AVL_MAX_COUNT == ptr_head->node_count)
    {
        return CLX_E_TABLE_FULL;
    }
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    pptr_node = &(ptr_head->ptr_root);
    ptr_node = *pptr_node;
    /* traverse tree to the leaf */
    while (NULL != ptr_node)
    {
        node_path[depth++] = pptr_node;

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
            if(TRUE == overwrite_flag)
            {
                *pptr_overwritten_data = ptr_node->ptr_data;
                ptr_node->ptr_data = ptr_data;
            }
            else
            {
                rc = CLX_E_ENTRY_EXISTS;
            }
            return rc;
        }
        ptr_node = *pptr_node;
    }

    /* insert the new leaf node */
    CMLIB_AVL_ALLOC_NODE(ptr_head, ptr_node);
    if (NULL != ptr_node)
    {
        osal_memset(ptr_node, 0x0, sizeof(CMLIB_AVL_NODE_T));
        ptr_node->height = 1;
        ptr_node->ptr_data = ptr_data;
        *pptr_node = ptr_node;
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
        I32_T   height_diff;
        I32_T   left_height;
        I32_T   right_height;

        /* trace the tree back to the root to reblance the subtree if need */
        while ((--depth) >= 0)
        {
            pptr_node = node_path[depth];
            ptr_node = *pptr_node;
            left_height = CMLIB_AVL_NODE_HEIGHT(ptr_node->ptr_left);
            right_height = CMLIB_AVL_NODE_HEIGHT(ptr_node->ptr_right);

            ptr_node->height = 1 + CMLIB_AVL_MAX_VAL(left_height, right_height);
            height_diff = right_height - left_height;
            switch (height_diff)
            {
                case -2:
                case 2:
                    *pptr_node = _cmlib_avl_balance(ptr_node, height_diff);
                    /* break for-loop */
                    depth = 0;
                    break;
                default:
                    break;
            }
        }
        ptr_head->node_count++;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_avl_lookup(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_node_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_AVL_NODE_T        *ptr_node;
    I32_T                   ret;
    CMLIB_AVL_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node_data);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (NULL != ptr_node)
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

    if (NULL != ptr_node)
    {
        *pptr_node_data = ptr_node->ptr_data;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_avl_delete(
    CMLIB_AVL_HEAD_T    *ptr_head,
    void                *ptr_data,
    void                **pptr_node_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_AVL_NODE_T        *ptr_node;
    CMLIB_AVL_NODE_T        **pptr_node;
    CMLIB_AVL_NODE_T        **node_path[CMLIB_AVL_MAX_HEIGHT];
    I32_T                   depth = 0;
    I32_T                   height_diff;
    I32_T                   right_height;
    I32_T                   left_height;
    I32_T                   ret;
    CMLIB_AVL_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_node_data);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    pptr_node = &(ptr_head->ptr_root);
    ptr_node = *pptr_node;
    /* traverse the tree to find the corresponding node */
    while (NULL != ptr_node)
    {
        node_path[depth++] = pptr_node;

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
            *pptr_node_data = ptr_node->ptr_data;
            break;
        }
        ptr_node = *pptr_node;
    }

    if (NULL != ptr_node)
    {
        /* delete node according to different cases */
        if ((NULL != ptr_node->ptr_left) &&
            (NULL != ptr_node->ptr_right))
        {
            CMLIB_AVL_NODE_T    *ptr_branch_node = ptr_node;
            CMLIB_AVL_NODE_T    *ptr_leaf_node;

            /* replace the node with the max node in left branch */
            pptr_node = &(ptr_node->ptr_left);
            ptr_node = *pptr_node;
            while (NULL != ptr_node)
            {
                node_path[depth++] = pptr_node;

                pptr_node = &(ptr_node->ptr_right);
                ptr_node = *pptr_node;
            }
            ptr_leaf_node = *node_path[depth - 1];
            ptr_branch_node->ptr_data = ptr_leaf_node->ptr_data;
            *node_path[--depth] = ptr_leaf_node->ptr_left;
            ptr_node = ptr_leaf_node;
        }
        else if ((NULL == ptr_node->ptr_left) &&
                 (NULL == ptr_node->ptr_right))
        {
            *node_path[--depth] = NULL;
        }
        else
        {
            *node_path[--depth] = (NULL != ptr_node->ptr_left) ? ptr_node->ptr_left :
                                                                 ptr_node->ptr_right;
        }
        CMLIB_AVL_FREE_NODE(ptr_head, ptr_node);

        /* trace the tree back to the root to reblance the subtree if need */
        while ((--depth) >= 0)
        {
            pptr_node = node_path[depth];
            ptr_node = *pptr_node;
            left_height = CMLIB_AVL_NODE_HEIGHT(ptr_node->ptr_left);
            right_height = CMLIB_AVL_NODE_HEIGHT(ptr_node->ptr_right);

            ptr_node->height = 1 + CMLIB_AVL_MAX_VAL(left_height, right_height);
            height_diff = right_height - left_height;
            switch (height_diff)
            {
                case -2:
                case 2:
                    *pptr_node = _cmlib_avl_balance(ptr_node, height_diff);
                    break;
                default:
                    break;
            }
        }
        ptr_head->node_count--;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_avl_traverse(
    const CMLIB_AVL_HEAD_T      *ptr_head,
    const CMLIB_AVL_TRAV_FUNC_T trav_func,
    void                        *ptr_cookie)
{
    CLX_ERROR_NO_T      rc = CLX_E_OK;
    CMLIB_AVL_NODE_T    *ptr_node;
    CMLIB_AVL_NODE_T    *node_path[CMLIB_AVL_MAX_HEIGHT];
    I32_T               depth = 0;
    void                *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(trav_func);
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    while (NULL != ptr_node)
    {
        /* use stack to do DFS in FILO manner */
        if (NULL != ptr_node->ptr_left)
        {
            /* traverse the branch node with left child */
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

            /* traverse back the subtree or node path with righ child */
            while (NULL == ptr_node->ptr_right)
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
cmlib_avl_getCount(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    UI32_T                  *ptr_count)
{
    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_count);

    *ptr_count = ptr_head->node_count;

    return CLX_E_OK;
}

CLX_ERROR_NO_T
cmlib_avl_getNextData(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_next_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_AVL_NODE_T        *ptr_node;
    CMLIB_AVL_NODE_T        *ptr_next_node = NULL;
    I32_T                   ret;
    CMLIB_AVL_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_next_data);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    /* traverse the tree to find the corresponding node */
    while (NULL != ptr_node)
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

    if ((NULL != ptr_node) &&
        (NULL != ptr_node->ptr_right))
    {
        ptr_node = ptr_node->ptr_right;
        while (NULL != ptr_node->ptr_left)
        {
            ptr_node = ptr_node->ptr_left;
        }
        ptr_next_node = ptr_node;
    }

    if (NULL != ptr_next_node)
    {
        *pptr_next_data = ptr_next_node->ptr_data;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

CLX_ERROR_NO_T
cmlib_avl_getPrevData(
    const CMLIB_AVL_HEAD_T  *ptr_head,
    void                    *ptr_data,
    void                    **pptr_prev_data)
{
    CLX_ERROR_NO_T          rc = CLX_E_OK;
    CMLIB_AVL_NODE_T        *ptr_node;
    CMLIB_AVL_NODE_T        *ptr_prev_node = NULL;
    I32_T                   ret;
    CMLIB_AVL_CMP_FUNC_T    cmp_func;
    void                    *ptr_user_param;

    HAL_CHECK_PTR(ptr_head);
    HAL_CHECK_PTR(ptr_data);
    HAL_CHECK_PTR(pptr_prev_data);
    cmp_func = ptr_head->cmp_func;
    ptr_user_param = ptr_head->ptr_user_param;

    ptr_node = ptr_head->ptr_root;
    /* traverse the tree to find the corresponding node */
    while (NULL != ptr_node)
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

    if ((NULL != ptr_node) &&
        (NULL != ptr_node->ptr_left))
    {
        ptr_node = ptr_node->ptr_left;
        while (NULL != ptr_node->ptr_right)
        {
            ptr_node = ptr_node->ptr_right;
        }
        ptr_prev_node = ptr_node;
    }

    if (NULL != ptr_prev_node)
    {
        *pptr_prev_data = ptr_prev_node->ptr_data;
    }
    else
    {
        rc = CLX_E_ENTRY_NOT_FOUND;
    }

    return rc;
}

/* LOCAL SUBPROGRAM BODIES
 */
static CMLIB_AVL_NODE_T *
_cmlib_avl_rotateRight(
    CMLIB_AVL_NODE_T    *ptr_root)
{
    CMLIB_AVL_NODE_T    *ptr_new_root = ptr_root->ptr_left;
    CMLIB_AVL_NODE_T    *ptr_new_left = ptr_new_root->ptr_right;

    ptr_new_root->ptr_right = ptr_root;
    ptr_root->ptr_left = ptr_new_left;

    ptr_root->height = 1 +
                       CMLIB_AVL_MAX_VAL(CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_left),
                                         CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_right));
    ptr_new_root->height = 1 +
                           CMLIB_AVL_MAX_VAL(CMLIB_AVL_NODE_HEIGHT(ptr_new_root->ptr_left),
                                             CMLIB_AVL_NODE_HEIGHT(ptr_new_root->ptr_right));

    return ptr_new_root;
}

static CMLIB_AVL_NODE_T *
_cmlib_avl_rotateLeft(
    CMLIB_AVL_NODE_T    *ptr_root)
{
    CMLIB_AVL_NODE_T    *ptr_new_root = ptr_root->ptr_right;
    CMLIB_AVL_NODE_T    *ptr_new_right = ptr_new_root->ptr_left;

    ptr_new_root->ptr_left = ptr_root;
    ptr_root->ptr_right = ptr_new_right;

    ptr_root->height = 1 +
                       CMLIB_AVL_MAX_VAL(CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_left),
                                         CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_right));
    ptr_new_root->height = 1 +
                           CMLIB_AVL_MAX_VAL(CMLIB_AVL_NODE_HEIGHT(ptr_new_root->ptr_left),
                                             CMLIB_AVL_NODE_HEIGHT(ptr_new_root->ptr_right));

    return ptr_new_root;
}

static CMLIB_AVL_NODE_T *
_cmlib_avl_balance(
    CMLIB_AVL_NODE_T    *ptr_root,
    const I32_T         height_diff)
{
    if (2 == height_diff)
    {
        /* equal sign for deletion */
        if (CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_right->ptr_right) >= CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_right->ptr_left))
        {
            return _cmlib_avl_rotateLeft(ptr_root);
        }
        else
        {
            ptr_root->ptr_right = _cmlib_avl_rotateRight(ptr_root->ptr_right);
            return _cmlib_avl_rotateLeft(ptr_root);
        }
    }
    else
    {
        /* equal sign for deletion */
        if (CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_left->ptr_left) >= CMLIB_AVL_NODE_HEIGHT(ptr_root->ptr_left->ptr_right))
        {
            return _cmlib_avl_rotateRight(ptr_root);
        }
        else
        {
            ptr_root->ptr_left = _cmlib_avl_rotateLeft(ptr_root->ptr_left);
            return _cmlib_avl_rotateRight(ptr_root);
        }
    }
}
