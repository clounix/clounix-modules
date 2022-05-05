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

/* FILE NAME:  cmlib_bitmap.c
 * PURPOSE:
 *   It provides API for bitmap operation
 * NOTES:
 *
 */
 /* INCLUDE FILE DECLARATIONS
  */
#include <clx_error.h>
#include <cmlib/cmlib_bitmap.h>
#include <cmlib/cmlib_util.h>

/* NAMING CONSTANT DECLARATIONS
*/
/* MACRO FUNCTION DECLARATIONS
*/
/* DATA TYPE DECLARATIONS
*/
/* GLOBAL VARIABLE DECLARATIONS
*/
/* LOCAL SUBPROGRAM DECLARATIONS
*/
/* STATIC VARIABLE DECLARATIONS
*/

/* EXPORTED SUBPROGRAM BODIES
*/

/* FUNCTION NAME:   cmlib_bitmap_empty
 * PURPOSE:
 *      cmlib_bitmap_empty() is a function to check if ptr_bitmap is empty.
 *
 * INPUT:
 *      ptr_bitmap   -- The specified bitmap pointer.
 *      word            -- The size of specified bitmap.
 * OUTPUT:
 *      None
 * RETURN:
 *      UI32_T       -- return 1 or 0. 1: empty, 0: not empty.
 *
 * NOTES:
 *      None
 *
 */
UI32_T
cmlib_bitmap_empty(
    const   UI32_T  *ptr_bitmap,
    const   UI32_T   word   )
{
    UI32_T i;
    for (i = 0; i < word; i++)
    {
        if (ptr_bitmap[i] != 0)
        {
            return 0;
        }
    }
    return 1;
}

/* FUNCTION NAME:   cmlib_bitmap_getFreeIdx
 * PURPOSE:
 *      cmlib_bitmap_getFreeIdx() is a function to check if ptr_bitmap is empty.
 *
 * INPUT:
 *      ptr_bitmap   -- The specified bitmap pointer.
 *      word            -- The size of specified bitmap.
 * OUTPUT:
 *      ptr_idx         -- Found pointer index.
 * RETURN:
 *      CLX_E_OK              -- get the free index success.
 *      CLX_E_ENTRY_NOT_FOUND -- get the free index failed.
 *
 * NOTES:
 *      None
 *
 */
CLX_ERROR_NO_T
cmlib_bitmap_getFreeIdx(
    const UI32_T   *ptr_bitmap,
    const UI32_T   word,
          UI32_T   *ptr_idx)
{
    UI32_T i, pos;
    CLX_ERROR_NO_T rc = CLX_E_TABLE_FULL;

    for(i=0; i<word; i++)
    {
        if(0xFFFFFFFF != ptr_bitmap[i])
        {
            pos = cmlib_util_findFirstZero(ptr_bitmap[i]);
            *ptr_idx = (i*32) + pos;
            rc = CLX_E_OK;
            break;
        }
    }

    return rc;
}

